#pragma once
// Include after windhawk-fixture.h, before the real mod. Every fault is opt-in;
// normal calls use the fixture already installed or the real Windows API.
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <wincodec.h>
#include <string>
#include <vector>

namespace platform_failure {
enum class Transfer { Normal, Fail, Short };
enum class ShellIcon { Normal, Fail, NoIcon, OwnedIcon };
struct Faults {
    bool tempName = false;
    bool open = false;
    Transfer write = Transfer::Normal;
    Transfer read = Transfer::Normal;
    bool flush = false;
    bool move = false;
    bool size = false;
    bool overrideSize = false;
    LONGLONG fileSize = 0;
    bool wideSizing = false;
    bool wideConversion = false;
    bool utf8Sizing = false;
    bool utf8Conversion = false;
    bool environmentSizing = false;
    bool environmentOversize = false;
    bool environmentConversion = false;
    bool environmentChangedSize = false;
    bool knownFolder = false;
    HRESULT knownFolderResult = S_OK;
    bool nullKnownFolder = false;
    std::wstring folder;
    DWORD knownFolderFlags = 0;
    bool volume = false;
    bool volumeResult = false;
    std::wstring volumeName;
    bool event = false;
    bool thread = false;
    bool coInitialize = false;
    bool wait = false;
    ShellIcon shellIcon = ShellIcon::Normal;
    bool enumerate = false;
    std::vector<WIN32_FIND_DATAW> entries;
    size_t enumerationIndex = 0;
    unsigned tempCalls = 0;
    unsigned writeCalls = 0;
    unsigned flushCalls = 0;
    unsigned moveCalls = 0;
    unsigned deleteCalls = 0;
    unsigned destroyedIcons = 0;
    unsigned findNextCalls = 0;
    unsigned findCloseCalls = 0;
    std::wstring temporary;
};
inline Faults faults;
struct Scope {
    Faults old = std::move(faults);
    Scope() { faults = {}; }
    ~Scope() { faults = std::move(old); }
};
inline HANDLE FindHandle() { return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(0x7171)); }
}

