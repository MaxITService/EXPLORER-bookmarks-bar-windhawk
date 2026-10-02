#pragma once
// Behavioral boundary checks of the real source. These helpers are included
// after stability-runtime's fixtures; they never replace a production function.

void CheckBookmarkStorageEdges() {
    fixture::Reset();
    const std::vector<std::wstring> valid{L"a:\\", L"Z:\\folder", L"\\\\s\\h"};
    for (const auto& path : valid) {
        Require(IsAbsoluteFolderPath(path) && ValidateImportedFolders({path}),
                "absolute drive/UNC boundary rejected");
    }
    const std::vector<std::wstring> invalid{
        L"", L"a", L"a:", L"0:\\folder", L"@:\\folder", L"[:\\folder",
        L"`:\\folder", L"{:\\folder", L"C?\\folder", L"C:/folder",
        L"\\\\", L"\\\\s", L"\\\\server", L"\\\\\\server\\share",
        std::wstring(L"C:\\bad\tname"), std::wstring(L"C:\\bad\rname")};
    for (const auto& path : invalid) {
        Require(!IsAbsoluteFolderPath(path) && !ValidateImportedFolders({path}),
                "invalid absolute-path boundary accepted");
    }
    Require(IsAbsoluteFolderPath(L"\\\\server\\") &&
            !ValidateImportedFolders({L"\\\\server\\"}),
            "unnormalized UNC root accepted for import");
    for (const auto& path : std::vector<std::wstring>{L"", L"C", L"C?\\folder", L"C:folder",
             L"0:\\folder", L"[:\\folder", L"{:\\folder"}) {
        Require(CheckFolderStatus(path) == FolderStatus::Unknown,
                "malformed drive path received a missing/available badge");
    }
    fixture::drives[L"a:\\"] = DRIVE_RAMDISK;
    fixture::attributes[L"a:/folder"] = {FILE_ATTRIBUTE_DIRECTORY, ERROR_SUCCESS};
    Require(CheckFolderStatus(L"a:/folder") == FolderStatus::Available,
            "lowercase RAM-disk path with forward separator was not checked");
    const std::wstring maximum = L"C:\\" + std::wstring(kMaxStorageChars - 3, L'x');
    Require(ValidateImportedFolders({maximum}) && SaveBookmarksLocked({maximum}) &&
            ReadStorageLocked() == maximum,
            "exact storage limit was rejected or shortened");
    const auto atLimit = fixture::storage;
    const auto writesAtLimit = fixture::writes;
    Require(!SaveBookmarksLocked({maximum, L"D:\\"}) &&
            fixture::writes == writesAtLimit && fixture::storage == atLimit,
            "oversize bookmark save touched storage");

    std::vector<std::wstring> full;
    for (size_t i = 0; i < kMaxBookmarks; ++i) {
        full.push_back(L"C:\\folder" + std::to_wstring(i));
    }
    Require(SaveBookmarksLocked(full), "capacity fixture save failed");
    const auto beforeFull = fixture::storage;
    const auto beforeWrites = fixture::writes;
    auto change = AddBookmark(L"D:\\extra");
    Require(!change.changed && change.noRoom == 1 && !change.added &&
            change.Notice() == BookmarkNotice::Full && fixture::storage == beforeFull &&
            fixture::writes == beforeWrites,
            "single add at capacity wrote storage or lost full result");
    change = InsertBookmarks({full.front(), L"relative"}, 0);
    Require(!change.changed && !change.added && change.existing == 1 && change.invalid == 1 &&
            change.Notice() == BookmarkNotice::None && fixture::writes == beforeWrites,
            "duplicate/invalid batch wrote storage or claimed already-bookmarked success");
    Require(!ToggleBookmarks({L"", L""}).changed && fixture::writes == beforeWrites,
            "empty toggle wrote storage");
    fixture::failWrites = true;
    Require(!MoveBookmarkToIndex(full.back(), 0) && fixture::storage == beforeFull,
            "failed reorder mutated persisted bookmarks");
    const auto failedWrites = fixture::writes;
    Require(!SaveLinesLocked(L"history", {L"C:\\recent"}) &&
            fixture::writes == failedWrites + 1 && !fixture::storage.contains(L"history"),
            "failed history save created a persisted value");
    fixture::failWrites = false;
    Require(NoticeText(MakeNotice(nullptr, BookmarkChange{})).empty() &&
            NoticeText(MakeNotice(nullptr, BookmarkChange{false, 0, 1})) ==
                L"Bookmarks full (32 folders)" &&
            NoticeText(MakeNotice(nullptr, BookmarkChange{false, 0, 0, 1})) ==
                L"Already bookmarked",
            "empty/full/duplicate bookmark notice text failed");
    fixture::Reset();
    std::puts("PASS: absolute-path boundaries, exact storage size, capacity no-ops and failed reorder/history writes");
}

