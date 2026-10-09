#pragma once

void CheckBackupAllocationRecovery() {
    fixture::Reset();
    BackupFixture backup;
    const std::string sentinel = "existing backup survives out-of-memory serialization";
    backup.Write(sentinel);
    fixture::storage[g_bookmarksKey] = L"C:\\preserved";
    const auto before = fixture::storage;
    const auto handlesBefore = [&] {
        DWORD handles = 0;
        Require(GetProcessHandleCount(GetCurrentProcess(), &handles), "allocation-test handle query failed");
        return handles;
    }();
    bool saved;
    {
        allocation_fixture::Scope fault((kMaxStorageChars + 1) * sizeof(wchar_t));
        saved = SaveBackup(backup.file);
        Require(fault.Triggered(), "backup serialization allocation fault was not consumed");
    }
    Require(!saved && PlatformReadBytes(backup.file) == sentinel && fixture::storage == before,
            "failed backup serialization changed source storage or destination file");

    backup.Write("{\"format\":\"explorer-folder-bookmarks-bar\",\"version\":1,\"folders\":[\"C:\\\\imported\"]}");
    bool loaded;
    {
        allocation_fixture::Scope fault;
        loaded = LoadBackup(backup.file);
        Require(fault.Triggered(), "backup input buffer allocation fault was not consumed");
    }
    DWORD handlesAfter = 0;
    Require(!loaded && fixture::storage == before &&
            GetProcessHandleCount(GetCurrentProcess(), &handlesAfter) && handlesBefore == handlesAfter,
            "failed backup input allocation changed storage or leaked its open file handle");
    Require(LoadBackup(backup.file) && fixture::storage[g_bookmarksKey] == L"C:\\imported" &&
            SaveBackup(backup.file), "backup operations did not recover after one-shot allocation faults");
    fixture::Reset();
    std::puts("PASS: out-of-memory backup preservation, file-handle cleanup and subsequent recovery");
}

void CheckIconAllocationCleanup() {
    platform_failure::Scope platform;
    const auto oldCreation = fixture::creation;
    fixture::creation = platform_failure::IconCreation;
    platform_failure::faults.shellIcon = platform_failure::ShellIcon::OwnedIcon;
    platform_failure::icon = {};
    platform_failure::icon.width = 13;
    platform_failure::icon.height = 17;
    const auto destroyed = platform_failure::faults.destroyedIcons;
    std::optional<IconPixels> pixels;
    {
        allocation_fixture::Scope fault(13 * 17 * 4);
        pixels = FolderIconPixels(L"allocation-fixture-icon");
        Require(fault.Triggered(), "icon pixel-vector allocation fault was not consumed");
    }
    Require(!pixels && platform_failure::icon.live == 0 && platform_failure::icon.copies == 0 &&
            platform_failure::faults.destroyedIcons == destroyed + 1,
            "failed icon pixel allocation leaked icon/COM ownership or copied into absent buffer");
    pixels = FolderIconPixels(L"allocation-fixture-icon");
    Require(pixels && pixels->bytes.size() == 13 * 17 * 4 && platform_failure::icon.live == 0 &&
            platform_failure::faults.destroyedIcons == destroyed + 2,
            "icon decoding did not recover with released ownership after allocation failure");
    fixture::creation = oldCreation;
    std::puts("PASS: out-of-memory icon pixels release all native/COM ownership and recover");
}
