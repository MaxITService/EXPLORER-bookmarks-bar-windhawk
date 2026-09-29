#pragma once
// Rename editor-only no-op APIs, then supply isolated in-memory test doubles.
// Include this before the real mod; no production source or SDK is rewritten.
#define Wh_GetStringValue Editor_GetStringValue
#define Wh_SetStringValue Editor_SetStringValue
#define Wh_GetIntSetting Editor_GetIntSetting
#define Wh_GetStringSetting Editor_GetStringSetting
#include <windhawk_api.h>
#undef Wh_GetStringValue
#undef Wh_SetStringValue
#undef Wh_GetIntSetting
#undef Wh_GetStringSetting
#include <map>
#include <string>
#include <algorithm>
#include <shlobj.h>

namespace fixture {
inline std::map<std::wstring, std::wstring> storage;
inline std::map<std::wstring, std::wstring> strings;
inline std::map<std::wstring, int> integers;
inline bool failWrites = false;
inline size_t writes = 0;
inline std::wstring recentDirectory;
inline std::map<std::wstring, DWORD> registry;
inline void Reset() {
    storage.clear();
    strings.clear();
    integers.clear();
    failWrites = false;
    writes = 0;
    recentDirectory.clear();
    registry.clear();
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

inline LSTATUS WINAPI FixtureRegGetValueW(HKEY root, LPCWSTR key, LPCWSTR name,
        DWORD, LPDWORD type, PVOID data, LPDWORD size) {
    const auto id = std::to_wstring(reinterpret_cast<uintptr_t>(root)) + L"\\" + key + L"\\" + name;
    const auto found = fixture::registry.find(id);
    if (found == fixture::registry.end()) return ERROR_FILE_NOT_FOUND;
    if (!size || *size < sizeof(DWORD)) return ERROR_MORE_DATA;
    *size = sizeof(DWORD);
    if (type) *type = REG_DWORD;
    if (data) *static_cast<DWORD*>(data) = found->second;
    return ERROR_SUCCESS;
}
inline HRESULT WINAPI FixtureSHGetKnownFolderPath(REFKNOWNFOLDERID id, DWORD flags,
                                                  HANDLE token, PWSTR* result) {
    if (id != FOLDERID_Recent || fixture::recentDirectory.empty()) {
        return SHGetKnownFolderPath(id, flags, token, result);
    }
    const size_t bytes = (fixture::recentDirectory.size() + 1) * sizeof(wchar_t);
    *result = static_cast<PWSTR>(CoTaskMemAlloc(bytes));
    if (!*result) return E_OUTOFMEMORY;
    memcpy(*result, fixture::recentDirectory.c_str(), bytes);
    return S_OK;
}
#define RegGetValueW FixtureRegGetValueW
#define SHGetKnownFolderPath FixtureSHGetKnownFolderPath
