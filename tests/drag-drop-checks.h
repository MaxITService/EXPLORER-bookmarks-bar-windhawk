#pragma once
#include "drag-drop-fixture.h"

// Requires an initialized XamlRuntime; every control belongs to this process.
void CheckExternalDropContracts() {
    fixture::Reset();
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture bar;
    SaveBookmarksLocked({L"\\\\drop.invalid\\old-A", L"\\\\drop.invalid\\old-B"});
    RefreshPanel(bar.panel);
    bar.strip.Width(500);
    bar.strip.Height(kRowHeight);
    bar.grid.Measure({500, 200});
    bar.grid.Arrange({0, 0, 500, 200});
    const auto buttons = BarButtons(bar.panel);
    Require(buttons.size() == 4 && bar.strip.ActualWidth() > 0 && bar.strip.ActualHeight() > 0,
            "external drop test strip was not arranged");
    drop_fixture::Request request(bar.strip);
    const auto event = request.Event();
    muxc::StackPanel unrelated;
    OnBarDragOver(unrelated, event);
    Require(request.args->accepted == wdt::DataPackageOperation::None,
            "untracked panel accepted external drop");
    g_unloading = true;
    OnBarDragOver(bar.panel, event);
    g_unloading = false;
    Require(request.args->accepted == wdt::DataPackageOperation::None,
            "unloading bar accepted external drop");
    request.view->storageItems = false;
    OnBarDragOver(bar.panel, event);
    request.view->storageItems = true;
    Require(request.args->accepted == wdt::DataPackageOperation::None,
            "text-only drag accepted as folders");
    const std::pair<wdt::DataPackageOperation, wdt::DataPackageOperation> operations[] = {
        {wdt::DataPackageOperation::None, wdt::DataPackageOperation::None},
        {wdt::DataPackageOperation::Move, wdt::DataPackageOperation::None},
        {wdt::DataPackageOperation::Copy, wdt::DataPackageOperation::Copy},
        {wdt::DataPackageOperation::Copy | wdt::DataPackageOperation::Move, wdt::DataPackageOperation::Copy},
        {wdt::DataPackageOperation::Link, wdt::DataPackageOperation::Link},
        {wdt::DataPackageOperation::Link | wdt::DataPackageOperation::Copy | wdt::DataPackageOperation::Move, wdt::DataPackageOperation::Link}
    };
    for (const auto& [allowed, expected] : operations) {
        request.args->allowed = allowed;
        OnBarDragOver(bar.panel, event);
        Require(request.args->accepted == expected,
                "external drag accepted destructive Move or ignored Link/Copy precedence");
    }
    Require(request.args->ui->caption == L"Add to bookmarks" && request.args->ui->captionVisible &&
            bar.state->dropTarget.get() == buttons[2] && !bar.state->dropAfter,
            "accepted drag did not show caption and leading bookmark marker");
    for (const drop_fixture::wf::Point position : {
            drop_fixture::wf::Point{-1, 5}, {5, -1}, {501, 5}, {5, kRowHeight + 1}}) {
        request.args->stripPoint = position;
        Require(ExternalDropIndex(*bar.state, bar.panel, event) == SIZE_MAX,
                "drop outside bookmark strip did not append");
    }
    request.args->stripPoint = {0, 0};
    Require(ExternalDropIndex(*bar.state, bar.panel, event) == 0,
            "top-left strip edge rejected insertion");
    request.args->stripPoint = {static_cast<float>(bar.strip.ActualWidth()),
                                static_cast<float>(bar.strip.ActualHeight())};
    request.args->panelPoint = {10000, 5};
    Require(ExternalDropIndex(*bar.state, bar.panel, event) == 2,
            "bottom-right strip edge did not permit append index");
    bar.state->strip = nullptr;
    Require(ExternalDropIndex(*bar.state, bar.panel, event) == SIZE_MAX,
            "missing strip did not default to append");
    bar.state->strip = winrt::make_weak(bar.strip);
    request.args->stripPoint = {5, 5};
    request.args->panelPoint = {0, 5};
    OnBarDragOver(bar.panel, event);
    OnBarDrop(unrelated, event);
    Require(request.view->reads == 0 && g_pendingDrops == 0,
            "untracked panel started item read");
    g_unloading = true;
    OnBarDrop(bar.panel, event);
    g_unloading = false;
    Require(!bar.state->dropTarget.get() && request.view->reads == 0,
            "unload guard kept insertion marker or read dropped items");
    OnBarDragOver(bar.panel, event);
    request.view->storageItems = false;
    OnBarDrop(bar.panel, event);
    request.view->storageItems = true;
    Require(!bar.state->dropTarget.get() && request.view->reads == 0,
            "unsupported drop kept marker or started item read");
    request.args->accepted = wdt::DataPackageOperation::None;
    OnBarDrop(bar.panel, event);
    Require(request.view->reads == 0 && g_pendingDrops == 0,
            "unaccepted drop started asynchronous read");
    std::puts("PASS: external drop guards, non-destructive operation precedence, caption, marker and strip boundaries");
}

