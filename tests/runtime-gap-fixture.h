#pragma once
// Opt-in native faults. Include after platform-failure-fixture.h, before the
// unchanged mod source. Default calls preserve the previously installed API.
#include <atomic>

namespace runtime_gap {
struct Faults {
    bool subclass = false;
    bool property = false;
    bool hook = false;
    bool mutex = false;
    bool procedure = false;
    bool library = false;
    bool timer = false;
    HWND visibleTab = nullptr;
    bool overrideVisibility = false;
    HANDLE scanEntered = nullptr;
    HANDLE scanRelease = nullptr;
    std::atomic<unsigned>* scanCalls = nullptr;
    LPTHREAD_START_ROUTINE workerWrapper = nullptr;
    LPTHREAD_START_ROUTINE workerOriginal = nullptr;
    unsigned libraryCalls = 0;
};
inline Faults faults;
struct Scope {
    Faults old = faults;
    Scope() { faults = {}; }
    ~Scope() { faults = old; }
};
}

namespace WindhawkUtils {
inline BOOL RuntimeSetWindowSubclassFromAnyThread(HWND window,
        WH_SUBCLASSPROC procedure, DWORD_PTR reference) {
    if (runtime_gap::faults.subclass) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    return SetWindowSubclassFromAnyThread(window, procedure, reference);
}
}
inline BOOL WINAPI RuntimeSetPropW(HWND window, LPCWSTR name, HANDLE value) {
    if (runtime_gap::faults.property) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    return SetPropW(window, name, value);
}
inline HHOOK WINAPI RuntimeSetWindowsHookExW(int id, HOOKPROC procedure,
        HINSTANCE module, DWORD thread) {
    if (runtime_gap::faults.hook) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return nullptr;
    }
    return SetWindowsHookExW(id, procedure, module, thread);
}
inline HANDLE WINAPI RuntimeCreateMutexW(LPSECURITY_ATTRIBUTES attributes,
        BOOL owner, LPCWSTR name) {
    if (runtime_gap::faults.mutex) {
        SetLastError(ERROR_ACCESS_DENIED);
        return nullptr;
    }
    return CreateMutexW(attributes, owner, name);
}
inline FARPROC WINAPI RuntimeGetProcAddress(HMODULE module, LPCSTR name) {
    if (runtime_gap::faults.procedure) return nullptr;
    return GetProcAddress(module, name);
}
inline HMODULE WINAPI RuntimeLoadLibraryExW(LPCWSTR name, HANDLE file, DWORD flags) {
    if (runtime_gap::faults.library) {
        ++runtime_gap::faults.libraryCalls;
        SetLastError(ERROR_MOD_NOT_FOUND);
        return nullptr;
    }
    return LoadLibraryExW(name, file, flags);
}
inline UINT_PTR WINAPI RuntimeSetTimer(HWND window, UINT_PTR id, UINT timeout,
        TIMERPROC procedure) {
    if (runtime_gap::faults.timer) return 0;
    return SetTimer(window, id, timeout, procedure);
}
inline BOOL WINAPI RuntimeIsWindowVisible(HWND window) {
    const auto& f = runtime_gap::faults;
    if (f.overrideVisibility) return window == f.visibleTab;
    return IsWindowVisible(window);
}
inline auto RuntimePriorKnownFolderPath = &SHGetKnownFolderPath;
#undef SHGetKnownFolderPath
inline HRESULT WINAPI RuntimeSHGetKnownFolderPath(REFKNOWNFOLDERID id,
        DWORD flags, HANDLE token, PWSTR* result) {
    const auto& f = runtime_gap::faults;
    if (id == FOLDERID_Recent && f.scanEntered) {
        if (f.scanCalls) ++*f.scanCalls;
        SetEvent(f.scanEntered);
        if (WaitForSingleObject(f.scanRelease, 5000) != WAIT_OBJECT_0) return E_ABORT;
    }
    return RuntimePriorKnownFolderPath(id, flags, token, result);
}
inline auto RuntimePriorCreateThread = &CreateThread;
#undef CreateThread
inline HANDLE WINAPI RuntimeCreateThread(LPSECURITY_ATTRIBUTES attributes,
        SIZE_T stack, LPTHREAD_START_ROUTINE procedure, LPVOID parameter,
        DWORD flags, LPDWORD id) {
    auto& f = runtime_gap::faults;
    if (f.workerWrapper) {
        f.workerOriginal = procedure;
        procedure = f.workerWrapper;
    }
    return RuntimePriorCreateThread(attributes, stack, procedure, parameter, flags, id);
}

#define SetWindowSubclassFromAnyThread RuntimeSetWindowSubclassFromAnyThread
#define SetPropW RuntimeSetPropW
#define SetWindowsHookExW RuntimeSetWindowsHookExW
#define CreateMutexW RuntimeCreateMutexW
#define GetProcAddress RuntimeGetProcAddress
#define LoadLibraryExW RuntimeLoadLibraryExW
#define SetTimer RuntimeSetTimer
#define IsWindowVisible RuntimeIsWindowVisible
#define SHGetKnownFolderPath RuntimeSHGetKnownFolderPath
#define CreateThread RuntimeCreateThread
