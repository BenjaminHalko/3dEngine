// Custom miniaudio decoding backend for OGG/Vorbis backed by stb_vorbis.
//
// stb_vorbis is a whole-buffer decoder (stb_vorbis_open_memory): it needs the
// entire encoded file resident in memory. We therefore drain the miniaudio
// read/seek/tell callbacks (or the file / memory block) into a private buffer
// once at init time, then decode f32/interleaved PCM on demand.
//
// Structural template: miniaudio's official extras/decoders/libvorbis backend
// (mackron/miniaudio), with every libvorbis call swapped for its stb_vorbis
// equivalent.
//
// Key contract for beat-sync (task 18): onGetCursor MUST advance. stb_vorbis
// cannot report a reliable Vorbis length, so onGetLength returns
// MA_NOT_IMPLEMENTED (length == 0 is acceptable), but the cursor is tracked as
// the cumulative number of PCM frames read and therefore always advances.

#include "Precompiled.h"
#include "VorbisDecoder.h"

#include <miniaudio/miniaudio.h>

#include <climits>
#include <cstring>

// Pull in ONLY the stb_vorbis declarations here. The implementation lives in
// its own translation unit (External/stb/stb_vorbis.c) so we avoid ODR issues
// and keep the upstream single-file decoder pristine/unmodified.
extern "C"
{
#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>
#undef STB_VORBIS_HEADER_ONLY
}

