// D3DCompiler implementation for macOS
// Supports both runtime compilation (via Wine + fxc.exe) and loading pre-compiled .cso files

#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <map>

// Simple hash for shader caching
static size_t hashShaderSource(const void* data, SIZE_T size, const char* entry, const char* target) {
    size_t hash = 0;
    const char* bytes = static_cast<const char*>(data);
    for (SIZE_T i = 0; i < size; ++i) {
        hash = hash * 31 + bytes[i];
    }
    if (entry) {
        for (const char* p = entry; *p; ++p) hash = hash * 31 + *p;
    }
    if (target) {
        for (const char* p = target; *p; ++p) hash = hash * 31 + *p;
    }
    return hash;
}

// Shader cache to avoid recompiling the same shader
static std::map<size_t, std::vector<char>> g_shaderCache;

// COM-compatible blob implementation that inherits from ID3D10Blob
class BlobImpl : public ID3D10Blob {
private:
    void* m_data;
    SIZE_T m_size;
    ULONG m_refCount;

public:
    BlobImpl(const void* data, SIZE_T size) : m_refCount(1), m_size(size) {
        m_data = malloc(size);
        if (m_data && data) {
            memcpy(m_data, data, size);
        }
    }

    virtual ~BlobImpl() {
        if (m_data) free(m_data);
    }

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (!ppvObject) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D10Blob)) {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return ++m_refCount;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = --m_refCount;
        if (ref == 0) {
            delete this;
        }
        return ref;
    }

    // ID3D10Blob methods
    void* STDMETHODCALLTYPE GetBufferPointer() override {
        return m_data;
    }

    SIZE_T STDMETHODCALLTYPE GetBufferSize() override {
        return m_size;
    }
};

// Helper to try opening a file with case variations
static FILE* tryOpenFile(const std::string& path) {
    FILE* file = fopen(path.c_str(), "rb");
    if (file) return file;

    // Try with lowercase 'assets' instead of 'Assets'
    std::string altPath = path;
    size_t pos = altPath.find("Assets/");
    if (pos != std::string::npos) {
        altPath.replace(pos, 7, "assets/");
        file = fopen(altPath.c_str(), "rb");
    }
    return file;
}

// Load pre-compiled shader (.cso file)
static HRESULT LoadPrecompiledShader(
    const std::string& shaderPath,
    const char* pEntrypoint,
    ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs)
{
    // Build the .cso path: replace extension and append entry point
    // e.g., "Assets/Shaders/DoColor.fx" with entry "VS" -> "Assets/Shaders/DoColor_VS.cso"
    size_t lastDot = shaderPath.rfind('.');
    std::string baseName = (lastDot != std::string::npos) ? shaderPath.substr(0, lastDot) : shaderPath;
    std::string csoPath = baseName + "_" + pEntrypoint + ".cso";

    printf("D3DCompile: Trying to load pre-compiled shader: %s\n", csoPath.c_str());

    FILE* file = tryOpenFile(csoPath);
    if (!file) {
        // Try without entry point suffix
        csoPath = baseName + ".cso";
        file = tryOpenFile(csoPath);
    }

    if (!file) {
        return E_FAIL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        fclose(file);
        return E_FAIL;
    }

    std::vector<char> data(size);
    fread(data.data(), 1, size, file);
    fclose(file);

    // Verify DXBC header
    if (size >= 4 && memcmp(data.data(), "DXBC", 4) == 0) {
        *ppCode = new BlobImpl(data.data(), size);
        return S_OK;
    }

    return E_FAIL;
}

// Convert Unix path to Wine Z: path
static std::string toWinePath(const std::string& unixPath) {
    std::string winePath = "Z:";
    for (char c : unixPath) {
        winePath += (c == '/') ? '\\' : c;
    }
    return winePath;
}

