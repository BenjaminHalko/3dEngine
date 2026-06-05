#pragma once

// Custom miniaudio decoding backend for OGG/Vorbis, backed by the vendored
// public-domain stb_vorbis (External/stb/stb_vorbis.c).
//
// This is a GENERIC engine capability: registering this vtable with the
// resource manager lets ALL examples load .ogg files through miniaudio.
//
// Usage (AudioSystem::Initialize):
//   ma_decoding_backend_vtable* customBackends[] = { &g_ma_decoding_backend_vtable_stbvorbis };
//   resourceManagerConfig.ppCustomDecodingBackendVTables = customBackends;
//   resourceManagerConfig.customDecodingBackendCount      = 1;

#include <miniaudio/miniaudio.h>

// Defined in VorbisDecoder.cpp. Plug this into a ma_resource_manager_config's
// ppCustomDecodingBackendVTables array.
extern ma_decoding_backend_vtable g_ma_decoding_backend_vtable_stbvorbis;
