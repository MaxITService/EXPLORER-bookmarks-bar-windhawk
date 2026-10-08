// Standalone checks of the real storage, settings and native frame lifetime code.
// WH_EDITING stubs Windhawk; this executable never installs hooks in Explorer.
#include "allocation-fixture.h"
#include "windhawk-fixture.h"
#include "platform-failure-fixture.h"
#include "runtime-gap-fixture.h"
#define BOOKMARK_EVENT_PLATFORM_ONLY
#include "event-gap-fixture.h"
#undef BOOKMARK_EVENT_PLATFORM_ONLY
#include <explorer-folder-bookmarks-bar.wh.cpp>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <werapi.h>

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

HWND CreateTestFrame() {
    HWND window = CreateWindowExW(0, L"STATIC", L"Bookmarks test", WS_OVERLAPPED,
                                  0, 0, 32, 32, nullptr, nullptr,
                                  GetModuleHandleW(nullptr), nullptr);
    Require(window && TrackFrame(window), "native frame tracking failed");
    return window;
}

unsigned unadvised = 0;
class TestConnectionPoint final : public IConnectionPoint {
    ULONG refs = 1;
    DWORD owner = GetCurrentThreadId();
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override {
        if (iid != IID_IUnknown && iid != IID_IConnectionPoint) {
            *result = nullptr;
            return E_NOINTERFACE;
        }
        *result = static_cast<IConnectionPoint*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = --refs;
        if (!remaining) {
            Require(GetCurrentThreadId() == owner, "COM connection released on wrong thread");
            delete this;
        }
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE GetConnectionInterface(IID*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetConnectionPointContainer(IConnectionPointContainer**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Advise(IUnknown*, DWORD*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Unadvise(DWORD) override {
        Require(GetCurrentThreadId() == owner, "COM connection unadvised on wrong thread");
        ++unadvised;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE EnumConnections(IEnumConnections**) override { return E_NOTIMPL; }
};

void ConsumeRefresh(HWND window) {
    MSG message;
    Require(PeekMessageW(&message, window, RefreshBarsMessage(),
                        RefreshBarsMessage(), PM_REMOVE), "window missed broadcast");
    DispatchMessageW(&message);
    Require(g_refreshPending, "broadcast did not queue UI refresh");
    StopThreadWork();
}

void CheckFramesAndSettings() {
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings.buttons = 1;
        g_sharedBookmarkHotkey = false;
        ++g_settingsGeneration;
    }
    UpdateThreadSettings();
    HWND first = CreateTestFrame();
    HWND second = CreateTestFrame();
    Require(TrackFrame(first) && g_frames.size() == 2,
            "repeated frame tracking duplicated lifetime state");
    Require(!TrackFrame(nullptr), "null frame accepted");
    g_bars->emplace_back().frame = first;
    g_bars->emplace_back().frame = second;
    HANDLE ready = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE consume = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE finished = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    std::thread other([&] {
        Require(!TrackFrame(first), "foreign-thread frame accepted");
        UpdateThreadSettings();
        Require(g_recentSettings.buttons == 1 && !g_bookmarkHotkey,
                "new UI thread missed initial settings");
        HWND third = CreateTestFrame();
        SetEvent(ready);
        Require(WaitForSingleObject(consume, 5000) == WAIT_OBJECT_0, "frame wait timed out");
        Require(g_recentSettings.buttons == 1 && !g_bookmarkHotkey,
                "UI snapshot mutated from another thread");
        ConsumeRefresh(third);
        Require(UpdateThreadSettings() && g_recentSettings.buttons == 3 && g_bookmarkHotkey,
                "UI thread failed to apply new settings snapshot");
        DestroyWindow(third);
        Require(g_frames.empty() && g_bars->empty(), "last frame left UI state behind");
        SetEvent(finished);
    });
    Require(WaitForSingleObject(ready, 5000) == WAIT_OBJECT_0, "other UI thread failed to start");
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings.buttons = 3;
        g_sharedBookmarkHotkey = true;
        ++g_settingsGeneration;
    }
    NotifyExplorerBars();
    ConsumeRefresh(first);
    ConsumeRefresh(second);
    SetEvent(consume);
    Require(WaitForSingleObject(finished, 5000) == WAIT_OBJECT_0, "other frame missed update");
    other.join();
    g_recentTracker = new RecentTracker();
    g_recentTracker->sink = new ShellEventSink();
    g_recentTracker->windowsPoint.attach(new TestConnectionPoint());
    g_recentTracker->windowsCookie = 1;
    BrowserConnection connection;
    connection.point.attach(new TestConnectionPoint());
    connection.cookie = 2;
    g_recentTracker->browsers.push_back(std::move(connection));
    {
        UiCallbackScope outerCallback;
        DestroyWindow(first);
        Require(g_bars->size() == 2 && g_recentTracker,
                "nested close destroyed state while callback still used it");
    }
    Require(g_frames.size() == 1 && GetPropW(second, kFrameProperty),
            "closing one frame removed the surviving frame");
    Require(g_bars->size() == 1 && g_bars->front().frame == second,
            "closing one frame removed another frame's bar");
    DestroyWindow(second);
    Require(g_frames.empty() && g_bars->empty() && !g_workTimer,
            "last frame left callbacks or state behind");
    Require(!g_recentTracker && unadvised == 2,
            "last frame left navigation or ShellWindows subscription connected");
    // A later window on the same thread must remain usable.
    HWND reopened = CreateTestFrame();
    Require(g_frames.size() == 1, "reopening a window failed after last-frame cleanup");
    DestroyWindow(reopened);
    CloseHandle(ready);
    CloseHandle(consume);
    CloseHandle(finished);
    std::puts("PASS: broadcast across UI threads, settings snapshots, nested close, COM teardown and reopen");
}

void CheckStorageMutex() {
    const auto name = L"Local\\BookmarksStorageTest_" + std::to_wstring(GetCurrentProcessId());
    StorageMutex first;
    StorageMutex second;
    first.processMutex = CreateMutexW(nullptr, FALSE, name.c_str());
    second.processMutex = CreateMutexW(nullptr, FALSE, name.c_str());
    Require(first.processMutex && second.processMutex, "named storage mutex creation failed");
    unsigned value = 0;
    auto increment = [&](StorageMutex& mutex) {
        for (unsigned i = 0; i < 1000; ++i) {
            std::lock_guard lock(mutex);
            const unsigned old = value;
            SwitchToThread();
            value = old + 1;
        }
    };
    std::thread writer([&] { increment(second); });
    increment(first);
    writer.join();
    Require(value == 2000, "independent storage locks allowed lost updates");
    CloseHandle(first.processMutex);
    CloseHandle(second.processMutex);
    std::puts("PASS: independent named storage mutex handles serialize writers");
}

void CheckBookmarkBoundaries() {
    Require(ValidateImportedFolders({}), "empty backup must allow clearing bookmarks");
    Require(ValidateImportedFolders({L"C:\\", L"\\\\server\\share", L"D:\\folder"}),
            "valid local and UNC backup rejected");
    for (const auto& path : std::vector<std::wstring>{L"relative", L"C:relative",
             L"C:\\folder\\", L"C:\\bad\nname", std::wstring(L"C:\\bad\0tail", 11)}) {
        Require(!ValidateImportedFolders({path}), "unsafe or unnormalized backup accepted");
    }
    Require(!ValidateImportedFolders({L"C:\\ПАПКА", L"c:\\папка"}),
            "Unicode case duplicate accepted");
    std::vector<std::wstring> folders;
    for (size_t i = 0; i < kMaxBookmarks - 1; ++i) {
        folders.push_back(L"C:\\folder" + std::to_wstring(i));
    }
    Require(ValidateImportedFolders(folders), "valid near-capacity backup rejected");
    auto change = InsertMissingLocked(folders,
        {L"C:\\inserted\\", L"c:\\INSERTED", L"relative", L"C:\\overflow"}, 1);
    Require(change.added == 1 && change.existing == 1 && change.invalid == 1 &&
            change.noRoom == 1 && folders.size() == kMaxBookmarks &&
            folders[1] == L"C:\\inserted" && folders[2] == L"C:\\folder1",
            "mixed insertion lost ordering, normalization or result counts");
    Require(change.Notice() == BookmarkNotice::Partial, "partial add notice missing");
    auto notice = DecodeNotice(nullptr, EncodeNotice(change));
    Require(notice.added == 1 && notice.total == 2 &&
            NoticeText(notice) == L"Added 1 of 2: bookmarks full (32)",
            "drop notification round trip lost partial result");
    change = InsertMissingLocked(folders, {L"C:\\folder0"}, 999);
    Require(change.existing == 1 && change.noRoom == 0 &&
            change.Notice() == BookmarkNotice::AlreadyBookmarked,
            "duplicate at capacity incorrectly reports full");
    change = InsertMissingLocked(folders, {L"C:\\extra"}, 999);
    Require(change.noRoom == 1 && change.Notice() == BookmarkNotice::Full,
            "full list accepted extra bookmark");
    folders.push_back(L"C:\\extra");
    Require(!ValidateImportedFolders(folders), "over-capacity backup accepted");
    Require(!ValidateImportedFolders({L"C:\\" + std::wstring(kMaxStorageChars, L'x')}),
            "oversized storage backup accepted");
    folders.clear();
    change = InsertMissingLocked(folders, {L"C:\\one", L"C:\\two"}, 999);
    Require(change.added == 2 && folders == std::vector<std::wstring>{L"C:\\one", L"C:\\two"},
            "out-of-range insertion did not append in input order");
    std::puts("PASS: backup validation, Unicode duplicates, insertion order, capacity and notices");
}

void CheckEncodingStorageAndLayout() {
    const std::wstring original = L"C:\\Папка\\日本語\\\U0001F4C1";
    std::string utf8;
    std::wstring wide;
    Require(WideToUtf8(original, utf8) && Utf8ToWide(utf8, wide) && wide == original,
            "Unicode backup encoding did not round trip");
    for (const auto& invalid : std::vector<std::string>{std::string("\xC0\xAF", 2),
             std::string("\xE2\x82", 2), std::string("\xED\xA0\x80", 3)}) {
        Require(!Utf8ToWide(invalid, wide), "malformed UTF-8 accepted");
    }
    Require(!WideToUtf8(std::wstring(1, static_cast<wchar_t>(0xD800)), utf8),
            "unpaired UTF-16 surrogate accepted");
    Require(NormalizePath(L"C:\\") == L"C:\\" &&
            NormalizePath(L"C:\\folder\\//") == L"C:\\folder",
            "normalization damaged drive root or retained trailing separators");
    const std::vector<std::wstring> paths{L"C:\\one", L"D:\\два"};
    Require(SplitBookmarks(JoinLines(paths)) == paths,
            "bookmark storage did not round trip");
    Require(SplitPaths(L"\nC:\\one\\\n\nD:\\two\nE:\\three", 2) ==
            std::vector<std::wstring>{L"C:\\one", L"D:\\two"},
            "storage empty lines, normalization or limit failed");
    Require(CheckFolderStatus(L"\\\\unreachable.invalid\\share") == FolderStatus::Unknown,
            "UNC path was classified as missing");
    Require(WrappedRows({}, 100) == 1 && WrappedRows({50, 50}, 100) == 1 &&
            WrappedRows({50, 50.5}, 100) == 1 &&
            WrappedRows({50, 50.6}, 100) == 2 &&
            WrappedRows({150, 10}, 100) == 2,
            "row wrapping broke exact fit, tolerance or oversized button handling");
    for (unsigned rows = 1; rows <= 4; ++rows) {
        Require(HostExtraAt96Dpi(rows) == static_cast<int>(40 * rows + 4),
                "confirmed adaptive host geometry changed");
    }
    Require(DecodeNotice(nullptr, 255).kind == BookmarkNotice::None,
            "invalid notification kind accepted");
    BookmarkChange huge;
    huge.added = 300;
    huge.noRoom = 300;
    const auto saturated = DecodeNotice(nullptr, EncodeNotice(huge));
    Require(saturated.added == 255 && saturated.total == 255,
            "notification counts overflowed wire fields");
    std::puts("PASS: strict Unicode encoding, storage round trips, wrapping geometry and notice saturation");
}

void CheckInsertionMatrix() {
    for (size_t count = 0; count <= kMaxBookmarks; ++count) {
        for (size_t index = 0; index <= count + 1; ++index) {
            std::vector<std::wstring> original;
            for (size_t i = 0; i < count; ++i) {
                original.push_back(L"C:\\old" + std::to_wstring(i));
            }
            auto actual = original;
            const std::vector<std::wstring> incoming{L"D:\\first", L"D:\\second", L"D:\\third"};
            const auto result = InsertMissingLocked(actual, incoming, index);
            const size_t accepted = std::min<size_t>(incoming.size(), kMaxBookmarks - count);
            auto expected = original;
            expected.insert(expected.begin() + std::min(index, count),
                            incoming.begin(), incoming.begin() + accepted);
            Require(actual == expected && result.added == accepted &&
                    result.noRoom == incoming.size() - accepted &&
                    result.invalid == 0 && result.existing == 0 &&
                    ValidateImportedFolders(actual),
                    "insertion matrix violated capacity, ordering or backup validity");
            const auto repeated = InsertMissingLocked(actual, incoming, index);
            Require(repeated.added == 0 && repeated.existing == accepted &&
                    repeated.noRoom == incoming.size() - accepted && actual == expected,
                    "repeating a batch changed storage or misclassified duplicates");
        }
    }
    std::puts("PASS: insertion/retry matrix for every capacity and insertion position");
}

void CheckStoredMutations() {
    fixture::Reset();
    Require(ReadStorageLocked().empty(), "missing storage is not empty");
    auto added = AddBookmark(L"C:\\one\\");
    Require(added.changed && added.added == 1 && ReadStorageLocked() == L"C:\\one",
            "add did not persist normalized path");
    Require(AddBookmark(L"c:\\ONE").existing == 1, "stored duplicate added");
    Require(!AddBookmark(L"").changed && !AddBookmark(L"C:\\bad\nname").changed,
            "invalid add mutated storage");
    Require(InsertBookmarks({L"D:\\two", L"E:\\three"}, 0).added == 2,
            "batch did not persist");
    Require(MoveBookmarkToIndex(L"c:\\ONE", 0) &&
            SplitBookmarks(ReadStorageLocked()) ==
                std::vector<std::wstring>{L"C:\\one", L"D:\\two", L"E:\\three"},
            "move to beginning failed");
    Require(MoveBookmarkToIndex(L"C:\\one", 3) &&
            SplitBookmarks(ReadStorageLocked()).back() == L"C:\\one",
            "move to end failed");
    Require(!MoveBookmarkToIndex(L"missing", 0) && !MoveBookmarkToIndex(L"C:\\one", 9) &&
            !MoveBookmarkToIndex(L"C:\\one", 3), "invalid/no-op move wrote storage");
    Require(ToggleBookmarks({L"C:\\one", L"D:\\two"}).changed &&
            ReadStorageLocked() == L"E:\\three", "toggle existing batch did not remove all");
    Require(ToggleBookmarks({L"E:\\three", L"F:\\four"}).added == 1,
            "mixed toggle removed existing entry instead of adding missing");
    Require(!ToggleBookmarks({L""}).changed, "empty toggle changed storage");
    Require(RemoveBookmark(L"e:\\THREE") && !RemoveBookmark(L"E:\\three"),
            "remove failed or missing removal changed storage");
    const auto before = fixture::storage;
    fixture::failWrites = true;
    Require(!AddBookmark(L"C:\\failed").changed &&
            InsertBookmarkAt(L"C:\\failed", 0).added == 0 &&
            !RemoveBookmark(L"F:\\four") && !ToggleBookmarks({L"F:\\four"}).changed &&
            fixture::storage == before, "failed storage write changed persisted data");
    fixture::failWrites = false;
    Require(!SaveBookmarksLocked({std::wstring(kMaxStorageChars + 1, L'x')}),
            "oversized bookmarks saved");
    Require(SaveLinesLocked(L"test", {std::wstring(kMaxStorageChars, L'x'), L"older"}) &&
            fixture::storage[L"test"].size() == kMaxStorageChars,
            "oldest history did not trim to storage limit");
    Require(SaveLinesLocked(L"test", {std::wstring(kMaxStorageChars + 1, L'x')}) &&
            fixture::storage[L"test"].empty(), "oversized single history item not dropped");
    fixture::storage[kBookmarksKey] = std::wstring(kMaxStorageChars + 1, L'x');
    Require(ReadStorageLocked().empty(), "oversized read accepted");
    fixture::Reset();
    std::puts("PASS: persisted bookmark operations, no-op moves, failed writes and storage truncation");
}

void CheckSettingsAndRecents() {
    fixture::Reset();
    fixture::integers[L"recent.enabled"] = 1;
    fixture::integers[L"recent.buttons"] = 99;
    fixture::integers[L"recent.history"] = -1;
    fixture::integers[L"bookmarkHotkey"] = 1;
    fixture::strings[L"recent.rcButton"] = L"clear";
    fixture::strings[L"fxCustomFolders[0].path"] = L" C:\\first\\ ";
    fixture::strings[L"fxCustomFolders[1].path"] = L"c:\\FIRST";
    fixture::strings[L"fxCustomFolders[2].path"] = L"relative";
    fixture::strings[L"fxCustomFolders[3].path"] = L"D:\\valid";
    fixture::strings[L"fxCustomFolders[3].label"] = std::wstring(81, L'x');
    fixture::strings[L"fxCustomFolders[4].path"] = L"E:\\valid";
    fixture::strings[L"fxCustomFolders[4].label"] = L"bad\nlabel";
    fixture::strings[L"fxCustomFolders[5].path"] = L"F:\\valid";
    fixture::strings[L"fxCustomFolders[5].label"] = L" Friendly ";
    LoadSettings();
    Require(UpdateThreadSettings() && g_recentSettings.enabled && g_recentSettings.buttons == 3 &&
            g_recentSettings.history == 3 && g_recentSettings.menuButton &&
            g_recentSettings.rightClickClears && g_bookmarkHotkey,
            "settings clamp or clear RC mode failed");
    Require(g_fxFolders.size() == 2 && g_fxFolders[0].label == L"first" &&
            g_fxFolders[1].label == L"Friendly", "FX validation or fallback labels failed");
    Require(!UpdateThreadSettings(), "unchanged settings generation reapplied");
    for (const auto& mode : {L"hidden", L"unknown", L"menu"}) {
        fixture::strings[L"recent.rcButton"] = mode;
        fixture::integers[L"recent.buttons"] = -1;
        fixture::integers[L"recent.history"] = 99;
        LoadSettings();
        Require(UpdateThreadSettings() && g_recentSettings.buttons == 1 &&
                g_recentSettings.history == 10 && !g_recentSettings.rightClickClears &&
                g_recentSettings.menuButton == (std::wstring_view(mode) != L"hidden"),
                "RC fallback or opposite settings clamp failed");
    }
    Require(TrimSetting(L" \t\r\n").empty() && ButtonLabel(L"C:\\") == L"C:\\" &&
            ButtonLabel(L"plain") == L"plain" && ButtonLabel(L"C:/folder") == L"folder",
            "label helpers failed");
    Require(ExpandFxPath(L"").empty() && ExpandFxPath(L"relative").empty() &&
            ExpandFxPath(std::wstring(4097, L'x')).empty(), "invalid FX expansion accepted");
    Require(SetEnvironmentVariableW(L"BOOKMARKS_TEST_PATH", L"C:\\expanded"),
            "test environment setup failed");
    Require(ExpandFxPath(L"%BOOKMARKS_TEST_PATH%\\") == L"C:\\expanded",
            "FX environment path did not expand");
    SetEnvironmentVariableW(L"BOOKMARKS_TEST_PATH", nullptr);
    Require(!RecordExplorerRecent(L"relative"), "invalid recent path recorded");
    for (size_t i = 0; i < kMaxStoredRecents + 2; ++i) {
        Require(RecordExplorerRecent(L"\\\\test.invalid\\share\\" + std::to_wstring(i)),
                "recent folder not recorded");
    }
    auto recent = SplitPaths(fixture::storage[kRecentsKey], kMaxStoredRecents + 2);
    Require(recent.size() == kMaxStoredRecents &&
            !RecordExplorerRecent(recent.front()) && RecordExplorerRecent(recent.back()),
            "recent cap, same-front no-op or promotion failed");
    recent = SplitPaths(fixture::storage[kRecentsKey], kMaxStoredRecents);
    g_recentSettings.history = 2;
    const auto view = LoadRecentView({recent.front()});
    Require(view.folders.size() == 2 &&
            view.folders[0] == recent[1], "recent view did not filter bookmarks or apply history limit");
    RemoveRecent(recent.front());
    Require(SplitPaths(fixture::storage[kRecentsKey], kMaxStoredRecents).size() == kMaxStoredRecents - 1,
            "recent removal failed");
    ClearRecents();
    Require(fixture::storage[kRecentsKey].empty(), "explorer history not cleared");
    fixture::Reset();
    std::puts("PASS: settings clamps/RC modes, FX validation and recent history");
}

struct BackupFixture {
    std::wstring directory;
    std::wstring file;
    BackupFixture() {
        wchar_t temp[MAX_PATH]{};
        wchar_t unique[MAX_PATH]{};
        Require(GetTempPathW(MAX_PATH, temp) > 0 &&
                GetTempFileNameW(temp, L"bmt", 0, unique), "backup fixture temp path failed");
        Require(DeleteFileW(unique) && CreateDirectoryW(unique, nullptr),
                "backup fixture directory creation failed");
        directory = unique;
        file = directory + L"\\backup.json";
    }
    ~BackupFixture() {
        DeleteFileW(file.c_str());
        Require(RemoveDirectoryW(directory.c_str()), "backup fixture leaked files");
    }
    void Write(const std::string& bytes) {
        ScopedFile output(CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        DWORD written = 0;
        Require(output.value != INVALID_HANDLE_VALUE &&
                WriteFile(output.value, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) &&
                written == bytes.size(), "backup fixture write failed");
    }
};

void CheckBackupFiles() {
    fixture::Reset();
    BackupFixture backup;
    Require(!LoadBackup(L"") && !SaveBackup(L"") && !LoadBackup(backup.file),
            "empty or absent backup succeeded");
    const std::vector<std::wstring> expected{L"C:\\Папка", L"\\\\test.invalid\\日本語"};
    Require(SaveBookmarksLocked(expected) && SaveBackup(backup.file),
            "Unicode backup save failed");
    fixture::storage[kBookmarksKey] = L"C:\\sentinel";
    Require(LoadBackup(backup.file) && SplitBookmarks(ReadStorageLocked()) == expected,
            "real JSON file round trip failed");
    Require(SaveBackup(backup.file), "atomic replacement of existing backup failed");
    const std::string prefix = "{\"format\":\"explorer-folder-bookmarks-bar\",\"version\":1,\"folders\":";
    const std::string valid = prefix + "[\"C:\\\\valid\"]}";
    backup.Write(std::string("\xEF\xBB\xBF", 3) + valid);
    Require(LoadBackup(backup.file) && ReadStorageLocked() == L"C:\\valid",
            "UTF-8 BOM backup failed");
    const auto before = fixture::storage;
    const std::vector<std::string> invalid{
        "", "{", "[]", "{}", std::string("\xFF", 1),
        "{\"format\":\"wrong\",\"version\":1,\"folders\":[]}",
        "{\"format\":\"explorer-folder-bookmarks-bar\",\"version\":2,\"folders\":[]}",
        "{\"format\":\"explorer-folder-bookmarks-bar\",\"version\":\"1\",\"folders\":[]}",
        prefix + "[42]}", prefix + "[\"relative\"]}",
        prefix + "[\"C:\\\\valid\\u0000tail\"]}",
        prefix + "[\"C:\\\\valid\",\"c:\\\\VALID\"]}",
        prefix + "[\"C:\\\\trailing\\\\\"]}",
        std::string(kMaxImportBytes + 1, 'x')
    };
    for (const auto& bytes : invalid) {
        backup.Write(bytes);
        Require(!LoadBackup(backup.file) && fixture::storage == before,
                "invalid JSON backup changed persisted bookmarks");
    }
    std::string tooMany = prefix + "[";
    for (size_t i = 0; i <= kMaxBookmarks; ++i) {
        if (i) tooMany += ',';
        tooMany += "\"C:\\\\folder" + std::to_string(i) + "\"";
    }
    backup.Write(tooMany + "]}");
    Require(!LoadBackup(backup.file), "over-capacity JSON array accepted");
    backup.Write(valid);
    fixture::failWrites = true;
    Require(!LoadBackup(backup.file) && fixture::storage == before,
            "failed import commit mutated storage");
    fixture::failWrites = false;
    backup.Write(prefix + "[]}");
    Require(LoadBackup(backup.file) && ReadStorageLocked().empty(),
            "empty JSON array did not clear bookmarks");
    Require(!SaveBackup(backup.directory + L"\\absent\\backup.json"),
            "save to absent directory succeeded");
    Require(!SaveBackup(backup.directory), "save over directory succeeded");
    fixture::Reset();
    std::puts("PASS: real JSON backup files, Unicode/BOM, atomic replacement and rejection without mutation");
}

void CheckCallbacksAndDeferredWork() {
    std::vector<int> order;
    std::vector<std::function<void()>> handlers;
    handlers.emplace_back([&] { order.push_back(1); });
    handlers.emplace_back([&] { order.push_back(2); throw winrt::hresult_error(E_FAIL); });
    handlers.emplace_back([&] { order.push_back(3); });
    RevokeHandlers(handlers);
    Require(handlers.empty() && order == std::vector<int>{3, 2, 1},
            "handler revocation stopped after exception or broke reverse order");
    RevokeHandlers(handlers);
    unsigned called = 0;
    auto guarded = GuardHandler(L"test", [&](unsigned amount) {
        called += amount;
        Require(g_uiCallbackDepth > 0, "guard did not enter callback scope");
        throw winrt::hresult_error(E_FAIL);
    });
    guarded(2);
    Require(called == 2 && g_uiCallbackDepth == 0, "guard leaked depth or failed to invoke handler");
    Require(g_activeOperations == 0, "operation state contaminated before test");
    {
        OperationScope outer;
        OperationScope inner;
        Require(outer.active && inner.active && g_activeOperations == 2,
                "nested operations were not counted");
    }
    Require(g_activeOperations == 0 && g_uiCallbackDepth == 0,
            "operation scope leaked count or callback depth");
    g_unloading = true;
    { OperationScope blocked; Require(!blocked.active && g_activeOperations == 0,
                                     "operation admitted during unload"); }
    ScheduleThreadWork();
    Require(!g_workTimer, "work scheduled during unload");
    g_unloading = false;
    g_threadClosing = false;
    g_recentSettings.enabled = false;
    ScheduleRecentWork(true, true);
    Require(!g_workTimer && !g_recentSyncPending && !g_refreshPending,
            "disabled recent work queued");
    g_recentSettings.enabled = true;
    ScheduleRecentWork(true, false);
    const auto timer = g_workTimer;
    ScheduleRecentWork(false, true);
    Require(timer && g_workTimer == timer && g_recentSyncPending && g_refreshPending,
            "deferred requests did not coalesce");
    g_noticeTimer = SetTimer(nullptr, 0, 1000, NoticeTimerProc);
    StopThreadWork();
    Require(!g_workTimer && !g_noticeTimer && !g_recentSyncPending && !g_refreshPending,
            "stop left timers or pending work");
    ShellEventSink* sink = new ShellEventSink();
    void* result = nullptr;
    Require(sink->QueryInterface(IID_IUnknown, nullptr) == E_POINTER,
            "sink accepted null interface output");
    for (const auto& iid : {IID_IUnknown, IID_IDispatch, kWebBrowserEvents2, kShellWindowsEvents}) {
        Require(sink->QueryInterface(iid, &result) == S_OK && result,
                "sink rejected supported event interface");
        static_cast<IUnknown*>(result)->Release();
    }
    Require(sink->QueryInterface(IID_IFileDialog, &result) == E_NOINTERFACE && !result,
            "sink accepted unsupported interface");
    UINT count = 99;
    Require(sink->GetTypeInfoCount(nullptr) == E_POINTER &&
            sink->GetTypeInfoCount(&count) == S_OK && count == 0 &&
            sink->GetTypeInfo(0, 0, nullptr) == E_NOTIMPL &&
            sink->GetIDsOfNames(IID_NULL, nullptr, 0, 0, nullptr) == E_NOTIMPL,
            "sink type-info contract failed");
    g_threadClosing = true;
    Require(sink->Invoke(kDispidWindowRegistered, IID_NULL, 0, 0, nullptr,
                         nullptr, nullptr, nullptr) == S_OK && !g_workTimer,
            "closing thread accepted shell event work");
    g_threadClosing = false;
    g_recentSettings.enabled = true;
    OnShellEvent(kDispidWindowRegistered, nullptr);
    Require(g_recentSyncPending && g_workTimer, "registration event did not schedule sync");
    StopThreadWork();
    OnShellEvent(kDispidDocumentComplete, nullptr);
    Require(!g_refreshPending, "navigation without arguments queued refresh");
    VARIANT argument{};
    DISPPARAMS params{&argument, nullptr, 1, 0};
    OnShellEvent(kDispidNavigateComplete2, &params);
    Require(!g_refreshPending, "incomplete navigation arguments queued refresh");
    Require(sink->Release() == 0, "sink reference count leaked");
    g_recentSettings.enabled = false;
    BarState drag;
    Require(!DragInProgress(drag), "empty drag active");
    drag.dragPath = L"C:\\folder";
    drag.dragging = true;
    drag.dragFromRecents = true;
    drag.refreshAfterDrag = true;
    Require(DragInProgress(drag), "non-mouse drag unexpectedly cancelled");
    ClearDrag(drag);
    Require(drag.dragPath.empty() && !drag.dragging && !drag.dragFromRecents &&
            !drag.refreshAfterDrag, "drag cleanup left flags");
    HWND frame = CreateTestFrame();
    BookmarkChange change;
    change.noRoom = 1;
    MSG message{};
    message.hwnd = frame;
    message.message = RefreshBarsMessage();
    message.wParam = EncodeNotice(change);
    ThreadMessageHook(HC_ACTION, PM_REMOVE, reinterpret_cast<LPARAM>(&message));
    Require(message.message == WM_NULL && g_refreshPending &&
            g_pendingNotice.kind == BookmarkNotice::Full, "message hook lost refresh or notice");
    StopThreadWork();
    message.message = RefreshBarsMessage();
    ThreadMessageHook(HC_ACTION, PM_NOREMOVE, reinterpret_cast<LPARAM>(&message));
    Require(message.message == RefreshBarsMessage() && !g_refreshPending,
            "peek-only hook consumed message");
    Require(!FrameHasInstalledBar(frame), "native-only frame claimed an installed bar");
    DestroyWindow(frame);
    std::puts("PASS: exception guards, reverse revocation, operation scopes, deferred timers and Shell sink contracts");
}

void CheckNativeFolderStatus() {
    fixture::Reset();
    BackupFixture scratch;
    const auto first = scratch.directory + L"\\first";
    Require(CreateDirectoryW(first.c_str(), nullptr), "status target directory creation failed");
    scratch.Write("test file");
    Require(CheckFolderStatus(first) == FolderStatus::Available &&
            CheckFolderStatus(scratch.file) == FolderStatus::Missing &&
            CheckFolderStatus(first + L"\\absent") == FolderStatus::Missing &&
            CheckFolderStatus(L"relative") == FolderStatus::Unknown,
            "native directory/file/missing status classification failed");
    Require(RemoveDirectoryW(first.c_str()), "status target cleanup failed");
    fixture::Reset();
    std::puts("PASS: native directory, file, missing and relative folder status");
}

void CheckIconPixelsAndCaches() {
    BackupFixture scratch;
    const auto pixels = FolderIconPixels(scratch.directory);
    Require(pixels && pixels->width > 0 && pixels->height > 0 &&
            pixels->bytes.size() == static_cast<size_t>(pixels->width * pixels->height * 4),
            "real Shell folder icon did not decode to BGRA pixels");
    const auto missing = scratch.directory + L"\\absent";
    Require(!FolderIconPixels(missing), "nonexistent path yielded icon pixels");
    g_iconCache.clear();
    Require(!CachedFolderBitmap(missing) && g_iconCache.size() == 1 &&
            g_iconCache.front().attempted && !g_iconCache.front().pixels,
            "failed icon request was not cached");
    const auto loaded = FolderIconLoadedAt(missing);
    Require(loaded && !CachedFolderBitmap(missing) && FolderIconLoadedAt(missing) == loaded,
            "failed icon cache ignored retry interval");
    for (size_t i = 1; i <= kMaxCachedIcons; ++i) {
        CachedFolderBitmap(missing + std::to_wstring(i));
    }
    Require(g_iconCache.size() == kMaxCachedIcons && FolderIconLoadedAt(missing) == 0,
            "icon cache exceeded cap or retained oldest entry");
    const auto cached = g_iconCache.front().path;
    ForgetCachedIcon(cached);
    Require(g_iconCache.size() == kMaxCachedIcons - 1 && FolderIconLoadedAt(cached) == 0,
            "explicit icon invalidation failed");
    Require(FolderTooltip(L"C:\\folder", FolderStatus::Missing, false).find(L"Folder not found") != std::wstring::npos &&
            FolderTooltip(L"C:\\folder", FolderStatus::Available, true).find(L"Drag onto") != std::wstring::npos &&
            FolderTooltip(L"C:\\folder", FolderStatus::Unknown, false).find(L"Folder not found") == std::wstring::npos,
            "folder tooltip status/recent instructions failed");
    g_iconCache.clear();
    std::puts("PASS: real Shell icon decoding, failed icon cache, eviction, invalidation and tooltips");
}

#include "com-fixture.h"
#include "native-checks.h"
#include "platform-failure-checks.h"
#include "allocation-checks.h"
#include "branch-gap-checks.h"
#include "runtime-gap-checks.h"
#include "xaml-checks.h"

int main(int argc, char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    Require(SUCCEEDED(WerSetFlags(WER_FAULT_REPORTING_NO_UI)),
            "could not disable interactive crash reporting for the test process");
    ComScope com;
    Require(SUCCEEDED(com.result), "test COM initialization failed");
    if (argc == 2 && std::string_view(argv[1]) == "--xaml") {
        try { CheckXamlRuntime(); }
        catch (const winrt::hresult_error& error) {
            std::fprintf(stderr, "FAIL: XAML exception %08X: %ls\n", static_cast<unsigned>(error.code().value), error.message().c_str());
            return 2;
        }
        return 0;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--preview") {
        PreviewXamlRuntime();
        return 0;
    }
    CheckBookmarkBoundaries();
    CheckEncodingStorageAndLayout();
    CheckInsertionMatrix();
    CheckStoredMutations();
    CheckSettingsAndRecents();
    CheckBackupFiles();
    CheckFramesAndSettings();
    CheckStorageMutex();
    CheckCallbacksAndDeferredWork();
    CheckNativeFolderStatus();
    CheckIconPixelsAndCaches();
    CheckShellSelectionAndNavigation();
    CheckShellConnections();
    CheckBackupDialogContracts();
    CheckDriveAndDeferredNativePaths();
    CheckNativeThreadRouting();
    CheckStorageAbandonmentAndFailure();
    CheckHookCoordinators();
    CheckPlatformFailures();
    CheckBackupAllocationRecovery();
    CheckIconAllocationCleanup();
    CheckBookmarkStorageEdges();
    CheckBackupSchemaAndSizeEdges();
    CheckRecentStorageAndViewEdges();
    CheckSettingsSnapshotEdges();
    CheckShellDiscoveryAndSelectionGaps();
    CheckNavigationEventAndTrackerGaps();
    CheckDeferredWorkAndSettingsGaps();
    CheckNativeFrameAndInitializationGaps();
    CheckUninitPendingOperationAndDropGaps();
    return 0;
}