// Runtime shader compilation using Wine + fxc.exe
static HRESULT CompileShaderWithFxc(
    const void* pSrcData,
    SIZE_T SrcDataSize,
    const char* pEntrypoint,
    const char* pTarget,
    ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs)
{
    // Check cache first
    size_t hash = hashShaderSource(pSrcData, SrcDataSize, pEntrypoint, pTarget);
    auto it = g_shaderCache.find(hash);
    if (it != g_shaderCache.end()) {
        *ppCode = new BlobImpl(it->second.data(), it->second.size());
        return S_OK;
    }

    // Create temp directory for shader compilation
    char tempDir[] = "/tmp/d3dcompile_XXXXXX";
    if (!mkdtemp(tempDir)) {
        if (ppErrorMsgs) {
            std::string err = "Failed to create temp directory";
            *ppErrorMsgs = new BlobImpl(err.c_str(), err.size() + 1);
        }
        return E_FAIL;
    }

    std::string srcPath = std::string(tempDir) + "/shader.hlsl";
    std::string outPath = std::string(tempDir) + "/shader.cso";
    std::string errPath = std::string(tempDir) + "/error.txt";

    // Write shader source to temp file
    FILE* srcFile = fopen(srcPath.c_str(), "wb");
    if (!srcFile) {
        rmdir(tempDir);
        if (ppErrorMsgs) {
            std::string err = "Failed to write shader source";
            *ppErrorMsgs = new BlobImpl(err.c_str(), err.size() + 1);
        }
        return E_FAIL;
    }
    fwrite(pSrcData, 1, SrcDataSize, srcFile);
    fclose(srcFile);

    // Build fxc command - look for fxc.exe in known locations
    std::string fxcPath;
    const char* fxcLocations[] = {
        "Tools/fxc/fxc.exe",
        "../Tools/fxc/fxc.exe",
        "../../Tools/fxc/fxc.exe",
        "../../../Tools/fxc/fxc.exe",
        nullptr
    };

    for (const char** loc = fxcLocations; *loc; ++loc) {
        struct stat st;
        if (stat(*loc, &st) == 0) {
            char absPath[PATH_MAX];
            if (realpath(*loc, absPath)) {
                fxcPath = absPath;
                break;
            }
        }
    }

    // Build Wine command
    std::string cmd = "wine \"" + toWinePath(fxcPath) + "\" "
                     "/T " + pTarget + " "
                     "/E " + pEntrypoint + " "
                     "/Fo \"" + toWinePath(outPath) + "\" "
                     "\"" + toWinePath(srcPath) + "\" "
                     "2>\"" + errPath + "\"";

    int ret = system(cmd.c_str());

    // Read error output if any
    std::string errorOutput;
    FILE* errFile = fopen(errPath.c_str(), "r");
    if (errFile) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), errFile)) {
            errorOutput += buf;
        }
        fclose(errFile);
    }

    // Read compiled shader
    FILE* outFile = fopen(outPath.c_str(), "rb");
    if (!outFile || ret != 0) {
        // Compilation failed
        if (outFile) fclose(outFile);
        unlink(srcPath.c_str());
        unlink(outPath.c_str());
        unlink(errPath.c_str());
        rmdir(tempDir);

        if (ppErrorMsgs && !errorOutput.empty()) {
            *ppErrorMsgs = new BlobImpl(errorOutput.c_str(), errorOutput.size() + 1);
        } else if (ppErrorMsgs) {
            std::string err = "Shader compilation failed (fxc.exe returned " + std::to_string(ret) + ")";
            *ppErrorMsgs = new BlobImpl(err.c_str(), err.size() + 1);
        }
        fprintf(stderr, "D3DCompile failed: %s\n", errorOutput.c_str());
        return E_FAIL;
    }

    fseek(outFile, 0, SEEK_END);
    long size = ftell(outFile);
    fseek(outFile, 0, SEEK_SET);

    std::vector<char> data(size);
    fread(data.data(), 1, size, outFile);
    fclose(outFile);

    // Cleanup temp files
    unlink(srcPath.c_str());
    unlink(outPath.c_str());
    unlink(errPath.c_str());
    rmdir(tempDir);

    // Verify DXBC header
    if (size < 4 || memcmp(data.data(), "DXBC", 4) != 0) {
        if (ppErrorMsgs) {
            std::string err = "Compiled shader has invalid format";
            *ppErrorMsgs = new BlobImpl(err.c_str(), err.size() + 1);
        }
        return E_FAIL;
    }

    // Cache the result
    g_shaderCache[hash] = data;

    *ppCode = new BlobImpl(data.data(), size);
    fprintf(stderr, "D3DCompile: Successfully compiled shader (%s, %s)\n", pEntrypoint, pTarget);
    return S_OK;
}

