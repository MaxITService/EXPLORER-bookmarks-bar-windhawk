#pragma once
#include "event-gap-fixture.h"

// Invoke the very same guarded WinRT delegates retained after real registration.
// These checks cover callback contracts; they do not claim routed OS dispatch,
// pointer capture, popup templates, physical input or Explorer compatibility.
void CheckObservedFolderActionsAndWeakGuards() {
    fixture::Reset();
    event_gap::ResetObservers();
    BackupFixture scratch;
    ShellFixture shell;
    fixture::activeWindow = shell.frame;
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture bar;
    SaveBookmarksLocked({scratch.directory});
    RefreshPanel(bar.panel);
    auto button = BarButtons(bar.panel)[2];
    auto click = event_gap::Click(button);
    auto context = event_gap::Context(button);
    auto contextArgs = winrt::make_self<event_gap::ContextArgs>();
    g_unloading = true;
    click(button, nullptr);
    context(button, contextArgs->Event());
    // A delegate that fires after unloading began does nothing at all: the
    // bar is about to be removed, and unloading must not wait for it.
    Require(!contextArgs->handled && shell.browser->navigated.empty() && fixture::launches.empty() &&
            !g_activeOperations, "unloading folder delegates ran");
    g_unloading = false;
    click(button, nullptr);
    Require(shell.browser->navigated == std::vector<std::wstring>{scratch.directory},
            "observed folder Click did not navigate original path");
    fixture::keys[VK_CONTROL] = static_cast<SHORT>(0x8000);
    click(button, nullptr);
    Require(fixture::launches.size() == 1 && fixture::launches.back().first == L"opennewtab",
            "observed Ctrl+Click did not open a new tab");
    fixture::keys[VK_CONTROL] = 0;
    context(button, contextArgs->Event());
    Require(fixture::launches.size() == 2 && fixture::launches.back().first == L"opennewwindow",
            "observed folder context request did not open a new window");
    context(nullptr, contextArgs->Event());
    click(nullptr, nullptr);
    fixture::activeWindow = nullptr;
    click(button, nullptr);
    context(button, contextArgs->Event());
    Require(fixture::launches.size() == 2 && shell.browser->navigated.size() == 1,
            "missing folder sender/window performed navigation");
    bar.state->suppressClick = scratch.directory;
    click(button, nullptr);
    Require(bar.state->suppressClick.empty(), "observed Click did not consume drag suppression");

    auto pointerArgs = winrt::make_self<pointer_fixture::Args>();
    pointerArgs->point->properties->left = true;
    muxc::StackPanel orphan;
    auto weakOrphan = winrt::make_weak(orphan);
    auto detached = MakeLabelButton(L"detached");
    AttachFolderButtonHandlers(*bar.state, detached, scratch.directory, weakOrphan, false);
    std::vector<muxi::PointerEventHandler> pointers;
    for (const auto& event : {mux::UIElement::PointerPressedEvent(), mux::UIElement::PointerMovedEvent(),
                            mux::UIElement::PointerReleasedEvent(), mux::UIElement::PointerCanceledEvent()}) {
        auto handler = pointer_observer::Handler(detached, event);
        Require(static_cast<bool>(handler), "detached original pointer handler missing");
        pointers.push_back(handler);
        handler(detached, pointerArgs->Event()); // Live panel lacking a tracked state.
    }
    orphan = nullptr;
    Require(!weakOrphan.get(), "weak pointer panel fixture did not expire");
    for (auto& handler : pointers) handler(detached, pointerArgs->Event());
    for (const auto& event : {mux::UIElement::PointerPressedEvent(), mux::UIElement::PointerMovedEvent(),
                            mux::UIElement::PointerReleasedEvent(), mux::UIElement::PointerCanceledEvent()}) {
        auto handler = pointer_observer::Handler(button, event);
        handler(nullptr, pointerArgs->Event());
    }
    auto weakPanel = bar.state->buttons;
    bar.state->buttons = nullptr;
    for (const auto& event : {mux::UIElement::PointerPressedEvent(), mux::UIElement::PointerMovedEvent(),
                            mux::UIElement::PointerReleasedEvent(), mux::UIElement::PointerCanceledEvent()}) {
        pointer_observer::Handler(button, event)(button, pointerArgs->Event());
    }
    bar.state->buttons = weakPanel;
    Require(bar.state->dragPath.empty() && ReadStorageLocked() == scratch.directory,
            "missing-state/sender/expired-panel pointer guards changed drag or storage");
    event_gap::ResetObservers();
    std::puts("PASS: observed folder Click/Ctrl+Click/context actions, unload, absent sender/window and weak pointer guards");
}

