#include "core/paths.h"

#include <filesystem>

#if !DGFX_EDITOR
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

// The folder of the running exe. Not the working directory: that changes with how the exe is started.
// ponytail: converts the path to the ANSI code page, so a folder name outside it fails. Use wide paths if needed.
static std::string exeFolder() {
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD length = 0;

    // A path longer than the buffer is cut. Then try again with a larger buffer.
    while ((length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()))) == buffer.size())
        buffer.resize(buffer.size() * 2);

    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path().generic_string() + "/";
#else
    return std::filesystem::read_symlink("/proc/self/exe").parent_path().generic_string() + "/";
#endif
}
#endif

const std::string& assetRoot() {
#if DGFX_EDITOR
    static const std::string root = ASSET_ROOT;
#else
    static const std::string root = exeFolder();
#endif
    return root;
}