// D3DCompiler API implementations
HRESULT WINAPI D3DCompile(
    const void* pSrcData,
    SIZE_T SrcDataSize,
    const char* pSourceName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const char* pEntrypoint,
    const char* pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs)
{
    return CompileShaderWithFxc(pSrcData, SrcDataSize, pEntrypoint, pTarget, ppCode, ppErrorMsgs);
}

// Helper to compile from file - tries pre-compiled first, then runtime compilation
static HRESULT CompileFromFileImpl(
    const std::string& fileName,
    const char* pEntrypoint,
    const char* pTarget,
    ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs)
{
    // First try to load pre-compiled shader
    HRESULT hr = LoadPrecompiledShader(fileName, pEntrypoint, ppCode, ppErrorMsgs);
    if (SUCCEEDED(hr)) {
        return hr;
    }

    // Pre-compiled not found, try runtime compilation
    // Read the source file
    FILE* file = tryOpenFile(fileName);
    if (!file) {
        std::string errorMsg = "Shader source file not found: " + fileName;
        fprintf(stderr, "D3DCompileFromFile: %s\n", errorMsg.c_str());
        if (ppErrorMsgs) {
            *ppErrorMsgs = new BlobImpl(errorMsg.c_str(), errorMsg.size() + 1);
        }
        return E_FAIL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::vector<char> source(size);
    fread(source.data(), 1, size, file);
    fclose(file);

    // Compile at runtime
    return CompileShaderWithFxc(source.data(), size, pEntrypoint, pTarget, ppCode, ppErrorMsgs);
}

// ANSI version - used on macOS where std::filesystem::path uses char strings
#undef D3DCompileFromFile  // Undefine the macro to expose the real function name
HRESULT WINAPI D3DCompileFromFileA(
    const char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const char* pEntrypoint,
    const char* pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs)
{
    return CompileFromFileImpl(pFileName, pEntrypoint, pTarget, ppCode, ppErrorMsgs);
}

// Wide character version - kept for compatibility
HRESULT WINAPI D3DCompileFromFileW(
    const WCHAR* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const char* pEntrypoint,
    const char* pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob** ppCode,
    ID3DBlob** ppErrorMsgs)
{
    // Convert wide string to narrow
    std::string narrowFileName;
    for (const WCHAR* p = pFileName; *p; p++) {
        narrowFileName += (char)(*p & 0x7F);
    }

    return CompileFromFileImpl(narrowFileName, pEntrypoint, pTarget, ppCode, ppErrorMsgs);
}

HRESULT WINAPI D3DReadFileToBlob(const WCHAR* pFileName, ID3DBlob** ppContents)
{
    if (!ppContents) return E_POINTER;

    std::string filename;
    for (const WCHAR* p = pFileName; *p; p++) {
        filename += (char)(*p & 0x7F);
    }

    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) return E_FAIL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::vector<char> data(size);
    fread(data.data(), 1, size, file);
    fclose(file);

    *ppContents = new BlobImpl(data.data(), size);
    return S_OK;
}

HRESULT WINAPI D3DWriteBlobToFile(ID3DBlob* pBlob, const WCHAR* pFileName, WINBOOL bOverwrite)
{
    if (!pBlob || !pFileName) return E_POINTER;

    std::string filename;
    for (const WCHAR* p = pFileName; *p; p++) {
        filename += (char)(*p & 0x7F);
    }

    FILE* file = fopen(filename.c_str(), bOverwrite ? "wb" : "wbx");
    if (!file) return E_FAIL;

    fwrite(pBlob->GetBufferPointer(), 1, pBlob->GetBufferSize(), file);
    fclose(file);
    return S_OK;
}

HRESULT WINAPI D3DCreateBlob(SIZE_T Size, ID3DBlob** ppBlob)
{
    if (!ppBlob) return E_POINTER;
    *ppBlob = new BlobImpl(nullptr, Size);
    return S_OK;
}

HRESULT WINAPI D3DGetBlobPart(
    const void* pSrcData,
    SIZE_T SrcDataSize,
    D3D_BLOB_PART Part,
    UINT Flags,
    ID3DBlob** ppPart)
{
    return E_NOTIMPL;
}