void CheckObservedRecentMenuCallbacks() {
    fixture::Reset();
    event_gap::ResetObservers();
    BackupFixture scratch;
    ShellFixture shell;
    fixture::activeWindow = shell.frame;
    event_gap::ConfigureRecents();
    XamlBarFixture bar;
    bar.state->recentFolders = {scratch.directory, L"\\\\event.invalid\\other"};
    auto anchor = MakeLabelButton(L"RC");
    auto weakAnchor = winrt::make_weak(anchor);
    muxc::MenuFlyout menu;
    FillRecentMenu(*bar.state, menu, weakAnchor);
    auto folderItem = menu.Items().GetAt(0).as<muxc::MenuFlyoutItem>();
    auto folderClick = event_gap::Click(folderItem);
    folderClick(folderItem, nullptr);
    Require(shell.browser->navigated == std::vector<std::wstring>{scratch.directory},
            "observed recent menu Click did not navigate folder");
    g_unloading = true;
    const auto writes = fixture::writes;
    folderClick(folderItem, nullptr);
    Require(fixture::writes == writes && shell.browser->navigated.size() == 1,
            "unloading recent folder delegate changed state");
    g_unloading = false;
    anchor = nullptr;
    Require(!weakAnchor.get(), "weak recent menu anchor did not expire");
    folderClick(folderItem, nullptr);
    Require(shell.browser->navigated.size() == 1, "expired recent anchor navigated");
    event_gap::ResetObservers();
    std::puts("PASS: original recent menu folder delegate, unload and expired anchor without popup activation");
}

void CheckObservedRecentOpeningAndClear() {
    fixture::Reset();
    event_gap::ResetObservers();
    ShellFixture shell;
    event_gap::ConfigureRecents();
    XamlBarFixture bar;
    const std::wstring recent = L"\\\\event.invalid\\recent";
    RecordExplorerRecent(recent);
    RefreshPanel(bar.panel);
    auto rc = bar.recents.Children().GetAt(bar.recents.Children().Size() - 1).as<muxc::Button>();
    auto menu = rc.Flyout().as<muxc::MenuFlyout>();
    auto opening = event_gap::Opening(menu);
    opening(menu, nullptr);
    Require(menu.Items().Size() >= 1 &&
            menu.Items().GetAt(0).as<muxc::MenuFlyoutItem>().Text() != L"No recent folders",
            "observed RC Opening did not populate menu");
    auto items = menu.Items().Size();
    opening(nullptr, nullptr);
    g_unloading = true;
    opening(menu, nullptr);
    g_unloading = false;
    auto weakPanel = bar.state->buttons;
    bar.state->buttons = nullptr;
    opening(menu, nullptr);
    bar.state->buttons = weakPanel;
    Require(menu.Items().Size() == items, "guarded RC Opening changed menu without usable state");
    auto clearContext = event_gap::Context(rc);
    auto context = winrt::make_self<event_gap::ContextArgs>();
    clearContext(rc, context->Event());
    Require(context->handled && JoinLines(g_recents).empty(),
            "observed RC context-clear failed");

    event_gap::ConfigureRecents(false);
    RecordExplorerRecent(recent);
    RefreshPanel(bar.panel);
    rc = bar.recents.Children().GetAt(bar.recents.Children().Size() - 1).as<muxc::Button>();
    auto clearMenu = rc.ContextFlyout().as<muxc::MenuFlyout>();
    auto clearItem = clearMenu.Items().GetAt(0).as<muxc::MenuFlyoutItem>();
    auto clearClick = event_gap::Click(clearItem);
    g_unloading = true;
    clearClick(clearItem, nullptr);
    g_unloading = false;
    Require(!JoinLines(g_recents).empty(), "unloading RC clear Click erased history");
    clearClick(clearItem, nullptr);
    Require(JoinLines(g_recents).empty(), "observed context-menu clear Click failed");

    muxc::StackPanel orphan;
    BarState isolated;
    muxc::StackPanel recents;
    auto weakOrphan = winrt::make_weak(orphan);
    AppendRecents(isolated, recents, weakOrphan);
    auto detachedRc = recents.Children().GetAt(recents.Children().Size() - 1).as<muxc::Button>();
    auto detachedMenu = detachedRc.Flyout().as<muxc::MenuFlyout>();
    auto detachedOpening = event_gap::Opening(detachedMenu);
    auto detachedClear = detachedRc.ContextFlyout().as<muxc::MenuFlyout>().Items().GetAt(0)
                           .as<muxc::MenuFlyoutItem>();
    auto detachedClick = event_gap::Click(detachedClear);
    orphan = nullptr;
    Require(!weakOrphan.get(), "detached RC panel did not expire");
    RecordExplorerRecent(recent);
    const auto retainedHistory = JoinLines(g_recents);
    detachedOpening(detachedMenu, nullptr);
    detachedClick(detachedClear, nullptr);
    Require(detachedMenu.Items().Size() == 1 && JoinLines(g_recents) == retainedHistory,
            "expired RC panel repopulated menu or cleared history");
    RevokeHandlers(isolated.recentHandlers);
    RevokeHandlers(isolated.recentMenuHandlers);
    event_gap::ResetObservers();
    std::puts("PASS: observed RC Opening/context clear/menu clear, unload and missing/expired panel guards");
}

