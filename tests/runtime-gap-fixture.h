#pragma once
// Opt-in native faults. Include after platform-failure-fixture.h, before the
// unchanged mod source. Default calls preserve the previously installed API.

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

#define SetWindowSubclassFromAnyThread RuntimeSetWindowSubclassFromAnyThread
#define SetPropW RuntimeSetPropW
#define SetWindowsHookExW RuntimeSetWindowsHookExW
#define CreateMutexW RuntimeCreateMutexW
#define GetProcAddress RuntimeGetProcAddress
#define LoadLibraryExW RuntimeLoadLibraryExW
#define SetTimer RuntimeSetTimer
#define IsWindowVisible RuntimeIsWindowVisible