void CheckExternalDropCompletionAndFailures() {
    fixture::Reset();
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture bar;
    const std::wstring a = L"\\\\drop.invalid\\A";
    const std::wstring b = L"\\\\drop.invalid\\B";
    const std::wstring c = L"\\\\drop.invalid\\C";
    const std::wstring d = L"\\\\drop.invalid\\D";
    SaveBookmarksLocked({a, b});
    RefreshPanel(bar.panel);
    bar.strip.Width(500);
    bar.strip.Height(kRowHeight);
    bar.grid.Measure({500, 200});
    bar.grid.Arrange({0, 0, 500, 200});
    drop_fixture::Request request(bar.strip);
    request.read->SetItems({drop_fixture::File(L"C:\\file.txt"), drop_fixture::Folder(L""),
                            drop_fixture::Folder(c), drop_fixture::Folder(c),
                            drop_fixture::Folder(a), drop_fixture::Folder(d)});
    OnBarDragOver(bar.panel, request.Event());
    OnBarDrop(bar.panel, request.Event());
    Require(g_pendingDrops == 1 && request.read->registrations == 1 && request.read->resultReads == 0 &&
            request.args->deferrals == 0 && SplitBookmarks(ReadStorageLocked()) == std::vector<std::wstring>{a, b} &&
            !bar.state->dropTarget.get(),
            "drop did not return with one pending, non-deferred read and cleared marker");
    request.read->Finish();
    Require(g_pendingDrops == 0 && request.read->callbacks == 1 && request.read->resultReads == 1 &&
            SplitBookmarks(ReadStorageLocked()) == std::vector<std::wstring>{c, d, a, b},
            "drop completion did not insert unique nonempty folders at captured index");

    // Rejected asynchronous states must consume the pending count without
    // GetResults or touching the saved bookmarks.
    const auto stable = ReadStorageLocked();
    for (auto status : {drop_fixture::wf::AsyncStatus::Error, drop_fixture::wf::AsyncStatus::Canceled}) {
        drop_fixture::Request rejected(bar.strip);
        rejected.args->accepted = wdt::DataPackageOperation::Copy;
        rejected.read->SetItems({drop_fixture::Folder(L"\\\\drop.invalid\\not-inserted")});
        OnBarDrop(bar.panel, rejected.Event());
        Require(g_pendingDrops == 1, "failed drop was not pending before completion");
        rejected.read->Finish(status);
        Require(g_pendingDrops == 0 && rejected.read->resultReads == 0 && ReadStorageLocked() == stable,
                "failed/canceled item read mutated storage or leaked pending drop");
    }
    drop_fixture::Request failedRegistration(bar.strip);
    failedRegistration.args->accepted = wdt::DataPackageOperation::Link;
    failedRegistration.read->failRegistration = true;
    OnBarDrop(bar.panel, failedRegistration.Event());
    Require(g_pendingDrops == 0 && failedRegistration.read->registrations == 1 &&
            !failedRegistration.read->completion && ReadStorageLocked() == stable,
            "completion-registration failure leaked pending count");
    drop_fixture::Request failedResults(bar.strip);
    failedResults.args->accepted = wdt::DataPackageOperation::Link;
    failedResults.read->failResults = true;
    OnBarDrop(bar.panel, failedResults.Event());
    failedResults.read->Finish();
    Require(g_pendingDrops == 0 && ReadStorageLocked() == stable,
            "GetResults exception leaked pending drop or changed bookmarks");
    auto failingItem = winrt::make_self<drop_fixture::Item>(L"\\\\drop.invalid\\throw");
    failingItem->failPath = true;
    drop_fixture::Request failedItem(bar.strip);
    failedItem.args->accepted = wdt::DataPackageOperation::Copy;
    failedItem.read->SetItems({drop_fixture::Folder(c), failingItem.as<drop_fixture::ws::IStorageItem>()});
    OnBarDrop(bar.panel, failedItem.Event());
    failedItem.read->Finish();
    Require(g_pendingDrops == 0 && ReadStorageLocked() == stable,
            "item property exception partially inserted folders or leaked pending count");
    drop_fixture::Request unloading(bar.strip);
    unloading.args->accepted = wdt::DataPackageOperation::Link;
    unloading.read->SetItems({drop_fixture::Folder(L"\\\\drop.invalid\\late")});
    OnBarDrop(bar.panel, unloading.Event());
    g_unloading = true;
    unloading.read->Finish();
    g_unloading = false;
    Require(g_pendingDrops == 0 && ReadStorageLocked() == stable,
            "completion during unload inserted folders or leaked pending count");

    // Already-completed WinRT operations can call the completion callback
    // synchronously during registration; the increment must happen first.
    drop_fixture::Request alreadyDone(bar.strip);
    alreadyDone.args->accepted = wdt::DataPackageOperation::Copy;
    alreadyDone.args->stripPoint = {-1, 5};
    alreadyDone.read->SetItems({drop_fixture::Folder(L"\\\\drop.invalid\\sync")});
    alreadyDone.read->Finish();
    OnBarDrop(bar.panel, alreadyDone.Event());
    Require(g_pendingDrops == 0 && alreadyDone.read->callbacks == 1 &&
            SplitBookmarks(ReadStorageLocked()).back() == L"\\\\drop.invalid\\sync",
            "synchronous completion underflowed counter or did not append outside strip");

    // Two independent drops may finish out of order; each uses the index
    // captured at drop time and reads the latest storage under its mutex.
    SaveBookmarksLocked({a, b});
    drop_fixture::Request first(bar.strip), second(bar.strip);
    for (auto* pending : {&first, &second}) pending->args->accepted = wdt::DataPackageOperation::Link;
    first.read->SetItems({drop_fixture::Folder(c)});
    second.read->SetItems({drop_fixture::Folder(d)});
    OnBarDrop(bar.panel, first.Event());
    OnBarDrop(bar.panel, second.Event());
    Require(g_pendingDrops == 2, "independent drops did not track two pending reads");
    second.read->Finish();
    Require(g_pendingDrops == 1, "first of two completions lost remaining pending read");
    first.read->Finish();
    Require(g_pendingDrops == 0 && SplitBookmarks(ReadStorageLocked()) == std::vector<std::wstring>{c, d, a, b},
            "out-of-order completion overwrote another drop or leaked its count");
    std::puts("PASS: asynchronous drop insertion, delayed/synchronous/out-of-order reads, cancel/error/exception and unload cleanup");
}