inline UINT WINAPI PlatformGetTempFileNameW(LPCWSTR directory, LPCWSTR prefix, UINT unique, LPWSTR name) {
    auto& f = platform_failure::faults;
    ++f.tempCalls;
    if (f.tempName) { SetLastError(ERROR_ACCESS_DENIED); return 0; }
    const auto result = GetTempFileNameW(directory, prefix, unique, name);
    if (result) f.temporary = name;
    return result;
}
inline HANDLE WINAPI PlatformCreateFileW(LPCWSTR name, DWORD access, DWORD share,
        LPSECURITY_ATTRIBUTES security, DWORD disposition, DWORD flags, HANDLE templateFile) {
    if (platform_failure::faults.open) { SetLastError(ERROR_ACCESS_DENIED); return INVALID_HANDLE_VALUE; }
    return CreateFileW(name, access, share, security, disposition, flags, templateFile);
}
inline BOOL WINAPI PlatformWriteFile(HANDLE file, LPCVOID bytes, DWORD count, LPDWORD written, LPOVERLAPPED overlapped) {
    auto& f = platform_failure::faults;
    ++f.writeCalls;
    if (f.write == platform_failure::Transfer::Fail) { if (written) *written = 0; SetLastError(ERROR_DISK_FULL); return FALSE; }
    if (f.write == platform_failure::Transfer::Short && count) --count;
    return WriteFile(file, bytes, count, written, overlapped);
}
inline BOOL WINAPI PlatformReadFile(HANDLE file, LPVOID bytes, DWORD count, LPDWORD read, LPOVERLAPPED overlapped) {
    const auto fault = platform_failure::faults.read;
    if (fault == platform_failure::Transfer::Fail) { if (read) *read = 0; SetLastError(ERROR_READ_FAULT); return FALSE; }
    if (fault == platform_failure::Transfer::Short && count) --count;
    return ReadFile(file, bytes, count, read, overlapped);
}
inline BOOL WINAPI PlatformFlushFileBuffers(HANDLE file) {
    auto& f = platform_failure::faults;
    ++f.flushCalls;
    if (f.flush) { SetLastError(ERROR_WRITE_FAULT); return FALSE; }
    return FlushFileBuffers(file);
}
inline BOOL WINAPI PlatformMoveFileExW(LPCWSTR from, LPCWSTR to, DWORD flags) {
    auto& f = platform_failure::faults;
    ++f.moveCalls;
    if (f.move) { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    return MoveFileExW(from, to, flags);
}
inline BOOL WINAPI PlatformDeleteFileW(LPCWSTR path) {
    ++platform_failure::faults.deleteCalls;
    return DeleteFileW(path);
}
inline BOOL WINAPI PlatformGetFileSizeEx(HANDLE file, PLARGE_INTEGER size) {
    const auto& f = platform_failure::faults;
    if (f.size) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
    if (f.overrideSize) { size->QuadPart = f.fileSize; return TRUE; }
    return GetFileSizeEx(file, size);
}
inline int WINAPI PlatformWideCharToMultiByte(UINT codePage, DWORD flags, LPCWCH wide,
        int wideCount, LPSTR bytes, int byteCount, LPCCH defaultChar, LPBOOL usedDefault) {
    const auto& f = platform_failure::faults;
    if ((!bytes && f.wideSizing) || (bytes && f.wideConversion)) { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return 0; }
    return WideCharToMultiByte(codePage, flags, wide, wideCount, bytes, byteCount, defaultChar, usedDefault);
}
inline int WINAPI PlatformMultiByteToWideChar(UINT codePage, DWORD flags, LPCCH bytes,
        int byteCount, LPWSTR wide, int wideCount) {
    const auto& f = platform_failure::faults;
    if ((!wide && f.utf8Sizing) || (wide && f.utf8Conversion)) { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return 0; }
    return MultiByteToWideChar(codePage, flags, bytes, byteCount, wide, wideCount);
}
inline DWORD WINAPI PlatformExpandEnvironmentStringsW(LPCWSTR input, LPWSTR output, DWORD count) {
    const auto& f = platform_failure::faults;
    if (!output && f.environmentSizing) return 0;
    if (!output && f.environmentOversize) return 32769;
    if (output && f.environmentConversion) return 0;
    if (output && f.environmentChangedSize) return count + 1;
    return ExpandEnvironmentStringsW(input, output, count);
}
// Preserve the existing Recent-folder redirection rather than bypassing it.
inline auto PlatformPriorKnownFolderPath = &SHGetKnownFolderPath;
#undef SHGetKnownFolderPath
inline HRESULT WINAPI PlatformSHGetKnownFolderPath(REFKNOWNFOLDERID id, DWORD flags, HANDLE token, PWSTR* result) {
    const auto& f = platform_failure::faults;
    if (!f.knownFolder) return PlatformPriorKnownFolderPath(id, flags, token, result);
    platform_failure::faults.knownFolderFlags = flags;
    *result = nullptr;
    if (FAILED(f.knownFolderResult) || f.nullKnownFolder) return f.knownFolderResult;
    const size_t bytes = (f.folder.size() + 1) * sizeof(wchar_t);
    *result = static_cast<PWSTR>(CoTaskMemAlloc(bytes));
    if (!*result) return E_OUTOFMEMORY;
    memcpy(*result, f.folder.c_str(), bytes);
    return f.knownFolderResult;
}
inline BOOL WINAPI PlatformGetVolumeInformationW(LPCWSTR root, LPWSTR name, DWORD capacity,
        LPDWORD serial, LPDWORD maximumComponent, LPDWORD flags, LPWSTR fileSystem, DWORD fileSystemCapacity) {
    const auto& f = platform_failure::faults;
    if (!f.volume) return GetVolumeInformationW(root, name, capacity, serial, maximumComponent, flags, fileSystem, fileSystemCapacity);
    if (!f.volumeResult) return FALSE;
    if (capacity) { const size_t length = std::min<size_t>(f.volumeName.size(), capacity - 1); std::copy_n(f.volumeName.begin(), length, name); name[length] = 0; }
    return TRUE;
}
inline HANDLE WINAPI PlatformCreateEventW(LPSECURITY_ATTRIBUTES security, BOOL manualReset, BOOL initial, LPCWSTR name) {
    if (platform_failure::faults.event) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return nullptr; }
    return CreateEventW(security, manualReset, initial, name);
}
inline HANDLE WINAPI PlatformCreateThread(LPSECURITY_ATTRIBUTES security, SIZE_T stack, LPTHREAD_START_ROUTINE proc,
        LPVOID argument, DWORD flags, LPDWORD id) {
    if (platform_failure::faults.thread) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return nullptr; }
    return CreateThread(security, stack, proc, argument, flags, id);
}
inline HRESULT WINAPI PlatformCoInitializeEx(LPVOID reserved, DWORD apartment) {
    if (platform_failure::faults.coInitialize) return RPC_E_CHANGED_MODE;
    return CoInitializeEx(reserved, apartment);
}
inline DWORD WINAPI PlatformMsgWaitForMultipleObjectsEx(DWORD count, const HANDLE* handles, DWORD timeout, DWORD wakeMask, DWORD flags) {
    if (platform_failure::faults.wait) { SetLastError(ERROR_INVALID_HANDLE); return WAIT_FAILED; }
    return MsgWaitForMultipleObjectsEx(count, handles, timeout, wakeMask, flags);
}
inline DWORD_PTR WINAPI PlatformSHGetFileInfoW(LPCWSTR path, DWORD attributes, SHFILEINFOW* info, UINT size, UINT flags) {
    switch (platform_failure::faults.shellIcon) {
    case platform_failure::ShellIcon::Fail: return 0;
    case platform_failure::ShellIcon::NoIcon: info->hIcon = nullptr; return 1;
    case platform_failure::ShellIcon::OwnedIcon: info->hIcon = CopyIcon(LoadIconW(nullptr, IDI_APPLICATION)); return info->hIcon ? 1 : 0;
    default: return SHGetFileInfoW(path, attributes, info, size, flags);
    }
}
inline BOOL WINAPI PlatformDestroyIcon(HICON icon) {
    ++platform_failure::faults.destroyedIcons;
    return DestroyIcon(icon);
}
inline HANDLE WINAPI PlatformFindFirstFileExW(LPCWSTR name, FINDEX_INFO_LEVELS level, LPVOID data,
        FINDEX_SEARCH_OPS operation, LPVOID filter, DWORD flags) {
    auto& f = platform_failure::faults;
    if (!f.enumerate) return FindFirstFileExW(name, level, data, operation, filter, flags);
    f.enumerationIndex = 0;
    if (f.entries.empty()) { SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE; }
    *static_cast<WIN32_FIND_DATAW*>(data) = f.entries.front();
    return platform_failure::FindHandle();
}
inline BOOL WINAPI PlatformFindNextFileW(HANDLE handle, LPWIN32_FIND_DATAW data) {
    if (handle != platform_failure::FindHandle()) return FindNextFileW(handle, data);
    auto& f = platform_failure::faults;
    ++f.findNextCalls;
    if (++f.enumerationIndex >= f.entries.size()) { SetLastError(ERROR_NO_MORE_FILES); return FALSE; }
    *data = f.entries[f.enumerationIndex];
    return TRUE;
}
inline BOOL WINAPI PlatformFindClose(HANDLE handle) {
    if (handle != platform_failure::FindHandle()) return FindClose(handle);
    ++platform_failure::faults.findCloseCalls;
    return TRUE;
}