void CheckBackupSchemaAndSizeEdges() {
    fixture::Reset();
    BackupFixture backup;
    const std::string prefix = "{\"format\":\"explorer-folder-bookmarks-bar\",\"version\":1,\"folders\":";
    const std::string valid = prefix + "[\"C:\\\\valid\"]}";
    fixture::storage[kBookmarksKey] = L"C:\\sentinel";
    const auto sentinel = fixture::storage;
    const std::vector<std::string> rejected{
        "null", "42", "true", "\"text\"",
        "{\"format\":null,\"version\":1,\"folders\":[]}",
        "{\"format\":42,\"version\":1,\"folders\":[]}",
        "{\"format\":\"explorer-folder-bookmarks-bar\",\"folders\":[]}",
        "{\"format\":\"explorer-folder-bookmarks-bar\",\"version\":1}",
        "{\"format\":\"explorer-folder-bookmarks-bar\",\"version\":1,\"folders\":null}",
        "{\"format\":\"explorer-folder-bookmarks-bar\",\"version\":1,\"folders\":{}}",
        prefix + "[false]}", prefix + "[null]}", prefix + "[{}]}", prefix + "[[]]}",
        prefix + "[\"C:\\\\bad\\rname\"]}", prefix + "[\"C:\\\\bad\\nname\"]}",
        std::string("\xEF\xBB\xBF", 3),
        std::string("\xEF", 1) + "xx" + valid,
        std::string("\xEF\xBB", 2) + "x" + valid,
        prefix + "[\"C:\\\\" + std::string(kMaxStorageChars - 2, 'x') + "\"]}"};
    for (const auto& bytes : rejected) {
        backup.Write(bytes);
        const auto writes = fixture::writes;
        Require(!LoadBackup(backup.file) && fixture::storage == sentinel &&
                fixture::writes == writes,
                "schema/encoding/length rejection changed persisted bookmarks");
    }
    // Include the newline between two stored paths when enforcing the cap.
    backup.Write(prefix + "[\"C:\\\\" + std::string(kMaxStorageChars / 2 - 3, 'x') +
                 "\",\"D:\\\\" + std::string(kMaxStorageChars / 2 - 3, 'y') + "\"]}");
    Require(!LoadBackup(backup.file) && fixture::storage == sentinel,
            "import omitted separator from storage-size limit");
    backup.Write(prefix + "[\"C:\\\\" + std::string(kMaxStorageChars - 3, 'x') + "\"]}");
    Require(LoadBackup(backup.file) && ReadStorageLocked().size() == kMaxStorageChars,
            "exact storage-size JSON import was rejected or truncated");
    std::string exactBytes = valid;
    exactBytes.append(kMaxImportBytes - exactBytes.size(), ' ');
    backup.Write(exactBytes);
    Require(LoadBackup(backup.file) && ReadStorageLocked() == L"C:\\valid",
            "exact byte-size JSON backup was rejected");
    fixture::Reset();
    std::puts("PASS: JSON schema types, partial/BOM-only encodings and exact import/storage-size boundaries");
}

