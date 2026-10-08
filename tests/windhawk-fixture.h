#pragma once
// Rename editor-only no-op APIs, then supply isolated in-memory test doubles.
// Include this before the real mod; no production source or SDK is rewritten.
#define Wh_GetStringValue Editor_GetStringValue
#define Wh_SetStringValue Editor_SetStringValue
#define Wh_GetIntSetting Editor_GetIntSetting
#define Wh_GetStringSetting Editor_GetStringSetting
#define Wh_Log Editor_Log
#define Wh_SetFunctionHook Editor_SetFunctionHook
#define Wh_ApplyHookOperations Editor_ApplyHookOperations
#include <windhawk_api.h>
#undef Wh_GetStringValue
#undef Wh_SetStringValue
#undef Wh_GetIntSetting
#undef Wh_GetStringSetting
#undef Wh_Log
#undef Wh_SetFunctionHook
#undef Wh_ApplyHookOperations
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <shlobj.h>
#include <shellapi.h>

namespace fixture {
inline std::map<std::wstring, std::wstring> storage;
inline std::map<std::wstring, std::wstring> strings;
inline std::map<std::wstring, int> integers;
inline bool failWrites = false;
inline size_t writes = 0;
inline bool logging = false;
using ComCreation = HRESULT(*)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
inline ComCreation creation = nullptr;
inline std::map<int, SHORT> keys;
inline std::vector<std::pair<std::wstring, std::wstring>> launches;
inline INT_PTR launchResult = 33;
inline std::map<std::wstring, UINT> drives;
inline std::map<std::wstring, std::pair<DWORD, DWORD>> attributes;
inline HWND activeWindow = nullptr;
inline HWND focusWindow = nullptr;
inline bool hookSuccess = false;
inline bool symbolSuccess = false;
inline unsigned symbolCalls = 0;
inline unsigned hookApplications = 0;
inline std::map<std::wstring, HMODULE> modules;
inline void Reset() {
    storage.clear();
    strings.clear();
    integers.clear();
    failWrites = false;
    writes = 0;
    logging = false;
    creation = nullptr;
    keys.clear();
    launches.clear();
    launchResult = 33;
    drives.clear();
    attributes.clear();
    activeWindow = nullptr;
    focusWindow = nullptr;
    hookSuccess = false;
    symbolSuccess = false;
    symbolCalls = 0;
    hookApplications = 0;
    modules.clear();
}
}

inline size_t Wh_GetStringValue(PCWSTR key, PWSTR buffer, size_t capacity) {
    auto it = fixture::storage.find(key);
    if (it == fixture::storage.end() || it->second.size() >= capacity) {
        if (capacity) buffer[0] = 0;
        return 0;
    }
    std::copy(it->second.begin(), it->second.end(), buffer);
    buffer[it->second.size()] = 0;
    return it->second.size();
}
inline BOOL Wh_SetStringValue(PCWSTR key, PCWSTR value) {
    ++fixture::writes;
    if (fixture::failWrites) return FALSE;
    fixture::storage[key] = value;
    return TRUE;
}
inline int Wh_GetIntSetting(PCWSTR key, ...) {
    return fixture::integers[key];
}
inline PCWSTR Wh_GetStringSetting(PCWSTR key, ...) {
    return fixture::strings[key].c_str();
}
inline void Wh_Log(PCWSTR format, ...) {
    if (!fixture::logging) return;
    va_list args;
    va_start(args, format);
    std::vfwprintf(stderr, format, args);
    std::fputwc(L'\n', stderr);
    va_end(args);
}
inline BOOL Wh_SetFunctionHook(void* target, void*, void** original) {
    if (!fixture::hookSuccess) return FALSE;
    if (original) *original = target;
    return TRUE;
}
inline BOOL Wh_ApplyHookOperations() { ++fixture::hookApplications; return TRUE; }
#define HookSymbols Editor_HookSymbols
#include <windhawk_utils.h>
#undef HookSymbols
namespace WindhawkUtils {
inline bool HookSymbols(HMODULE, const SYMBOL_HOOK*, size_t,
                        const WH_HOOK_SYMBOLS_OPTIONS* = nullptr) {
    ++fixture::symbolCalls;
    // Tests supply typed originals explicitly; this stub never resolves symbols
    // or installs a hook in either this process or Explorer.
    return fixture::symbolSuccess;
}
}

inline BOOL WINAPI FixtureEnumWindows(WNDENUMPROC callback, LPARAM parameter) {
    struct Context { WNDENUMPROC callback; LPARAM parameter; } context{callback, parameter};
    return EnumWindows([](HWND window, LPARAM opaque) -> BOOL {
        DWORD process = 0;
        GetWindowThreadProcessId(window, &process);
        if (process != GetCurrentProcessId()) return TRUE;
        auto& current = *reinterpret_cast<Context*>(opaque);
        return current.callback(window, current.parameter);
    }, reinterpret_cast<LPARAM>(&context));
}
#define EnumWindows FixtureEnumWindows

inline auto realComCreation = &CoCreateInstance;
inline HRESULT WINAPI FixtureCoCreateInstance(REFCLSID clsid, LPUNKNOWN outer,
                                              DWORD context, REFIID iid, LPVOID* output) {
    if (fixture::creation) return fixture::creation(clsid, outer, context, iid, output);
    return CoCreateInstance(clsid, outer, context, iid, output);
}
inline HINSTANCE WINAPI FixtureShellExecuteW(HWND, LPCWSTR operation, LPCWSTR file,
                                             LPCWSTR, LPCWSTR, INT) {
    fixture::launches.emplace_back(operation ? operation : L"", file ? file : L"");
    return reinterpret_cast<HINSTANCE>(fixture::launchResult);
}
inline SHORT WINAPI FixtureGetKeyState(int key) { return fixture::keys[key]; }
inline SHORT WINAPI FixtureGetAsyncKeyState(int key) { return fixture::keys[key]; }
inline UINT WINAPI FixtureGetDriveTypeW(LPCWSTR root) {
    auto found = fixture::drives.find(root ? root : L"");
    return found == fixture::drives.end() ? GetDriveTypeW(root) : found->second;
}
inline DWORD WINAPI FixtureGetFileAttributesW(LPCWSTR path) {
    auto found = fixture::attributes.find(path);
    if (found == fixture::attributes.end()) return GetFileAttributesW(path);
    SetLastError(found->second.second);
    return found->second.first;
}
#define CoCreateInstance FixtureCoCreateInstance
#define ShellExecuteW FixtureShellExecuteW
#define GetKeyState FixtureGetKeyState
#define GetAsyncKeyState FixtureGetAsyncKeyState
#define GetDriveTypeW FixtureGetDriveTypeW
#define GetFileAttributesW FixtureGetFileAttributesW
inline HWND WINAPI FixtureGetActiveWindow() {
    return fixture::activeWindow ? fixture::activeWindow : GetActiveWindow();
}
#define GetActiveWindow FixtureGetActiveWindow
inline HWND WINAPI FixtureGetFocus() {
    return fixture::focusWindow ? fixture::focusWindow : GetFocus();
}
#define GetFocus FixtureGetFocus
inline HMODULE WINAPI FixtureGetModuleHandleW(LPCWSTR name) {
    const auto found = fixture::modules.find(name ? name : L"");
    return found == fixture::modules.end() ? GetModuleHandleW(name) : found->second;
}
#define GetModuleHandleW FixtureGetModuleHandleW