void CheckObservedBackupAndFxFailureCallbacks() {
    fixture::Reset();
    event_gap::ResetObservers();
    BackupFixture scratch;
    ShellFixture shell;
    fixture::activeWindow = shell.frame;
    fixture::creation = DialogCreation;
    fakeDialog = NewFake<FakeDialog>();
    fakeDialog->item = NewFake<FakeItem>(scratch.file).as<IShellItem>();
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture bar;
    RefreshPanel(bar.panel);
    auto buttons = BarButtons(bar.panel);
    auto add = buttons[0];
    auto addClick = event_gap::Click(add);
    auto backupMenu = add.ContextFlyout().as<muxc::MenuFlyout>();
    auto save = backupMenu.Items().GetAt(0).as<muxc::MenuFlyoutItem>();
    auto load = backupMenu.Items().GetAt(1).as<muxc::MenuFlyoutItem>();
    auto saveClick = event_gap::Click(save);
    auto loadClick = event_gap::Click(load);
    g_unloading = true;
    const auto shows = fakeDialog->shows;
    saveClick(save, nullptr);
    loadClick(load, nullptr);
    addClick(add, nullptr);
    g_unloading = false;
    Require(fakeDialog->shows == shows && ReadStorageLocked().empty(),
            "unloading backup/add delegates performed work");
    addClick(nullptr, nullptr);
    fixture::activeWindow = nullptr;
    addClick(add, nullptr);
    Require(ReadStorageLocked().empty(), "add without sender/window changed storage");
    fixture::activeWindow = shell.frame;
    const auto writesBeforeCancelled = fixture::writes;
    fakeDialog->showResult = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    saveClick(save, nullptr);
    loadClick(load, nullptr);
    Require(fixture::writes == writesBeforeCancelled && ReadStorageLocked().empty() &&
            GetFileAttributesW(scratch.file.c_str()) == INVALID_FILE_ATTRIBUTES,
            "cancelled backup delegates wrote files or changed bookmark storage");
    fakeDialog->showResult = S_OK;
    auto normalDialog = fakeDialog;
    fakeDialog.attach(new event_gap::UnloadingDialog());
    fakeDialog->item = normalDialog->item;
    saveClick(save, nullptr);
    g_unloading = false;
    loadClick(load, nullptr);
    g_unloading = false;
    Require(GetFileAttributesW(scratch.file.c_str()) == INVALID_FILE_ATTRIBUTES,
            "unload after file selection did not stop backup callbacks");
    fakeDialog = normalDialog;
    {
        platform_failure::Scope fault;
        platform_failure::faults.open = true;
        saveClick(save, nullptr);
    }
    auto saveTip = winrt::unbox_value<winrt::hstring>(muxc::ToolTipService::GetToolTip(save));
    Require(std::wstring_view(saveTip).starts_with(L"Save failed:"),
            "failed observed Save did not update tooltip");
    loadClick(load, nullptr); // Missing backup.
    auto loadTip = winrt::unbox_value<winrt::hstring>(muxc::ToolTipService::GetToolTip(load));
    Require(std::wstring_view(loadTip).starts_with(L"Load failed:"),
            "failed observed Load did not update tooltip");
    saveClick(nullptr, nullptr); // Successful I/O with sender which cannot accept tooltip.
    Require(GetFileAttributesW(scratch.file.c_str()) != INVALID_FILE_ATTRIBUTES,
            "sender-less observed Save did not preserve I/O behavior");
    loadClick(nullptr, nullptr);
    auto weakPanel = bar.state->buttons;
    bar.state->buttons = nullptr;
    loadClick(load, nullptr); // Valid backup but live panel no longer tracked.
    bar.state->buttons = weakPanel;

    auto fx = buttons[1];
    auto folders = fx.Flyout().as<muxc::MenuFlyout>();
    auto drives = fx.ContextFlyout().as<muxc::MenuFlyout>();
    auto foldersOpening = event_gap::Opening(folders);
    auto drivesOpening = event_gap::Opening(drives);
    const auto initialFolderCount = folders.Items().Size();
    const auto initialDriveCount = drives.Items().Size();
    g_unloading = true;
    foldersOpening(folders, nullptr);
    drivesOpening(drives, nullptr);
    g_unloading = false;
    Require(folders.Items().Size() == initialFolderCount && drives.Items().Size() == initialDriveCount,
            "unloading FX Opening changed menu contents");
    foldersOpening(winrt::box_value(1), nullptr); // GuardHandler contains failed sender.as.
    drivesOpening(nullptr, nullptr);
    Require(!g_uiCallbackDepth && folders.Items().Size() == initialFolderCount &&
            drives.Items().Size() == initialDriveCount,
            "invalid FX Opening sender escaped guard or changed menus");
    {
        platform_failure::Scope fault;
        platform_failure::faults.knownFolder = true;
        platform_failure::faults.knownFolderResult = E_FAIL;
        foldersOpening(folders, nullptr);
        Require(folders.Items().Size() == 1 && !folders.Items().GetAt(0).IsEnabled(),
                "FX known-folder failures did not create disabled placeholder");
        g_fxFolders = {{L"valid", scratch.directory}, {L"missing", scratch.directory + L"\\absent"}};
        foldersOpening(folders, nullptr);
        Require(folders.Items().Size() == 2, "FX did not skip missing custom folder");
    }
    auto folderItem = folders.Items().GetAt(1).as<muxc::MenuFlyoutItem>();
    auto folderClick = event_gap::Click(folderItem);
    const auto navigations = shell.browser->navigated.size();
    folderClick(folderItem, nullptr);
    Require(shell.browser->navigated.size() == navigations + 1, "observed FX location Click failed");
    g_unloading = true;
    folderClick(folderItem, nullptr);
    g_unloading = false;
    Require(shell.browser->navigated.size() == navigations + 1,
            "unloading FX location Click navigated");
    {
        event_gap_platform::Scope scope;
        event_gap_platform::faults.logicalMask = 0; // Documented GetLogicalDrives failure result.
        drivesOpening(drives, nullptr);
        Require(event_gap_platform::faults.logicalCalls == 1 && drives.Items().Size() == 1 &&
                drives.Items().GetAt(0).as<muxc::MenuFlyoutItem>().Text() == L"No drives available" &&
                !drives.Items().GetAt(0).IsEnabled() && bar.state->driveHandlers.empty(),
                "zero logical-drive mask did not replace commands with disabled explanation");
    }
    {
        event_gap_platform::Scope scope;
        platform_failure::Scope platformScope;
        platform_failure::faults.volume = true; // No real probe of the synthetic root.
        const std::wstring missing = L"Y:\\";
        const std::wstring available = L"Z:\\";
        event_gap_platform::faults.logicalMask = (DWORD{1} << 24) | (DWORD{1} << 25);
        fixture::drives[missing] = DRIVE_FIXED;
        fixture::drives[available] = DRIVE_FIXED;
        event_gap_platform::faults.attributes[missing] = {INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND};
        event_gap_platform::faults.attributes[available] = {FILE_ATTRIBUTE_DIRECTORY, ERROR_SUCCESS};
        drivesOpening(drives, nullptr);
        Require(event_gap_platform::faults.logicalCalls == 1 &&
                event_gap_platform::faults.attributeQueries == std::vector<std::wstring>{missing, available} &&
                drives.Items().Size() == 1 && drives.Items().GetAt(0).IsEnabled() &&
                bar.state->driveHandlers.size() == 1,
                "drive menu did not probe advertised fixed drives or filter missing root");
        auto driveItem = drives.Items().GetAt(0).as<muxc::MenuFlyoutItem>();
        Require(winrt::unbox_value<winrt::hstring>(muxc::ToolTipService::GetToolTip(driveItem)) == available,
                "remaining drive command did not retain available root");
        const auto launches = fixture::launches.size();
        fixture::keys[VK_CONTROL] = static_cast<SHORT>(0x8000);
        event_gap::Click(driveItem)(driveItem, nullptr);
        fixture::keys[VK_CONTROL] = 0;
        Require(fixture::launches.size() == launches + 1 &&
                fixture::launches.back() == std::pair<std::wstring, std::wstring>{L"opennewtab", available},
                "available drive command did not invoke original navigation with retained root");
        event_gap_platform::faults.attributes[available] = {INVALID_FILE_ATTRIBUTES, ERROR_PATH_NOT_FOUND};
        drivesOpening(drives, nullptr);
        Require(drives.Items().Size() == 1 && !drives.Items().GetAt(0).IsEnabled() &&
                bar.state->driveHandlers.empty(),
                "all advertised drives missing did not revoke commands and show placeholder");
    }
    // Remove the anchor while preserving captured delegates and the owning state.
    buttons.clear();
    bar.panel.Children().Clear();
    auto weakFx = winrt::make_weak(fx);
    fx = nullptr;
    Require(!weakFx.get(), "FX anchor did not expire after visual removal");
    const auto beforeMissingAnchor = shell.browser->navigated.size();
    folderClick(folderItem, nullptr);
    Require(shell.browser->navigated.size() == beforeMissingAnchor,
            "expired FX anchor navigated");
    shell.folder->persist = NewFake<FakePersist>(scratch.directory).as<IPersistFolder2>();
    bar.strip.Content(nullptr);
    bar.panel = nullptr;
    loadClick(load, nullptr); // Successful load whose actual captured panel has expired.
    addClick(add, nullptr); // Successful add cannot refresh its expired captured panel.
    Require(ReadStorageLocked() == scratch.directory,
            "expired add panel lost successful bookmark insertion");
    fakeDialog = nullptr;
    event_gap::ResetObservers();
    std::puts("PASS: observed backup/add failure and unload callbacks, FX Opening/location guards and missing anchors");
}