void CheckRecentStorageAndViewEdges() {
    fixture::Reset();
    std::vector<std::wstring> hiddenLines;
    for (size_t i = 0; i < kMaxStoredRecents; ++i) {
        hiddenLines.push_back(FormatFileTime(i + 1) + L"\t\\\\s\\folder" + std::to_wstring(i));
    }
    fixture::storage[kWindowsRecentHiddenKey] = JoinLines(hiddenLines);
    HideWindowsRecentLocked(L"\\\\s\\new");
    auto hidden = ReadHiddenWindowsRecentsLocked();
    Require(hidden.size() == kMaxStoredRecents && hidden.front().path == L"\\\\s\\new" &&
            hidden.back().path == L"\\\\s\\folder62",
            "hidden history cap failed to evict oldest item");
    HideWindowsRecentLocked(L"\\\\S\\FOLDER10");
    hidden = ReadHiddenWindowsRecentsLocked();
    Require(hidden.size() == kMaxStoredRecents && hidden.front().path == L"\\\\S\\FOLDER10" &&
            std::count_if(hidden.begin(), hidden.end(), [](const auto& item) {
                return SamePath(item.path, L"\\\\s\\folder10");
            }) == 1, "hidden recent promotion retained a duplicate");
    fixture::storage[kRecentsKey] = L"\\\\s\\one\n\\\\s\\two";
    const auto before = fixture::storage;
    const auto writes = fixture::writes;
    RemoveRecent(RecentSource::Explorer, L"\\\\s\\absent");
    Require(fixture::storage == before && fixture::writes == writes,
            "absent recent removal wrote storage");
    fixture::failWrites = true;
    RemoveRecent(RecentSource::Explorer, L"\\\\s\\one");
    HideWindowsRecentLocked(L"\\\\s\\failed");
    Require(!RecordExplorerRecent(L"\\\\s\\failed") && fixture::storage == before,
            "failed recent write mutated persisted history");
    fixture::failWrites = false;

    const auto previousSettings = g_recentSettings;
    fixture::storage[kRecentsKey] =
        L"C:\\missing\n\\\\s\\one\n\\\\S\\ONE\n\\\\s\\two\n\\\\s\\three";
    fixture::drives[L"C:\\"] = DRIVE_FIXED;
    fixture::attributes[L"C:\\missing"] = {INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND};
    g_recentSettings.history = 10;
    auto view = LoadRecentView({L"\\\\S\\TWO"});
    Require(view.source == RecentSource::Explorer && !view.windowsRecentsOff &&
            view.folders == std::vector<std::wstring>{L"\\\\s\\one", L"\\\\s\\three"},
            "recent view failed missing/duplicate/case-insensitive bookmark filtering");
    g_recentSettings.history = 0;
    Require(LoadRecentView({}).folders.empty(), "zero recent history exposed an item");

    const bool oldStopping = g_windowsRecentStopping;
    const bool oldActive = g_windowsRecentsActive;
    const bool oldRequested = g_windowsRecentsRequested;
    const auto oldWindows = g_windowsRecents;
    Require(!g_windowsRecentThread && !g_windowsRecentWake,
            "recent view fixture started while a worker was live");
    g_windowsRecentStopping = true;
    g_windowsRecentsActive = false;
    g_windowsRecentsRequested = false;
    g_recentSettings.history = 10;
    fixture::storage[kRecentSourceKey] = L"windows";
    fixture::storage[kWindowsRecentClearedKey] = FormatFileTime(1);
    fixture::storage[kWindowsRecentHiddenKey] = FormatFileTime(20) + L"\t\\\\s\\hidden";
    g_windowsRecents = {{L"\\\\s\\different", 10}, {L"\\\\s\\hidden", 10},
                       {L"\\\\s\\hidden", 21}, {L"\\\\S\\HIDDEN", 22},
                       {L"C:\\missing", 23}, {L"\\\\s\\old", 1}};
    view = LoadRecentView({});
    Require(view.folders == std::vector<std::wstring>{L"\\\\s\\different", L"\\\\s\\hidden"},
            "Windows recent view hid unrelated paths or failed reuse/deduplication/missing filtering");
    g_windowsRecents = oldWindows;
    g_windowsRecentStopping = oldStopping;
    g_windowsRecentsActive = oldActive;
    g_windowsRecentsRequested = oldRequested;
    g_recentSettings = previousSettings;
    fixture::Reset();
    std::puts("PASS: hidden history eviction/promotion, failed recent writes and Explorer/Windows view filter edges");
}

