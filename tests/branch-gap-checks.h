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
    fixture::storage[g_bookmarksKey] = L"C:\\sentinel";
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
    g_recents = SplitPaths(L"\\\\s\\one\n\\\\s\\two", kMaxStoredRecents);
    const auto before = g_recents;
    RemoveRecent(L"\\\\s\\absent");
    Require(g_recents == before, "absent recent removal changed history");

    const auto previousSettings = g_recentSettings;
    g_recents = SplitPaths(
        L"C:\\missing\n\\\\s\\one\n\\\\S\\ONE\n\\\\s\\two\n\\\\s\\three", kMaxStoredRecents);
    fixture::drives[L"C:\\"] = DRIVE_FIXED;
    fixture::attributes[L"C:\\missing"] = {INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND};
    g_recentSettings.history = 10;
    auto view = LoadRecentView({L"\\\\S\\TWO"});
    Require(view.folders == std::vector<std::wstring>{L"\\\\s\\one", L"\\\\s\\three"},
            "recent view failed missing/duplicate/case-insensitive bookmark filtering");
    g_recentSettings.history = 0;
    Require(LoadRecentView({}).folders.empty(), "zero recent history exposed an item");

    g_recentSettings = previousSettings;
    fixture::Reset();
    std::puts("PASS: absent recent removal and recent view filter edges");
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