void CheckExternalDropLimitsAndRealPackage() {
    fixture::Reset();
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture bar;
    drop_fixture::Request limited(bar.strip);
    limited.args->accepted = wdt::DataPackageOperation::Copy;
    std::vector<drop_fixture::ws::IStorageItem> items;
    for (size_t i = 0; i < kMaxFoldersPerAction; ++i) items.push_back(drop_fixture::Folder(L"\\\\drop.invalid\\same"));
    items.push_back(drop_fixture::Folder(L"\\\\drop.invalid\\beyond-action-limit"));
    limited.read->SetItems(std::move(items));
    OnBarDrop(bar.panel, limited.Event());
    limited.read->Finish();
    Require(g_pendingDrops == 0 && SplitBookmarks(ReadStorageLocked()) ==
            std::vector<std::wstring>{L"\\\\drop.invalid\\same"},
            "drop reader scanned past folder action limit before duplicate filtering");
    fixture::failWrites = true;
    const auto prior = ReadStorageLocked();
    drop_fixture::Request failedWrite(bar.strip);
    failedWrite.args->accepted = wdt::DataPackageOperation::Link;
    failedWrite.read->SetItems({drop_fixture::Folder(L"\\\\drop.invalid\\not-saved")});
    OnBarDrop(bar.panel, failedWrite.Event());
    failedWrite.read->Finish();
    Require(g_pendingDrops == 0 && ReadStorageLocked() == prior,
            "storage write failure changed bookmarks or leaked pending count");
    fixture::failWrites = false;
    SaveBookmarksLocked({});
    BackupFixture scratch;
    auto folder = drop_fixture::ws::StorageFolder::GetFolderFromPathAsync(scratch.directory).get();
    wdt::DataPackage package;
    package.SetStorageItems({folder});
    auto args = winrt::make_self<drop_fixture::DragArgs>();
    args->strip = bar.strip;
    args->view = package.GetView();
    args->accepted = wdt::DataPackageOperation::Copy;
    Require(HasStorageItems(args.as<mux::DragEventArgs>()), "real DataPackage did not expose storage items");
    OnBarDrop(bar.panel, args.as<mux::DragEventArgs>());
    const auto deadline = GetTickCount64() + 5000;
    while (g_pendingDrops) {
        Require(GetTickCount64() < deadline, "real DataPackage asynchronous drop did not finish");
        PumpXamlMessages(20);
    }
    Require(SplitBookmarks(ReadStorageLocked()) == std::vector<std::wstring>{scratch.directory} &&
            args->deferrals == 0,
            "real StorageFolder/DataPackage drop did not bookmark its temporary folder");
    fixture::Reset();
    std::puts("PASS: external drop action limit, write failure and real StorageFolder/DataPackage async integration");
}
