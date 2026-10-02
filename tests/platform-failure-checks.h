#pragma once
// Fault injection exercises the real mod's error branches with test-owned
// files, read-only fake enumeration and reference-counted COM objects.

std::string PlatformReadBytes(const std::wstring& path) {
    platform_failure::Scope normal;
    ScopedFile input(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    LARGE_INTEGER size{};
    Require(input.value != INVALID_HANDLE_VALUE && GetFileSizeEx(input.value, &size),
            "platform backup verification open failed");
    std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    Require(ReadFile(input.value, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) &&
            read == bytes.size(), "platform backup verification read failed");
    return bytes;
}

void CheckBackupPlatformFailures() {
    fixture::Reset();
    BackupFixture backup;
    const std::string sentinel = "previous backup survives every failed save";
    fixture::storage[kBookmarksKey] = L"C:\\source";
    for (unsigned stage = 0; stage < 7; ++stage) {
        backup.Write(sentinel);
        platform_failure::Scope injected;
        auto& f = platform_failure::faults;
        if (stage == 0) f.tempName = true;
        if (stage == 1) f.open = true;
        if (stage == 2) f.write = platform_failure::Transfer::Fail;
        if (stage == 3) f.write = platform_failure::Transfer::Short;
        if (stage == 4) f.flush = true;
        if (stage == 5) f.move = true;
        if (stage == 6) f.wideConversion = true;
        Require(!SaveBackup(backup.file) && PlatformReadBytes(backup.file) == sentinel &&
                fixture::storage[kBookmarksKey] == L"C:\\source",
                "failed backup save replaced destination or changed bookmark storage");
        if (stage >= 1 && stage <= 5) {
            Require(!f.temporary.empty() && f.deleteCalls == 1 &&
                    GetFileAttributesW(f.temporary.c_str()) == INVALID_FILE_ATTRIBUTES,
                    "failed backup save leaked its temporary file");
        }
        Require(f.moveCalls == (stage == 5 ? 1u : 0u),
                "backup attempted atomic replacement before a complete durable write");
        Require(f.flushCalls == (stage == 4 || stage == 5 ? 1u : 0u),
                "backup flushed an unsuccessful or short write");
    }
    const std::string valid = "{\"format\":\"explorer-folder-bookmarks-bar\",\"version\":1,\"folders\":[\"C:\\\\imported\"]}";
    backup.Write(valid);
    const auto storage = fixture::storage;
    for (unsigned stage = 0; stage < 10; ++stage) {
        platform_failure::Scope injected;
        auto& f = platform_failure::faults;
        if (stage == 0) f.open = true;
        if (stage == 1) f.size = true;
        if (stage >= 2 && stage <= 5) {
            f.overrideSize = true;
            f.fileSize = stage == 2 ? -1 : stage == 3 ? 0 : stage == 4 ? kMaxImportBytes + 1 : 1;
        }
        if (stage == 6) f.read = platform_failure::Transfer::Fail;
        if (stage == 7) f.read = platform_failure::Transfer::Short;
        if (stage == 8) f.utf8Sizing = true;
        if (stage == 9) f.utf8Conversion = true;
        Require(!LoadBackup(backup.file) && fixture::storage == storage,
                "failed backup import changed persisted bookmarks");
    }
    Require(LoadBackup(backup.file) && fixture::storage[kBookmarksKey] == L"C:\\imported",
            "backup import did not recover after native faults were removed");
    fixture::Reset();
    std::puts("PASS: atomic backup preservation/cleanup, failed/short transfers, size and conversion failures");
}

void CheckEncodingAndKnownFolderFailures() {
    platform_failure::Scope isolated;
    auto& f = platform_failure::faults;
    std::string utf8 = "unchanged";
    std::wstring wide = L"unchanged";
    Require(!WideToUtf8(L"", utf8) && !Utf8ToWide("", wide),
            "empty encoding input unexpectedly succeeded");
    f.wideSizing = true;
    Require(!WideToUtf8(L"C:\\valid", utf8), "failed UTF-8 sizing accepted");
    f.wideSizing = false;
    f.wideConversion = true;
    Require(!WideToUtf8(L"C:\\valid", utf8), "failed UTF-8 second conversion accepted");
    f.wideConversion = false;
    f.utf8Conversion = true;
    Require(!Utf8ToWide("C:\\valid", wide), "failed UTF-16 second conversion accepted");
    f.utf8Conversion = false;
    for (auto flag : {&f.environmentSizing, &f.environmentOversize,
                      &f.environmentConversion, &f.environmentChangedSize}) {
        *flag = true;
        Require(ExpandFxPath(L"C:\\valid").empty(), "failed or changing environment expansion accepted");
        *flag = false;
    }
    f.knownFolder = true;
    f.knownFolderResult = E_ACCESSDENIED;
    Require(SuggestedBackupPath().empty() && KnownFolderPath(FOLDERID_Recent).empty() &&
            ScanWindowsRecentFolders().empty(), "failed known folder lookup returned a usable path");
    f.knownFolderResult = S_OK;
    f.nullKnownFolder = true;
    Require(SuggestedBackupPath().empty() && KnownFolderPath(FOLDERID_Desktop).empty(),
            "successful lookup with null path was accepted");
    f.nullKnownFolder = false;
    Require(SuggestedBackupPath().empty() && KnownFolderPath(FOLDERID_Desktop).empty(),
            "empty known folder path was accepted");
    f.folder = L"C:\\profile";
    Require(SuggestedBackupPath() == f.folder + L"\\" + kExportFileName,
            "profile suggestion did not add separator");
    Require(f.knownFolderFlags == KF_FLAG_DONT_VERIFY,
            "profile suggestion verified the folder on the UI thread");
    f.folder += L"\\";
    Require(SuggestedBackupPath() == f.folder + kExportFileName &&
            KnownFolderPath(FOLDERID_Profile) == L"C:\\profile",
            "profile suggestion duplicated separator or known path retained trailing separator");
    fixture::drives[L"Z:\\"] = DRIVE_FIXED;
    f.volume = true;
    Require(DriveMenuLabel(L"plain") == L"plain" && DriveMenuLabel(L"Z:\\") == L"Z: — Local disk",
            "invalid drive root or failed volume label fallback was incorrect");
    f.volumeResult = true;
    Require(DriveMenuLabel(L"Z:\\") == L"Z: — Local disk", "empty successful volume label omitted fallback");
    f.volumeName = L"Test volume";
    Require(DriveMenuLabel(L"Z:\\") == L"Z: — Test volume", "volume name was ignored");
    fixture::drives.clear();
    std::puts("PASS: encoding completion, environment size races, known-folder failures/null/empty values and volume labels");
}

inline std::atomic<unsigned> platformApartmentScans{0};
DWORD WINAPI PlatformFailedApartmentWorker(void*) {
    return RunWindowsRecentWorker([] {
        ++platformApartmentScans;
        return std::vector<TimedFolder>{{L"C:\\should-not-publish", 1}};
    });
}
void CheckWorkerPlatformFailures() {
    platform_failure::Scope isolated;
    StopWindowsRecentWorker();
    g_windowsRecentStopping = false;
    g_unloading = false;
    auto& f = platform_failure::faults;
    DWORD before = 0;
    DWORD after = 0;
    Require(GetProcessHandleCount(GetCurrentProcess(), &before), "worker handle count query failed");
    f.event = true;
    Require(!StartWindowsRecentWorkerLocked() && !g_windowsRecentWake && !g_windowsRecentThread,
            "failed worker event creation published handles");
    f.event = false;
    f.thread = true;
    Require(!StartWindowsRecentWorkerLocked() && !g_windowsRecentWake && !g_windowsRecentThread,
            "failed worker thread creation retained event handle");
    f.thread = false;
    Require(GetProcessHandleCount(GetCurrentProcess(), &after) && after == before,
            "worker creation failure leaked a native handle");
    f.wait = true;
    unsigned scans = 0;
    Require(RunWindowsRecentWorker([&] { ++scans; return std::vector<TimedFolder>{}; }) == 0 && !scans,
            "failed worker wait ran a scan or failed to exit");
    f.wait = false;
    f.coInitialize = true;
    platformApartmentScans = 0;
    g_windowsRecentsActive = true;
    g_windowsRecentsRequested = true;
    g_windowsRecentsScanning = false;
    g_windowsRecentsScanned = false;
    g_windowsRecents = {{L"C:\\old", 1}};
    g_windowsRecentWake = CreateEventW(nullptr, FALSE, TRUE, nullptr);
    g_windowsRecentThread = CreateThread(nullptr, 0, PlatformFailedApartmentWorker, nullptr, 0, nullptr);
    Require(g_windowsRecentWake && g_windowsRecentThread, "failed-apartment worker fixture creation failed");
    const auto deadline = GetTickCount64() + 5000;
    for (;;) {
        bool ready;
        { std::lock_guard lock(g_windowsRecentsMutex); ready = g_windowsRecentsScanned; }
        if (ready) break;
        Require(GetTickCount64() < deadline, "failed-apartment worker did not publish a safe result");
        Sleep(1);
    }
    StopWindowsRecentWorker();
    Require(platformApartmentScans == 0 && g_windowsRecents.empty() && !g_windowsRecentsScanning &&
            !g_windowsRecentThread && !g_windowsRecentWake,
            "failed COM apartment ran scanner, retained stale folders or leaked worker handles");
    g_windowsRecentsActive = false;
    g_windowsRecentStopping = false;
    std::puts("PASS: worker event/thread allocation cleanup, failed wait and COM apartment scan suppression");
}

namespace platform_failure {
struct IconBehavior {
    bool creation = false;
    HRESULT bitmap = S_OK;
    HRESULT size = S_OK;
    UINT width = 2;
    UINT height = 2;
    HRESULT converter = S_OK;
    HRESULT initialize = S_OK;
    HRESULT copy = S_OK;
    unsigned live = 0;
    unsigned copies = 0;
} inline icon;
struct IconBitmap : TestComObject<IWICBitmap> {
    IconBitmap() { ++icon.live; }
    ~IconBitmap() override { --icon.live; }
    HRESULT STDMETHODCALLTYPE GetSize(UINT* width, UINT* height) override { *width = icon.width; *height = icon.height; return icon.size; }
    HRESULT STDMETHODCALLTYPE GetPixelFormat(WICPixelFormatGUID*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetResolution(double*, double*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CopyPixels(const WICRect*, UINT, UINT, BYTE*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Lock(const WICRect*, DWORD, IWICBitmapLock**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetPalette(IWICPalette*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetResolution(double, double) override { return E_NOTIMPL; }
};
struct IconConverter : TestComObject<IWICFormatConverter> {
    IconConverter() { ++icon.live; }
    ~IconConverter() override { --icon.live; }
    HRESULT STDMETHODCALLTYPE GetSize(UINT*, UINT*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetPixelFormat(WICPixelFormatGUID*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetResolution(double*, double*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CopyPixels(const WICRect* rectangle, UINT stride, UINT count, BYTE* bytes) override {
        ++icon.copies;
        Require(!rectangle && stride == icon.width * 4 && count == icon.width * icon.height * 4,
                "folder icon requested incorrect BGRA buffer geometry");
        if (SUCCEEDED(icon.copy)) std::fill_n(bytes, count, 0x5a);
        return icon.copy;
    }
    HRESULT STDMETHODCALLTYPE Initialize(IWICBitmapSource* source, REFWICPixelFormatGUID format,
            WICBitmapDitherType dither, IWICPalette* palette, double threshold, WICBitmapPaletteType translation) override {
        Require(source && format == GUID_WICPixelFormat32bppBGRA && dither == WICBitmapDitherTypeNone &&
                !palette && threshold == 0 && translation == WICBitmapPaletteTypeCustom,
                "folder icon converter was initialized with wrong pixel format");
        return icon.initialize;
    }
    HRESULT STDMETHODCALLTYPE CanConvert(REFWICPixelFormatGUID, REFWICPixelFormatGUID, BOOL*) override { return E_NOTIMPL; }
};
struct IconFactory : TestComObject<IWICImagingFactory> {
    IconFactory() { ++icon.live; }
    ~IconFactory() override { --icon.live; }
    HRESULT STDMETHODCALLTYPE CreateBitmapFromHICON(HICON handle, IWICBitmap** result) override {
        Require(handle, "WIC factory received no owned Shell icon");
        *result = SUCCEEDED(icon.bitmap) ? new IconBitmap : nullptr;
        return icon.bitmap;
    }
    HRESULT STDMETHODCALLTYPE CreateFormatConverter(IWICFormatConverter** result) override {
        *result = SUCCEEDED(icon.converter) ? new IconConverter : nullptr;
        return icon.converter;
    }
    HRESULT STDMETHODCALLTYPE CreateDecoderFromFilename(LPCWSTR, const GUID*, DWORD, WICDecodeOptions, IWICBitmapDecoder**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateDecoderFromStream(IStream*, const GUID*, WICDecodeOptions, IWICBitmapDecoder**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateDecoderFromFileHandle(ULONG_PTR, const GUID*, WICDecodeOptions, IWICBitmapDecoder**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateComponentInfo(REFCLSID, IWICComponentInfo**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateDecoder(REFGUID, const GUID*, IWICBitmapDecoder**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateEncoder(REFGUID, const GUID*, IWICBitmapEncoder**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreatePalette(IWICPalette**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateBitmapScaler(IWICBitmapScaler**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateBitmapClipper(IWICBitmapClipper**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateBitmapFlipRotator(IWICBitmapFlipRotator**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateStream(IWICStream**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateColorContext(IWICColorContext**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateColorTransformer(IWICColorTransform**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateBitmap(UINT, UINT, REFWICPixelFormatGUID, WICBitmapCreateCacheOption, IWICBitmap**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateBitmapFromSource(IWICBitmapSource*, WICBitmapCreateCacheOption, IWICBitmap**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateBitmapFromSourceRect(IWICBitmapSource*, UINT, UINT, UINT, UINT, IWICBitmap**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateBitmapFromMemory(UINT, UINT, REFWICPixelFormatGUID, UINT, UINT, BYTE*, IWICBitmap**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateBitmapFromHBITMAP(HBITMAP, HPALETTE, WICBitmapAlphaChannelOption, IWICBitmap**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateComponentEnumerator(DWORD, DWORD, IEnumUnknown**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateFastMetadataEncoderFromDecoder(IWICBitmapDecoder*, IWICFastMetadataEncoder**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateFastMetadataEncoderFromFrameDecode(IWICBitmapFrameDecode*, IWICFastMetadataEncoder**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateQueryWriter(REFGUID, const GUID*, IWICMetadataQueryWriter**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE CreateQueryWriterFromReader(IWICMetadataQueryReader*, const GUID*, IWICMetadataQueryWriter**) override { return E_NOTIMPL; }
};
inline HRESULT IconCreation(REFCLSID clsid, LPUNKNOWN outer, DWORD context, REFIID iid, LPVOID* out) {
    if (clsid != CLSID_WICImagingFactory) return realComCreation(clsid, outer, context, iid, out);
    *out = nullptr;
    if (icon.creation) return E_NOINTERFACE;
    auto factory = NewFake<IconFactory>();
    return factory->QueryInterface(iid, out);
}
}

void CheckIconPlatformFailures() {
    platform_failure::Scope isolated;
    const auto previous = fixture::creation;
    fixture::creation = platform_failure::IconCreation;
    auto& f = platform_failure::faults;
    f.shellIcon = platform_failure::ShellIcon::Fail;
    Require(!FolderIconPixels(L"test-icon"), "failed Shell icon query accepted");
    f.shellIcon = platform_failure::ShellIcon::NoIcon;
    Require(!FolderIconPixels(L"test-icon") && f.destroyedIcons == 0,
            "Shell icon query without handle accepted or destroyed null icon");
    f.shellIcon = platform_failure::ShellIcon::OwnedIcon;
    for (unsigned stage = 0; stage < 10; ++stage) {
        auto& icon = platform_failure::icon;
        icon = {};
        if (stage == 0) icon.creation = true;
        if (stage == 1) icon.bitmap = E_FAIL;
        if (stage == 2) icon.size = E_FAIL;
        if (stage == 3) icon.width = 0;
        if (stage == 4) icon.height = 0;
        if (stage == 5) icon.width = 257;
        if (stage == 6) icon.height = 257;
        if (stage == 7) icon.converter = E_FAIL;
        if (stage == 8) icon.initialize = E_FAIL;
        if (stage == 9) icon.copy = E_FAIL;
        const auto destroyed = f.destroyedIcons;
        Require(!FolderIconPixels(L"test-icon") && icon.live == 0 && f.destroyedIcons == destroyed + 1,
                "failed WIC icon stage accepted pixels or leaked icon/COM ownership");
        Require(icon.copies == (stage == 9 ? 1u : 0u), "failed WIC stage copied pixels prematurely");
    }
    platform_failure::icon = {};
    const auto pixels = FolderIconPixels(L"test-icon");
    Require(pixels && pixels->width == 2 && pixels->height == 2 &&
            pixels->bytes == std::vector<BYTE>(16, 0x5a) && platform_failure::icon.live == 0,
            "WIC icon did not recover with exact BGRA pixels and released ownership");
    fixture::creation = previous;
    std::puts("PASS: Shell/WIC icon failures, dimension bounds, exact BGRA conversion and icon/COM cleanup");
}

namespace platform_failure {
struct LinkBehavior {
    std::wstring path = L"C:\\valid";
    DWORD attributes = FILE_ATTRIBUTE_DIRECTORY;
    HRESULT load = S_OK;
    HRESULT getPath = S_OK;
};
inline std::map<std::wstring, LinkBehavior> links;
inline bool linkCreationFailure = false;
inline bool persistInterface = true;
inline unsigned linkLoads = 0;
inline unsigned linkLive = 0;
struct RecentLink : IShellLinkW, IPersistFile {
    ULONG refs = 1;
    LinkBehavior behavior;
    RecentLink() { ++linkLive; }
    virtual ~RecentLink() { --linkLive; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == IID_IShellLinkW) *out = static_cast<IShellLinkW*>(this);
        else if (persistInterface && (iid == IID_IPersist || iid == IID_IPersistFile)) *out = static_cast<IPersistFile*>(this);
        else return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { const auto count = --refs; if (!count) delete this; return count; }
    HRESULT STDMETHODCALLTYPE Load(LPCOLESTR path, DWORD mode) override {
        ++linkLoads;
        Require(mode == STGM_READ, "Recent shortcut scanner requested writable access");
        std::wstring file(path);
        const auto found = links.find(file.substr(file.find_last_of(L'\\') + 1));
        if (found == links.end()) return E_FAIL;
        behavior = found->second;
        return behavior.load;
    }
    HRESULT STDMETHODCALLTYPE GetPath(LPWSTR out, int count, WIN32_FIND_DATAW* data, DWORD flags) override {
        Require(flags == 0 && count > 0 && data, "Recent shortcut path query changed flags or omitted attributes");
        const size_t length = std::min<size_t>(behavior.path.size(), count - 1);
        std::copy_n(behavior.path.begin(), length, out);
        out[length] = 0;
        data->dwFileAttributes = behavior.attributes;
        return behavior.getPath;
    }
    HRESULT STDMETHODCALLTYPE GetIDList(PIDLIST_ABSOLUTE*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetIDList(PCIDLIST_ABSOLUTE) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetDescription(LPWSTR, int) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetDescription(LPCWSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetWorkingDirectory(LPWSTR, int) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetWorkingDirectory(LPCWSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetArguments(LPWSTR, int) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetArguments(LPCWSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetHotkey(WORD*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetHotkey(WORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetShowCmd(int*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetShowCmd(int) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetIconLocation(LPWSTR, int, int*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetIconLocation(LPCWSTR, int) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetRelativePath(LPCWSTR, DWORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Resolve(HWND, DWORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetPath(LPCWSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE IsDirty() override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Save(LPCOLESTR, BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SaveCompleted(LPCOLESTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR*) override { return E_NOTIMPL; }
};
inline HRESULT RecentCreation(REFCLSID clsid, LPUNKNOWN outer, DWORD context, REFIID iid, LPVOID* out) {
    if (clsid != CLSID_ShellLink) return realComCreation(clsid, outer, context, iid, out);
    *out = nullptr;
    if (linkCreationFailure) return E_FAIL;
    auto link = NewFake<RecentLink>();
    return link->QueryInterface(iid, out);
}
inline void AddLink(unsigned index, LinkBehavior behavior, DWORD attributes = FILE_ATTRIBUTE_NORMAL) {
    const auto name = L"fake" + std::to_wstring(index) + L".lnk";
    WIN32_FIND_DATAW entry{};
    std::copy(name.begin(), name.end(), entry.cFileName);
    entry.dwFileAttributes = attributes;
    entry.ftLastWriteTime = {index + 1, 0};
    faults.entries.push_back(entry);
    links[name] = std::move(behavior);
}
}

void CheckRecentScannerPlatformFailures() {
    fixture::Reset();
    BackupFixture directory;
    platform_failure::Scope isolated;
    auto& f = platform_failure::faults;
    fixture::recentDirectory = directory.directory;
    fixture::creation = platform_failure::RecentCreation;
    f.enumerate = true;
    g_windowsRecentStopping = false;
    Require(ScanWindowsRecentFolders().empty() && f.findCloseCalls == 0,
            "missing shortcut enumeration returned entries or closed invalid find handle");
    platform_failure::AddLink(0, {});
    platform_failure::linkCreationFailure = true;
    Require(ScanWindowsRecentFolders().empty() && platform_failure::linkLive == 0,
            "failed shortcut COM creation produced folders");
    platform_failure::linkCreationFailure = false;
    platform_failure::persistInterface = false;
    Require(ScanWindowsRecentFolders().empty() && platform_failure::linkLoads == 0 && platform_failure::linkLive == 0,
            "shortcut without persist interface was loaded or leaked");
    platform_failure::persistInterface = true;
    for (unsigned stage = 0; stage < 5; ++stage) {
        auto& behavior = platform_failure::links[L"fake0.lnk"];
        behavior = {};
        if (stage == 0) behavior.load = E_FAIL;
        if (stage == 1) behavior.getPath = E_FAIL;
        if (stage == 2) behavior.getPath = S_FALSE;
        if (stage == 3) behavior.path.clear();
        if (stage == 4) behavior.path = L"relative";
        Require(ScanWindowsRecentFolders().empty() && platform_failure::linkLive == 0,
                "invalid shortcut load/result/empty/relative target was accepted or leaked");
    }
    f.entries.clear();
    platform_failure::links.clear();
    platform_failure::AddLink(0, {L"C:\\skip-directory"}, FILE_ATTRIBUTE_DIRECTORY);
    platform_failure::AddLink(1, {L"C:\\file", FILE_ATTRIBUTE_NORMAL});
    platform_failure::AddLink(2, {L"C:\\good", FILE_ATTRIBUTE_DIRECTORY});
    platform_failure::AddLink(3, {L"c:\\GOOD\\", FILE_ATTRIBUTE_DIRECTORY});
    platform_failure::AddLink(4, {L"C:\\failed-load", FILE_ATTRIBUTE_DIRECTORY, E_FAIL});
    platform_failure::AddLink(5, {directory.directory, 0});
    platform_failure::AddLink(6, {directory.directory + L"\\absent", 0});
    const auto scanned = ScanWindowsRecentFolders();
    Require(scanned.size() == 2 && scanned[0].path == directory.directory &&
            scanned[1].path == L"c:\\GOOD" && scanned[1].usedAt == 4 && platform_failure::linkLive == 0,
            "shortcut failures stopped later valid entries, directory entries/files leaked, or duplicate/timestamp handling failed");
    f.entries.clear();
    platform_failure::links.clear();
    platform_failure::linkLoads = 0;
    for (unsigned i = 0; i < 4097; ++i) platform_failure::AddLink(i, {L"C:\\file", FILE_ATTRIBUTE_NORMAL});
    const auto enumerated = f.findNextCalls;
    Require(ScanWindowsRecentFolders().empty() && f.findNextCalls - enumerated == 4095 &&
            platform_failure::linkLoads == kMaxRecentLinksScanned && platform_failure::linkLive == 0,
            "scanner exceeded enumeration or shortcut-processing safety cap");
    f.entries.clear();
    platform_failure::links.clear();
    platform_failure::linkLoads = 0;
    for (unsigned i = 0; i < kMaxWindowsRecentFolders + 1; ++i) {
        platform_failure::AddLink(i, {L"C:\\folder" + std::to_wstring(i), FILE_ATTRIBUTE_DIRECTORY});
    }
    Require(ScanWindowsRecentFolders().size() == kMaxWindowsRecentFolders &&
            platform_failure::linkLoads == kMaxWindowsRecentFolders && platform_failure::linkLive == 0,
            "scanner exceeded folder collection limit");
    platform_failure::links.clear();
    fixture::Reset();
    std::puts("PASS: read-only Recent enumeration/COM/load/path failures, target filtering, cleanup and scanner caps");
}

void CheckPlatformFailures() {
    CheckBackupPlatformFailures();
    CheckEncodingAndKnownFolderFailures();
    CheckWorkerPlatformFailures();
    CheckIconPlatformFailures();
    CheckRecentScannerPlatformFailures();
}