void CheckObservedInstalledRootDelegates() {
    fixture::Reset();
    event_gap::ResetObservers();
    LoadSettings();
    UpdateThreadSettings();
    SaveBookmarksLocked({L"\\\\event.invalid\\old"});
    event_gap::InstalledBar bar;
    auto root = bar.state->barRoot.get();
    auto panel = bar.state->buttons.get();
    auto strip = bar.state->strip.get();
    auto entered = event_observer::Find(root, L"PointerEntered").pointer;
    auto over = event_observer::Find(root, L"DragOver").drag;
    auto leave = event_observer::Find(root, L"DragLeave").drag;
    auto drop = event_observer::Find(root, L"Drop").drag;
    Require(entered && over && leave && drop, "installed original root callbacks not observed");
    SaveBookmarksLocked({L"\\\\event.invalid\\old", L"\\\\event.invalid\\new"});
    entered(root, nullptr);
    Require(bar.state->folderButtons.size() == 2, "root PointerEntered did not refresh bookmarks");
    drop_fixture::Request request(strip);
    over(root, request.Event());
    Require(request.args->accepted == wdt::DataPackageOperation::Link && bar.state->dropTarget.get(),
            "root DragOver did not accept folders or create insertion marker");
    leave(root, request.Event());
    Require(!bar.state->dropTarget.get(), "root DragLeave did not clear insertion marker");
    request.read->SetItems({drop_fixture::Folder(L"\\\\event.invalid\\drop")});
    request.read->status = drop_fixture::wf::AsyncStatus::Completed;
    drop(root, request.Event());
    Require(request.read->callbacks == 1 && request.read->resultReads == 1 && !g_pendingDrops &&
            ReadStorageLocked().find(L"\\\\event.invalid\\drop") != std::wstring::npos,
            "root Drop did not execute real asynchronous insertion callback");
    auto trackedPanel = bar.state->buttons;
    bar.state->buttons = nullptr;
    leave(root, request.Event()); // A live panel which no longer resolves to state.
    bar.state->buttons = trackedPanel;
    // Expire buttons before revocation. Captured callbacks own only weak refs.
    auto weakPanel = winrt::make_weak(panel);
    strip.Content(nullptr);
    panel = nullptr;
    Require(!weakPanel.get(), "installed root panel failed to expire before revocation");
    const auto reads = request.view->reads;
    entered(root, nullptr);
    over(root, request.Event());
    leave(root, request.Event());
    drop(root, request.Event());
    Require(request.view->reads == reads, "expired root panel started a drop read");
    event_gap::ResetObservers();
    std::puts("PASS: original installed root PointerEntered/DragOver/DragLeave/Drop delegates and expired weak panel guards");
}

void CheckObservedEventGaps() {
    CheckObservedFolderActionsAndWeakGuards();
    CheckObservedRecentMenuCallbacks();
    CheckObservedRecentOpeningAndClear();
    CheckObservedBackupAndFxFailureCallbacks();
    CheckObservedInstalledRootDelegates(); // Close the hidden Window after detached-tree checks.
    event_gap::ResetObservers();
    g_iconCache.clear();
}