void CheckSettingsSnapshotEdges() {
    fixture::Reset();
    const auto previousRecent = g_sharedRecentSettings;
    const auto previousFx = g_sharedFxFolders;
    const bool previousHotkey = g_sharedBookmarkHotkey;
    fixture::integers[L"recent.enabled"] = 1;
    fixture::integers[L"recent.buttons"] = 2;
    fixture::integers[L"recent.history"] = 7;
    fixture::strings[L"fxCustomFolders[23].path"] = L"z:\\last";
    fixture::strings[L"fxCustomFolders[23].label"] = std::wstring(80, L'x');
    fixture::strings[L"fxCustomFolders[24].path"] = L"C:\\out-of-range";
    LoadSettings();
    Require(UpdateThreadSettings() && g_recentSettings.buttons == 2 &&
            g_recentSettings.history == 7 && !g_bookmarkHotkey && g_fxFolders.size() == 1 &&
            g_fxFolders.front().path == L"z:\\last" && g_fxFolders.front().label.size() == 80,
            "in-range settings or final custom-folder/label boundary failed");
    const auto appliedGeneration = g_threadSettingsGeneration;
    Require(!UpdateThreadSettings() && g_threadSettingsGeneration == appliedGeneration,
            "same generation changed settings snapshot");
    fixture::strings[L"fxCustomFolders[23].path"] = L"C:\\changed";
    LoadSettings();
    Require(g_fxFolders.front().path == L"z:\\last" &&
            g_threadSettingsGeneration == appliedGeneration,
            "settings publication mutated the owning thread's snapshot before apply");
    Require(UpdateThreadSettings() && g_fxFolders.front().path == L"C:\\changed",
            "next settings generation did not apply the replacement FX snapshot");
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings = previousRecent;
        g_sharedFxFolders = previousFx;
        g_sharedBookmarkHotkey = previousHotkey;
        ++g_settingsGeneration;
    }
    UpdateThreadSettings();
    fixture::Reset();
    std::puts("PASS: valid settings, final FX slot/80-character label and generation-controlled snapshot replacement");
}

namespace branch_worker {
struct Fixture {
    HANDLE ready = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE messageHandled = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE entered = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE release = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    std::atomic<HWND> window = nullptr;
    ~Fixture() {
        CloseHandle(ready);
        CloseHandle(messageHandled);
        CloseHandle(entered);
        CloseHandle(release);
    }
    void Probe() {
        Require(PostMessageW(window.load(), WM_APP, 0, 0), "worker probe post failed");
        Require(WaitForSingleObject(messageHandled, 5000) == WAIT_OBJECT_0,
                "worker message pump did not acknowledge probe");
    }
};
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        SetWindowLongPtrW(window, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams));
    }
    if (message == WM_APP) {
        auto* current = reinterpret_cast<Fixture*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        SetEvent(current->messageHandled);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
DWORD WINAPI Worker(void* opaque) {
    auto& current = *static_cast<Fixture*>(opaque);
    current.window = CreateWindowExW(0, L"BookmarksBranchWorker", L"", 0,
                                     0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                     GetModuleHandleW(nullptr), &current);
    Require(current.window != nullptr, "worker message window creation failed");
    SetEvent(current.ready);
    // Reuse the suite's scanner so coverage maps this coordinator specialization
    // once, rather than growing the denominator for another equivalent lambda.
    const auto result = TestWorker(nullptr);
    DestroyWindow(current.window.load());
    current.window = nullptr;
    return result;
}
}