namespace
{
struct ma_stbvorbis
{
    ma_data_source_base ds; // Base data source. MUST be the first member.
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    void* pReadSeekTellUserData;
    ma_format format; // Always f32.
    stb_vorbis* stb;
    ma_uint32 channels;
    ma_uint32 sampleRate;
    ma_uint64 cursor; // Cumulative frames read. MUST advance (beat-sync depends on this).
    unsigned char* pData; // Whole encoded file, owned by this backend.
    size_t dataSize;
    ma_allocation_callbacks allocationCallbacks;
};

ma_result ma_stbvorbis_read_pcm_frames(ma_stbvorbis* p, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead);
ma_result ma_stbvorbis_seek_to_pcm_frame(ma_stbvorbis* p, ma_uint64 frameIndex);
ma_result ma_stbvorbis_get_data_format(
    ma_stbvorbis* p, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap);
ma_result ma_stbvorbis_get_cursor_in_pcm_frames(ma_stbvorbis* p, ma_uint64* pCursor);
ma_result ma_stbvorbis_get_length_in_pcm_frames(ma_stbvorbis* p, ma_uint64* pLength);

// ---- ma_data_source vtable shims -------------------------------------------

ma_result ma_stbvorbis_ds_read(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
{
    return ma_stbvorbis_read_pcm_frames(static_cast<ma_stbvorbis*>(pDataSource), pFramesOut, frameCount, pFramesRead);
}

ma_result ma_stbvorbis_ds_seek(ma_data_source* pDataSource, ma_uint64 frameIndex)
{
    return ma_stbvorbis_seek_to_pcm_frame(static_cast<ma_stbvorbis*>(pDataSource), frameIndex);
}

ma_result ma_stbvorbis_ds_get_data_format(
    ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap)
{
    return ma_stbvorbis_get_data_format(
        static_cast<ma_stbvorbis*>(pDataSource), pFormat, pChannels, pSampleRate, pChannelMap, channelMapCap);
}

ma_result ma_stbvorbis_ds_get_cursor(ma_data_source* pDataSource, ma_uint64* pCursor)
{
    return ma_stbvorbis_get_cursor_in_pcm_frames(static_cast<ma_stbvorbis*>(pDataSource), pCursor);
}

ma_result ma_stbvorbis_ds_get_length(ma_data_source* pDataSource, ma_uint64* pLength)
{
    return ma_stbvorbis_get_length_in_pcm_frames(static_cast<ma_stbvorbis*>(pDataSource), pLength);
}

ma_data_source_vtable g_ma_stbvorbis_ds_vtable = {
    ma_stbvorbis_ds_read,
    ma_stbvorbis_ds_seek,
    ma_stbvorbis_ds_get_data_format,
    ma_stbvorbis_ds_get_cursor,
    ma_stbvorbis_ds_get_length,
    NULL, // onSetLooping
    0     // flags
};

// ---- Helpers ---------------------------------------------------------------

// Drain the entire encoded stream provided by the miniaudio callbacks into a
// freshly allocated buffer owned by the backend. stb_vorbis needs the whole
// file up-front.
ma_result ma_stbvorbis_read_entire_stream(ma_stbvorbis* p)
{
    size_t capacity = 0;
    size_t size = 0;
    unsigned char* pBuffer = NULL;

    for (;;)
    {
        if (size == capacity)
        {
            size_t newCapacity = (capacity == 0) ? (64 * 1024) : (capacity * 2);
            unsigned char* pNew = static_cast<unsigned char*>(ma_realloc(pBuffer, newCapacity, &p->allocationCallbacks));
            if (pNew == NULL)
            {
                ma_free(pBuffer, &p->allocationCallbacks);
                return MA_OUT_OF_MEMORY;
            }
            pBuffer = pNew;
            capacity = newCapacity;
        }

        size_t bytesRead = 0;
        ma_result result = p->onRead(p->pReadSeekTellUserData, pBuffer + size, capacity - size, &bytesRead);
        size += bytesRead;

        if (result != MA_SUCCESS || bytesRead == 0)
        {
            // MA_AT_END (or any error after partial read) terminates the drain.
            break;
        }
    }

    if (size == 0)
    {
        ma_free(pBuffer, &p->allocationCallbacks);
        return MA_INVALID_FILE;
    }

    p->pData = pBuffer;
    p->dataSize = size;
    return MA_SUCCESS;
}

// Common init: opens stb_vorbis on the already-populated p->pData buffer and
// caches the stream format.
ma_result ma_stbvorbis_post_open(ma_stbvorbis* p)
{
    if (p->dataSize > INT_MAX)
    {
        return MA_TOO_BIG;
    }

    int error = 0;
    p->stb = stb_vorbis_open_memory(p->pData, static_cast<int>(p->dataSize), &error, NULL);
    if (p->stb == NULL)
    {
        return MA_INVALID_FILE;
    }

    stb_vorbis_info info = stb_vorbis_get_info(p->stb);
    p->channels = static_cast<ma_uint32>(info.channels);
    p->sampleRate = info.sample_rate;
    p->cursor = 0;
    return MA_SUCCESS;
}

ma_result ma_stbvorbis_init_internal(const ma_decoding_backend_config* pConfig, ma_stbvorbis* p)
{
    if (p == NULL)
    {
        return MA_INVALID_ARGS;
    }

    std::memset(p, 0, sizeof(*p));
    p->format = ma_format_f32; // stb_vorbis decodes to f32 interleaved.

    (void)pConfig; // We always serve f32; preferredFormat is ignored.

    ma_data_source_config dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &g_ma_stbvorbis_ds_vtable;

    return ma_data_source_init(&dataSourceConfig, &p->ds);
}

ma_result ma_stbvorbis_init(
    ma_read_proc onRead,
    ma_seek_proc onSeek,
    ma_tell_proc onTell,
    void* pReadSeekTellUserData,
    const ma_decoding_backend_config* pConfig,
    const ma_allocation_callbacks* pAllocationCallbacks,
    ma_stbvorbis* p)
{
    if (onRead == NULL || onSeek == NULL)
    {
        return MA_INVALID_ARGS;
    }

    ma_result result = ma_stbvorbis_init_internal(pConfig, p);
    if (result != MA_SUCCESS)
    {
        return result;
    }

    p->onRead = onRead;
    p->onSeek = onSeek;
    p->onTell = onTell;
    p->pReadSeekTellUserData = pReadSeekTellUserData;
    if (pAllocationCallbacks != NULL)
    {
        p->allocationCallbacks = *pAllocationCallbacks;
    }

    result = ma_stbvorbis_read_entire_stream(p);
    if (result != MA_SUCCESS)
    {
        ma_data_source_uninit(&p->ds);
        return result;
    }

    result = ma_stbvorbis_post_open(p);
    if (result != MA_SUCCESS)
    {
        ma_free(p->pData, &p->allocationCallbacks);
        ma_data_source_uninit(&p->ds);
        return result;
    }

    return MA_SUCCESS;
}

ma_result ma_stbvorbis_init_memory(
    const void* pInputData,
    size_t inputDataSize,
    const ma_decoding_backend_config* pConfig,
    const ma_allocation_callbacks* pAllocationCallbacks,
    ma_stbvorbis* p)
{
    ma_result result = ma_stbvorbis_init_internal(pConfig, p);
    if (result != MA_SUCCESS)
    {
        return result;
    }

    if (pAllocationCallbacks != NULL)
    {
        p->allocationCallbacks = *pAllocationCallbacks;
    }

    // Copy so the backend owns a stable buffer for stb_vorbis's lifetime.
    p->pData = static_cast<unsigned char*>(ma_malloc(inputDataSize, &p->allocationCallbacks));
    if (p->pData == NULL)
    {
        ma_data_source_uninit(&p->ds);
        return MA_OUT_OF_MEMORY;
    }
    std::memcpy(p->pData, pInputData, inputDataSize);
    p->dataSize = inputDataSize;

    result = ma_stbvorbis_post_open(p);
    if (result != MA_SUCCESS)
    {
        ma_free(p->pData, &p->allocationCallbacks);
        ma_data_source_uninit(&p->ds);
        return result;
    }

    return MA_SUCCESS;
}

void ma_stbvorbis_uninit(ma_stbvorbis* p, const ma_allocation_callbacks* pAllocationCallbacks)
{
    if (p == NULL)
    {
        return;
    }

    (void)pAllocationCallbacks;

    if (p->stb != NULL)
    {
        stb_vorbis_close(p->stb);
        p->stb = NULL;
    }
    ma_free(p->pData, &p->allocationCallbacks);
    p->pData = NULL;

    ma_data_source_uninit(&p->ds);
}

ma_result ma_stbvorbis_read_pcm_frames(ma_stbvorbis* p, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
{
    if (pFramesRead != NULL)
    {
        *pFramesRead = 0;
    }
    if (frameCount == 0)
    {
        return MA_INVALID_ARGS;
    }
    if (p == NULL || p->stb == NULL)
    {
        return MA_INVALID_ARGS;
    }

    float* pOut = static_cast<float*>(pFramesOut);
    ma_uint64 totalFramesRead = 0;
    ma_result result = MA_SUCCESS;

    while (totalFramesRead < frameCount)
    {
        ma_uint64 framesRemaining = frameCount - totalFramesRead;
        int framesToRead = (framesRemaining > 1024) ? 1024 : static_cast<int>(framesRemaining);

        float* pDst = pOut + (totalFramesRead * p->channels);
        int framesRead = stb_vorbis_get_samples_float_interleaved(
            p->stb, static_cast<int>(p->channels), pDst, framesToRead * static_cast<int>(p->channels));
        if (framesRead <= 0)
        {
            result = MA_AT_END;
            break;
        }

        totalFramesRead += static_cast<ma_uint64>(framesRead);
    }

    p->cursor += totalFramesRead;

    if (pFramesRead != NULL)
    {
        *pFramesRead = totalFramesRead;
    }
    if (result == MA_SUCCESS && totalFramesRead == 0)
    {
        result = MA_AT_END;
    }
    return result;
}

ma_result ma_stbvorbis_seek_to_pcm_frame(ma_stbvorbis* p, ma_uint64 frameIndex)
{
    if (p == NULL || p->stb == NULL)
    {
        return MA_INVALID_ARGS;
    }
    if (frameIndex > UINT_MAX)
    {
        return MA_INVALID_ARGS;
    }

    if (stb_vorbis_seek(p->stb, static_cast<unsigned int>(frameIndex)) == 0)
    {
        return MA_ERROR;
    }

    p->cursor = frameIndex;
    return MA_SUCCESS;
}

ma_result ma_stbvorbis_get_data_format(
    ma_stbvorbis* p, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap)
{
    if (pFormat != NULL)
    {
        *pFormat = ma_format_unknown;
    }
    if (pChannels != NULL)
    {
        *pChannels = 0;
    }
    if (pSampleRate != NULL)
    {
        *pSampleRate = 0;
    }
    if (pChannelMap != NULL)
    {
        std::memset(pChannelMap, 0, sizeof(*pChannelMap) * channelMapCap);
    }

    if (p == NULL)
    {
        return MA_INVALID_OPERATION;
    }

    if (pFormat != NULL)
    {
        *pFormat = p->format;
    }
    if (pChannels != NULL)
    {
        *pChannels = p->channels;
    }
    if (pSampleRate != NULL)
    {
        *pSampleRate = p->sampleRate;
    }
    if (pChannelMap != NULL)
    {
        ma_channel_map_init_standard(ma_standard_channel_map_vorbis, pChannelMap, channelMapCap, p->channels);
    }
    return MA_SUCCESS;
}

ma_result ma_stbvorbis_get_cursor_in_pcm_frames(ma_stbvorbis* p, ma_uint64* pCursor)
{
    if (pCursor == NULL)
    {
        return MA_INVALID_ARGS;
    }
    *pCursor = 0;
    if (p == NULL)
    {
        return MA_INVALID_ARGS;
    }

    // Cumulative frames decoded so far -> the cursor ADVANCES (beat-sync needs this).
    *pCursor = p->cursor;
    return MA_SUCCESS;
}

ma_result ma_stbvorbis_get_length_in_pcm_frames(ma_stbvorbis* p, ma_uint64* pLength)
{
    if (pLength != NULL)
    {
        *pLength = 0;
    }
    (void)p;
    // Vorbis length via stb_vorbis is unreliable for our purposes; intentionally
    // unsupported. Cursor (above) still advances, which is what matters.
    return MA_NOT_IMPLEMENTED;
}

// ---- Decoding backend vtable entry points ----------------------------------

ma_result ma_decoding_backend_init__stbvorbis(
    void* pUserData,
    ma_read_proc onRead,
    ma_seek_proc onSeek,
    ma_tell_proc onTell,
    void* pReadSeekTellUserData,
    const ma_decoding_backend_config* pConfig,
    const ma_allocation_callbacks* pAllocationCallbacks,
    ma_data_source** ppBackend)
{
    (void)pUserData;

    ma_stbvorbis* p = static_cast<ma_stbvorbis*>(ma_malloc(sizeof(*p), pAllocationCallbacks));
    if (p == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_result result = ma_stbvorbis_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, p);
    if (result != MA_SUCCESS)
    {
        ma_free(p, pAllocationCallbacks);
        return result;
    }

    *ppBackend = p;
    return MA_SUCCESS;
}

ma_result ma_decoding_backend_init_memory__stbvorbis(
    void* pUserData,
    const void* pData,
    size_t dataSize,
    const ma_decoding_backend_config* pConfig,
    const ma_allocation_callbacks* pAllocationCallbacks,
    ma_data_source** ppBackend)
{
    (void)pUserData;

    ma_stbvorbis* p = static_cast<ma_stbvorbis*>(ma_malloc(sizeof(*p), pAllocationCallbacks));
    if (p == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_result result = ma_stbvorbis_init_memory(pData, dataSize, pConfig, pAllocationCallbacks, p);
    if (result != MA_SUCCESS)
    {
        ma_free(p, pAllocationCallbacks);
        return result;
    }

    *ppBackend = p;
    return MA_SUCCESS;
}

void ma_decoding_backend_uninit__stbvorbis(void* pUserData, ma_data_source* pBackend, const ma_allocation_callbacks* pAllocationCallbacks)
{
    (void)pUserData;

    ma_stbvorbis* p = static_cast<ma_stbvorbis*>(pBackend);
    ma_stbvorbis_uninit(p, pAllocationCallbacks);
    ma_free(p, pAllocationCallbacks);
}
} // namespace

// The vtable plugged into ma_resource_manager_config::ppCustomDecodingBackendVTables.
ma_decoding_backend_vtable g_ma_decoding_backend_vtable_stbvorbis = {
    ma_decoding_backend_init__stbvorbis,
    NULL, // onInitFile   - not needed; the resource manager drives onInit via its VFS.
    NULL, // onInitFileW
    ma_decoding_backend_init_memory__stbvorbis,
    ma_decoding_backend_uninit__stbvorbis};