#define GetTempFileNameW PlatformGetTempFileNameW
#define CreateFileW PlatformCreateFileW
#define WriteFile PlatformWriteFile
#define ReadFile PlatformReadFile
#define FlushFileBuffers PlatformFlushFileBuffers
#define MoveFileExW PlatformMoveFileExW
#define DeleteFileW PlatformDeleteFileW
#define GetFileSizeEx PlatformGetFileSizeEx
#define WideCharToMultiByte PlatformWideCharToMultiByte
#define MultiByteToWideChar PlatformMultiByteToWideChar
#define ExpandEnvironmentStringsW PlatformExpandEnvironmentStringsW
#define SHGetKnownFolderPath PlatformSHGetKnownFolderPath
#define GetVolumeInformationW PlatformGetVolumeInformationW
#define CreateEventW PlatformCreateEventW
#define CreateThread PlatformCreateThread
#define CoInitializeEx PlatformCoInitializeEx
#define MsgWaitForMultipleObjectsEx PlatformMsgWaitForMultipleObjectsEx
#define SHGetFileInfoW PlatformSHGetFileInfoW
#define DestroyIcon PlatformDestroyIcon
#define FindFirstFileExW PlatformFindFirstFileExW
#define FindNextFileW PlatformFindNextFileW
#define FindClose PlatformFindClose