void CheckWorkerWakeAndActiveGenerationEdges() {
    fixture::Reset();
    Require(!g_windowsRecentThread && !g_windowsRecentWake && !g_unloading,
            "worker branch fixture started with live/unloading state");
    const bool oldStopping = g_windowsRecentStopping;
    const auto previousRecent = g_sharedRecentSettings;
    const HANDLE previousEntered = scanEntered;
    const HANDLE previousRelease = releaseScan;
    const unsigned previousScanCount = scanCount;
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = branch_worker::WindowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"BookmarksBranchWorker";
    Require(RegisterClassW(&windowClass), "worker fixture class registration failed");
    branch_worker::Fixture current;
    Require(current.ready && current.messageHandled && current.entered && current.release,
            "worker branch gate creation failed");
    scanEntered = current.entered;
    releaseScan = current.release;
    scanCount = 0;
    g_windowsRecentStopping = false;
    g_windowsRecentsActive = true;
    g_windowsRecentsRequested = false;
    g_windowsRecentsScanning = false;
    g_windowsRecentsScanned = false;
    g_windowsRecents.clear();
    g_windowsRecentWake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(g_windowsRecentWake != nullptr, "worker wake event creation failed");
    g_windowsRecentThread = CreateThread(nullptr, 0, branch_worker::Worker, &current, 0, nullptr);
    Require(g_windowsRecentThread && WaitForSingleObject(current.ready, 5000) == WAIT_OBJECT_0,
            "worker branch thread setup failed");
    SetEvent(g_windowsRecentWake);
    current.Probe();
    Require(scanCount == 0 && !g_windowsRecentsRequested,
            "unrequested worker wake started a scan");
    {
        std::lock_guard lock(g_windowsRecentsMutex);
        g_windowsRecentsActive = false;
        g_windowsRecentsRequested = true;
    }
    SetEvent(g_windowsRecentWake);
    current.Probe();
    Require(scanCount == 0 && g_windowsRecentsRequested,
            "inactive source scanned or consumed its pending request");
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings.enabled = true;
    }
    fixture::storage[kRecentSourceKey] = L"windows";
    HWND frame = CreateTestFrame();
    UpdateWindowsRecentMode();
    Require(WaitForSingleObject(current.entered, 5000) == WAIT_OBJECT_0,
            "active mode transition did not start a scan");
    const auto firstGeneration = g_windowsRecentsGeneration;
    UpdateWindowsRecentMode(true);
    Require(g_windowsRecentsActive && g_windowsRecentsGeneration == firstGeneration + 1,
            "active-source invalidation did not advance generation");
    SetEvent(current.release);
    Require(WaitForSingleObject(current.entered, 5000) == WAIT_OBJECT_0,
            "invalidation during scan did not run the next generation");
    {
        std::lock_guard lock(g_windowsRecentsMutex);
        Require(g_windowsRecents.empty() && !g_windowsRecentsScanned && g_windowsRecentsScanning,
                "stale scan published while source remained active");
    }
    MSG message{};
    Require(!PeekMessageW(&message, frame, RefreshBarsMessage(), RefreshBarsMessage(), PM_REMOVE),
            "discarded scan notified Explorer bars");
    SetEvent(current.release);
    current.Probe();
    const auto readySnapshot = CachedWindowsRecentFolders();
    Require(scanCount == 2 && readySnapshot.size() == 1 && readySnapshot.front().path == L"C:\\new",
            "valid generation failed to publish its ready snapshot");
    Require(PeekMessageW(&message, frame, RefreshBarsMessage(), RefreshBarsMessage(), PM_REMOVE),
            "published scan failed to notify Explorer bars");
    const auto readyGeneration = g_windowsRecentsGeneration;
    UpdateWindowsRecentMode();
    current.Probe();
    Require(scanCount == 2 && g_windowsRecentsGeneration == readyGeneration &&
            !g_windowsRecentsRequested,
            "unchanged ready mode invalidated cache or requested a scan");
    StopWindowsRecentWorker();
    Require(!g_windowsRecentThread && !g_windowsRecentWake && !current.window &&
            !g_windowsRecentsRequested && !g_windowsRecentsScanning,
            "worker join left native resources or pending state");
    DestroyWindow(frame);
    Require(UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance),
            "worker fixture window class cleanup failed");
    g_windowsRecents.clear();
    g_windowsRecentsActive = false;
    g_windowsRecentsScanned = false;
    g_windowsRecentStopping = oldStopping;
    scanEntered = previousEntered;
    releaseScan = previousRelease;
    scanCount = previousScanCount;
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings = previousRecent;
    }
    fixture::Reset();
    std::puts("PASS: worker message wakes, unrequested/inactive requests and active-source stale/publication generations");
}
