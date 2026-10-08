#pragma once
#include "xaml-fixture.h"
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Microsoft.UI.Xaml.Automation.Peers.h>
#include <winrt/Microsoft.UI.Xaml.Automation.Provider.h>

void CheckXamlReflowAndDropMarkers() {
    XamlBarFixture fixture;
    auto& state = *fixture.state;
    muxc::StackPanel row;
    fixture.panel.Children().Append(row);
    std::vector<muxc::Button> buttons;
    for (unsigned i = 0; i < 8; ++i) {
        auto button = MakeLabelButton(L"T");
        button.Width(50);
        button.Margin({0, 0, 0, 0});
        buttons.push_back(button);
        row.Children().Append(button);
    }
    ReflowPanel(fixture.panel, 110);
    Require(state.rowCount == 4 && fixture.panel.Children().Size() == 4 &&
            BarButtons(fixture.panel) == buttons && !state.reflowing,
            "XAML reflow did not preserve buttons across four rows");
    ReflowPanel(fixture.panel, 1000);
    Require(state.rowCount == 1 && fixture.panel.Children().Size() == 1 &&
            fixture.strip.Height() == RowAllocation(1), "XAML reflow did not collapse to one row");
    ReflowPanel(fixture.panel, 1000);
    ReflowPanel(fixture.panel, 0);
    ReflowPanel(fixture.panel, std::numeric_limits<double>::quiet_NaN());
    Require(state.rowCount == 1 && state.lastLayoutWidth == 1000,
            "invalid or unchanged width changed layout");
    state.dragPath = L"C:\\drag";
    ReflowPanel(fixture.panel, 110);
    Require(state.rowCount == 1, "active drag was reparented by reflow");
    ClearDrag(state);
    SetBarRowCount(state, 99);
    Require(state.rowCount == 4 && fixture.root.Height() == RowAllocation(4) &&
            state.addedRow.Height().Value == RowAllocation(4) &&
            fixture.grid.MinHeight() == RowAllocation(4), "row clamp or host height update failed");
    fixture.grid.MinHeight(999);
    fixture.nav.MinHeight(888);
    SetBarRowCount(state, 0);
    Require(state.rowCount == 1 && fixture.grid.MinHeight() == 999 && fixture.nav.MinHeight() == 888,
            "row update overwrote external minimum height changes");
    ShowInsertionMark(state, fixture.panel, 0);
    Require(state.dropTarget.get() == buttons[2] && !state.dropAfter &&
            buttons[2].BorderThickness().Left == 3, "leading insertion mark targeted fixed buttons");
    ShowInsertionMark(state, fixture.panel, 0);
    ShowInsertionMark(state, fixture.panel, 999);
    Require(state.dropTarget.get() == buttons.back() && state.dropAfter &&
            buttons.back().BorderThickness().Right == 3, "append marker not on last bookmark");
    ClearDropTarget(state);
    Require(!state.dropTarget.get() && !state.dropAfter,
            "drop marker not cleared");
    fixture.panel.Measure({1000, 1000});
    fixture.panel.Arrange({0, 0, 1000, 1000});
    Require(BookmarkDropIndex(fixture.panel, {-10, 0}) == 0 &&
            BookmarkDropIndex(fixture.panel, {10000, 0}) == 6,
            "drop index did not exclude fixed buttons or clamp to last bookmark");
    muxc::StackPanel empty;
    Require(BookmarkDropIndex(empty, {0, 0}) == 0, "empty panel drop index failed");
    muxc::Grid navigation;
    navigation.Name(L"NavigationBarControlGrid");
    Require(FindNavigationGrid(navigation, 0) == navigation &&
            !FindNavigationGrid(navigation, 65) && !FindNavigationGrid(nullptr, 0),
            "navigation grid search ignored name, null or depth bound");
    std::puts("PASS: real XAML reflow, row heights, external sizing and bookmark drop markers");
}

void CheckXamlRecentFitting() {
    XamlBarFixture fixture;
    auto& state = *fixture.state;
    Require(FitRecents(state, 100, {50, 50}) == 0, "empty recents consumed space");
    auto append = [&](double width) {
        muxc::Border element;
        element.Width(width);
        element.Height(20);
        fixture.recents.Children().Append(element);
    };
    append(10);
    append(20);
    append(20);
    append(20);
    state.recentButtonCount = 2;
    state.hasRecentMenuButton = true;
    Require(FitRecents(state, 200, {50, 50}) == 70 && state.visibleRecents == 2,
            "spacious recent column hid buttons");
    Require(FitRecents(state, 150, {50, 50}) == 30 && state.visibleRecents == 0,
            "RC did not fold buttons to preserve bookmark row count");
    fixture.recents.Children().RemoveAtEnd();
    state.hasRecentMenuButton = false;
    Require(FitRecents(state, 150, {50, 50}) == 50 && state.visibleRecents == 2,
            "recent buttons without RC did not fit exact width");
    Require(FitRecents(state, 130, {50, 50}) == 30 && state.visibleRecents == 1 &&
            fixture.recents.Children().GetAt(1).as<mux::FrameworkElement>().Visibility() == mux::Visibility::Collapsed,
            "without RC oldest button was not hidden first");
    Require(FitRecents(state, 100, {50, 50}) == 0 && state.visibleRecents == 0 &&
            fixture.recents.Children().GetAt(0).as<mux::FrameworkElement>().Visibility() == mux::Visibility::Collapsed,
            "empty recent column retained separator");
    state.recentButtonCount = 99;
    Require(FitRecents(state, 100, {50, 50}) == 0, "malformed recent child count consumed width");
    std::puts("PASS: real XAML recent folding, oldest-first hiding and separator visibility");
}

void CheckXamlPanelRefresh() {
    fixture::Reset();
    BackupFixture scratch;
    fixture::integers[L"recent.enabled"] = 1;
    fixture::integers[L"recent.buttons"] = 2;
    fixture::integers[L"recent.history"] = 10;
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture fixture;
    auto& state = *fixture.state;
    const auto missing = scratch.directory + L"\\missing";
    SaveBookmarksLocked({scratch.directory, missing});
    RecordExplorerRecent(L"\\\\test.invalid\\recent1");
    RefreshPanel(fixture.panel);
    Require(state.renderComplete && state.folderButtons.size() == 2 &&
            BarButtons(fixture.panel).size() == 4 && state.recentButtonCount == 1,
            "real XAML bookmark panel did not finish initial render");
    const auto original = BarButtons(fixture.panel);
    RefreshPanel(fixture.panel);
    Require(BarButtons(fixture.panel) == original,
            "unchanged panel replaced existing bookmark buttons");
    RecordExplorerRecent(L"\\\\test.invalid\\recent2");
    RefreshPanel(fixture.panel);
    Require(BarButtons(fixture.panel) == original && state.recentButtonCount == 2,
            "recent-only update rebuilt bookmarks or omitted new recent button");
    Require(CreateDirectoryW(missing.c_str(), nullptr), "restored folder setup failed");
    state.lastFolderCheckAt = 0;
    RefreshPanel(fixture.panel);
    Require(BarButtons(fixture.panel) == original &&
            state.folderButtons[1].status == FolderStatus::Available,
            "hover status refresh did not retain button while restoring folder");
    Require(RemoveDirectoryW(missing.c_str()), "restored folder cleanup failed");
    state.lastFolderCheckAt = 0;
    RefreshPanel(fixture.panel);
    Require(state.folderButtons[1].status == FolderStatus::Missing,
            "hover failed to mark removed local folder missing");
    state.dragPath = scratch.directory;
    RefreshPanel(fixture.panel);
    Require(state.refreshAfterDrag, "refresh during drag was not deferred");
    ClearDrag(state);
    muxc::MenuFlyout menu;
    auto anchor = MakeLabelButton(L"RC");
    state.visibleRecents = 0;
    FillRecentMenu(state, menu, winrt::make_weak(anchor));
    Require(menu.Items().Size() == state.recentFolders.size(),
            "recent menu omitted folders");
    state.visibleRecents = state.recentFolders.size();
    FillRecentMenu(state, menu, winrt::make_weak(anchor));
    Require(menu.Items().Size() == 1 && !menu.Items().GetAt(0).IsEnabled(),
            "all-visible recent menu missing disabled placeholder");
    fixture::integers[L"recent.enabled"] = 0;
    LoadSettings();
    RefreshPanel(fixture.panel);
    Require(state.renderComplete && fixture.recents.Children().Size() == 0,
            "disabled recents left controls behind");
    Require(!state.panelHandlers.empty(), "panel handlers not registered");
    RemoveBarVisuals(state);
    Require(state.panelHandlers.empty() && state.recentHandlers.empty() &&
            state.recentMenuHandlers.empty() && fixture.grid.Children().Size() == 0,
            "XAML teardown retained handlers or root");
    fixture::Reset();
    std::puts("PASS: real XAML panel construction, incremental refresh, recent menu and teardown");
}

void PumpXamlMessages(unsigned milliseconds = 100) {
    const auto deadline = GetTickCount64() + milliseconds;
    const auto wake = SetTimer(nullptr, 0, 10, nullptr);
    Require(wake != 0, "XAML pump wake timer failed");
    do {
        MSG message;
        if (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    } while (GetTickCount64() < deadline);
    KillTimer(nullptr, wake);
}

void CheckXamlCommandBarInstallation() {
    fixture::Reset();
    LoadSettings();
    UpdateThreadSettings();
    if (!g_bars) g_bars.emplace();
    g_threadClosing = false;
    g_frameHooked = true; // Enable only the insertion guard; no native hook is installed.
    ExplorerHeaderFixture header;
    auto host = header.host;
    auto navigation = header.navigation;
    auto command = header.command;
    muxc::CommandBar ignored;
    TrackCommandBar(ignored);
    TrackCommandBar(nullptr);
    Require(g_bars->empty(), "unrelated or null command bar tracked");
    TrackCommandBar(command);
    Require(g_bars->size() == 1 && !g_bars->front().strip.get(),
            "detached command bar created visuals before XamlRoot existed");
    g_frameHooked = false;
    mux::Window window;
    window.Content(header.root);
    window.AppWindow().Resize({800, 400});
    window.AppWindow().Show();
    window.Activate();
    PumpXamlMessages(300);
    host.UpdateLayout();
    const auto baselineNavTop = header.nav.TransformToVisual(host).TransformPoint({0, 0}).Y;
    const auto baselineGridTop = navigation.TransformToVisual(host).TransformPoint({0, 0}).Y;
    Require(std::fabs(host.ActualHeight() - 134) < 0.5 &&
            std::fabs(header.tabs.ActualHeight() - 38) < 0.5 &&
            std::fabs(header.nav.ActualHeight() - 48) < 0.5 &&
            std::fabs(command.ActualHeight() - 48) < 0.5 &&
            std::fabs(navigation.ActualHeight() - 54) < 0.5 &&
            std::fabs(baselineNavTop - baselineGridTop - 3) < 0.5,
            "stock fixture does not reproduce documented header geometry");
    host.RowDefinitions().GetAt(0).Height({1, mux::GridUnitType::Star});
    g_frameHooked = true;
    TryInstallBar(command);
    Require(!g_bars->front().strip.get(), "unknown header row geometry was modified");
    host.RowDefinitions().GetAt(0).Height({0, mux::GridUnitType::Auto});
    TryInstallBar(command);
    Require(g_bars->front().strip.get() && g_bars->front().renderComplete &&
            navigation.RowDefinitions().Size() == 2 &&
            host.RowDefinitions().GetAt(2).Height().GridUnitType == mux::GridUnitType::Auto,
            "valid Explorer-like header did not install and render bar");
    PumpXamlMessages();
    const auto root = g_bars->front().barRoot.get();
    // The native sizing hook isn't installed: explicitly supply its confirmed
    // 134 + (40*N + 4) XAML-unit host allocation, then measure real WinUI layout.
    for (unsigned count : {1u, 2u, 3u, 4u, 1u}) {
        SetBarRowCount(g_bars->front(), count);
        host.Height(134 + 40 * count + 4);
        host.UpdateLayout();
        const auto navTop = header.nav.TransformToVisual(host).TransformPoint({0, 0}).Y;
        const auto strip = g_bars->front().strip.get();
        const auto stripTop = strip.TransformToVisual(host).TransformPoint({0, 0}).Y;
        const auto commandTop = command.TransformToVisual(host).TransformPoint({0, 0}).Y;
        Require(std::fabs(header.tabs.ActualHeight() - 38) < 0.5 &&
                std::fabs(command.ActualHeight() - 48) < 0.5 &&
                std::fabs(host.RowDefinitions().GetAt(2).ActualHeight() - 48) < 0.5,
                "measured header changed tab or natural command height");
        Require(stripTop >= navTop - 0.5 &&
                stripTop + strip.ActualHeight() <= navTop + header.nav.ActualHeight() + 0.5 &&
                commandTop - (stripTop + strip.ActualHeight()) >= -0.5,
                "measured header clipped/overlapped bookmark strip");
        Require(std::fabs(strip.ActualHeight() - (40 * count - 2)) < 0.5,
                "measured header did not allocate requested bookmark rows");
    }
    TrackCommandBar(command);
    TryInstallBar(command);
    Require(g_bars->size() == 1 && g_bars->front().barRoot.get() == root &&
            navigation.RowDefinitions().Size() == 2,
            "repeat command bar tracking duplicated installed row");
    std::vector<muxc::CommandBar> found;
    CollectCommandBars(host, 0, found);
    Require(found == std::vector<muxc::CommandBar>{command},
            "visual-tree command bar discovery missed or duplicated bar");
    CollectCommandBars(nullptr, 0, found);
    CollectCommandBars(host, 65, found);
    Require(found.size() == 1, "null/depth-bound discovery added bars");
    ScanXamlRootForCommandBars(command);
    Require(g_bars->size() == 1, "XamlRoot scan duplicated tracked state");
    // Both the command bar and strip leave the live tree; Unloaded must undo
    // the inserted row and restore the command row's Star sizing.
    window.Content(nullptr);
    PumpXamlMessages();
    Require(!g_bars->front().strip.get() && navigation.RowDefinitions().Size() == 1 &&
            navigation.RowDefinitions().GetAt(0).Height().GridUnitType == mux::GridUnitType::Auto &&
            navigation.Children().Size() == 3 && navigation.MinHeight() == 0 && header.nav.MinHeight() == 0 &&
            host.RowDefinitions().GetAt(2).Height().GridUnitType == mux::GridUnitType::Star,
            "real XAML Unloaded left added rows or altered host sizing");
    // A valid reattached header must install through its real Loaded callback,
    // without an explicit TryInstallBar call from the test.
    host.Height(178);
    window.Content(header.root);
    PumpXamlMessages(300);
    Require(g_bars->size() == 1 && g_bars->front().strip.get() &&
            g_bars->front().renderComplete && navigation.RowDefinitions().Size() == 2 &&
            host.RowDefinitions().GetAt(2).Height().GridUnitType == mux::GridUnitType::Auto,
            "valid reattached header did not install through real Loaded callback");
    window.Content(nullptr);
    PumpXamlMessages();
    Require(!g_bars->front().strip.get() && navigation.RowDefinitions().Size() == 1 &&
            navigation.Children().Size() == 3 && navigation.MinHeight() == 0 &&
            header.nav.MinHeight() == 0 &&
            host.RowDefinitions().GetAt(2).Height().GridUnitType == mux::GridUnitType::Star,
            "second real XAML Unloaded failed to restore header state");
    window.Close();
    CleanupCurrentThread();
    CleanupCurrentThread();
    Require(!g_bars && !g_messageHook && !g_workTimer,
            "repeated thread cleanup retained XAML state or hook");
    g_bars.emplace();
    g_frameHooked = false;
    fixture::Reset();
    std::puts("PASS: real XAML loaded/unloaded installation, unknown-header guard, discovery and idempotent cleanup");
}

void CheckLiveWindowSizing() {
    fixture::Reset();
    fixture::integers[L"recent.enabled"] = 1;
    fixture::integers[L"recent.buttons"] = 2;
    fixture::integers[L"recent.history"] = 10;
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture fixture;
    auto& state = *fixture.state;
    std::vector<std::wstring> paths;
    for (unsigned i = 0; i < 6; ++i) paths.push_back(L"\\\\fixture.invalid\\Folder " + std::to_wstring(i));
    SaveBookmarksLocked(paths);
    RefreshPanel(fixture.panel);
    fixture.root.ColumnDefinitions().Append(muxc::ColumnDefinition());
    muxc::ColumnDefinition recentColumn;
    recentColumn.Width({0, mux::GridUnitType::Auto});
    fixture.root.ColumnDefinitions().Append(recentColumn);
    muxc::Grid::SetColumn(fixture.recents, 1);
    fixture.recents.Orientation(muxc::Orientation::Horizontal);
    fixture.recents.Height(kRowHeight);
    fixture.recents.VerticalAlignment(mux::VerticalAlignment::Top);
    fixture.root.Height(RowAllocation(1));
    fixture.strip.Height(RowAllocation(1));
    fixture.strip.HorizontalScrollBarVisibility(muxc::ScrollBarVisibility::Hidden);
    muxc::Grid::SetRow(fixture.root, 1);
    auto token = fixture.root.SizeChanged([&](const auto&, const mux::SizeChangedEventArgs& args) {
        ReflowPanel(fixture.panel, args.NewSize().Width);
    });
    mux::Window window;
    window.Content(fixture.grid);
    window.AppWindow().Resize({1000, 500});
    window.Activate();
    PumpXamlMessages();
    Require(state.rowCount == 1 && fixture.root.ActualWidth() > 900,
            "wide live window did not lay out one row");
    window.AppWindow().Resize({420, 500});
    PumpXamlMessages();
    Require(state.rowCount > 1 && fixture.root.ActualWidth() < 420 &&
            std::fabs(state.lastLayoutWidth - fixture.root.ActualWidth()) < 1,
            "live resize clipped a fixed-width bar instead of wrapping");
    const auto rc = fixture.recents.Children().GetAt(fixture.recents.Children().Size() - 1).as<muxc::Button>();
    const auto rcOrigin = rc.TransformToVisual(fixture.root).TransformPoint({0, 0});
    Require(std::fabs(rcOrigin.X + rc.ActualWidth() + rc.Margin().Right - fixture.root.ActualWidth()) < 1,
            "RC does not remain at live root's right edge");
    window.AppWindow().Resize({1000, 500});
    PumpXamlMessages();
    Require(state.rowCount == 1, "widening live window did not collapse wrapped rows");
    fixture.root.SizeChanged(token);
    window.Content(nullptr);
    window.Close();
    fixture::Reset();
    std::puts("PASS: live window resizing wraps/collapses rows and keeps RC at the right edge");
}

template<typename Element, typename Peer>
void InvokeControl(const Element& element) {
    Peer peer(element);
    auto invoke = peer.GetPattern(mux::Automation::Peers::PatternInterface::Invoke)
                      .template as<mux::Automation::Provider::IInvokeProvider>();
    invoke.Invoke();
    PumpXamlMessages(30);
}

void CheckXamlMenuActionsAndNotices() {
    BackupFixture scratch;
    ShellFixture shell;
    shell.folder->persist = NewFake<FakePersist>(scratch.directory).as<IPersistFolder2>();
    fixture::creation = DialogCreation;
    fixture::activeWindow = shell.frame;
    fakeDialog = NewFake<FakeDialog>();
    fakeDialog->item = NewFake<FakeItem>(scratch.file).as<IShellItem>();
    fixture::integers[L"recent.enabled"] = 1;
    fixture::integers[L"recent.buttons"] = 1;
    fixture::integers[L"recent.history"] = 10;
    fixture::strings[L"fxCustomFolders[0].path"] = scratch.directory;
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture fixture;
    RefreshPanel(fixture.panel);
    mux::Window window;
    window.Content(fixture.grid);
    window.AppWindow().Resize({900, 500});
    window.Activate();
    PumpXamlMessages();
    auto buttons = BarButtons(fixture.panel);
    auto add = buttons[0];
    InvokeControl<muxc::Button, mux::Automation::Peers::ButtonAutomationPeer>(add);
    Require(ReadStorageLocked() == scratch.directory,
            "real XAML add button did not bookmark active Shell folder");
    buttons = BarButtons(fixture.panel);
    const auto backupMenu = buttons[0].ContextFlyout().as<muxc::MenuFlyout>();
    auto save = backupMenu.Items().GetAt(0).as<muxc::MenuFlyoutItem>();
    InvokeControl<muxc::MenuFlyoutItem, mux::Automation::Peers::MenuFlyoutItemAutomationPeer>(save);
    Require(GetFileAttributesW(scratch.file.c_str()) != INVALID_FILE_ATTRIBUTES &&
            fakeDialog->shows == 1, "real XAML Save command did not export backup");
    fixture::storage[kBookmarksKey] = L"C:\\different";
    auto load = backupMenu.Items().GetAt(1).as<muxc::MenuFlyoutItem>();
    InvokeControl<muxc::MenuFlyoutItem, mux::Automation::Peers::MenuFlyoutItemAutomationPeer>(load);
    Require(ReadStorageLocked() == scratch.directory, "real XAML Load command did not restore backup");
    RefreshPanel(fixture.panel);
    buttons = BarButtons(fixture.panel);
    // A suppressed click following a drag must not navigate.
    fixture.state->suppressClick = scratch.directory;
    const auto browsed = shell.browser->navigated.size();
    InvokeControl<muxc::Button, mux::Automation::Peers::ButtonAutomationPeer>(buttons[2]);
    Require(fixture.state->suppressClick.empty() && shell.browser->navigated.size() == browsed,
            "drag-suppressed click navigated or retained suppression");
    const auto fx = buttons[1];
    auto foldersMenu = fx.Flyout().as<muxc::MenuFlyout>();
    foldersMenu.ShowAt(fx);
    PumpXamlMessages();
    Require(foldersMenu.Items().Size() > 1 && !fixture.state->fxMenuHandlers.empty(),
            "FX Opening did not populate standard/custom folders");
    foldersMenu.Hide();
    auto drivesMenu = fx.ContextFlyout().as<muxc::MenuFlyout>();
    drivesMenu.ShowAt(fx);
    PumpXamlMessages();
    Require(drivesMenu.Items().Size() > 0, "FX drive menu empty");
    drivesMenu.Hide();
    auto bookmark = buttons[2];
    fixture::keys[VK_CONTROL] = static_cast<SHORT>(0x8000);
    InvokeControl<muxc::Button, mux::Automation::Peers::ButtonAutomationPeer>(bookmark);
    Require(!fixture::launches.empty() && fixture::launches.back().first == L"opennewtab",
            "real XAML bookmark Ctrl+click did not request new tab");
    fixture::keys[VK_CONTROL] = 0;
    InvokeControl<muxc::Button, mux::Automation::Peers::ButtonAutomationPeer>(bookmark);
    Require(!shell.browser->navigated.empty(), "real XAML bookmark click did not navigate active tab");
    ShowBookmarkNotice({nullptr, BookmarkNotice::Full, 0, 1});
    PumpXamlMessages();
    Require(fixture.state->notice && fixture.state->notice.IsOpen() && g_noticeTimer &&
            fixture.state->noticeText.Text() == L"Bookmarks full (32 folders)",
            "full notice did not open in loaded XAML tree");
    ShowBookmarkNotice({nullptr, BookmarkNotice::Partial, 1, 2});
    Require(fixture.state->noticeText.Text() == L"Added 1 of 2: bookmarks full (32)",
            "already-open notice did not update text");
    NoticeTimerProc(nullptr, 0, g_noticeTimer, 0);
    PumpXamlMessages();
    Require(!fixture.state->notice.IsOpen() && !g_noticeTimer, "notice timer left popup open");
    RecordExplorerRecent(L"\\\\test.invalid\\menu-folder");
    RefreshPanel(fixture.panel);
    const auto rc = fixture.recents.Children().GetAt(fixture.recents.Children().Size() - 1).as<muxc::Button>();
    Require(rc.Content().as<muxc::TextBlock>().Text() == L"RC" && rc.Flyout(),
            "recent-menu fixture did not provide an RC flyout");
    auto recentMenu = rc.Flyout().as<muxc::MenuFlyout>();
    // Test population/callbacks directly; opening the flyout in the CBS
    // standalone host remains an explicit gap (see README). List the recent
    // folder in the menu rather than as a button so the menu gets a handler.
    fixture.state->visibleRecents = 0;
    FillRecentMenu(*fixture.state, recentMenu, winrt::make_weak(rc));
    Require(!fixture.state->recentMenuHandlers.empty() &&
            recentMenu.Items().Size() == fixture.state->recentFolders.size(),
            "RC menu population omitted folder handlers");
    auto clearMenu = rc.ContextFlyout().as<muxc::MenuFlyout>();
    auto clear = clearMenu.Items().GetAt(0).as<muxc::MenuFlyoutItem>();
    InvokeControl<muxc::MenuFlyoutItem, mux::Automation::Peers::MenuFlyoutItemAutomationPeer>(clear);
    Require(fixture::storage[kRecentsKey].empty(), "RC clear menu did not clear recent storage");
    fakeDialog->showResult = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    const auto shown = fakeDialog->shows;
    auto currentSave = BarButtons(fixture.panel)[0].ContextFlyout().as<muxc::MenuFlyout>()
                          .Items().GetAt(0).as<muxc::MenuFlyoutItem>();
    InvokeControl<muxc::MenuFlyoutItem, mux::Automation::Peers::MenuFlyoutItemAutomationPeer>(currentSave);
    Require(fakeDialog->shows == shown + 1 && !g_threadFileDialog && !g_activeOperations,
            "cancelled menu dialog was not shown or leaked active state");
    window.Content(nullptr);
    window.Close();
    fakeDialog = nullptr;
    std::puts("PASS: real XAML button/menu actions, backup commands, FX opening and transient notices");
}

void CheckXamlTextFocusAndHotkeys() {
    ShellFixture shell;
    fixture::integers[L"bookmarkHotkey"] = 1;
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture fixture;
    mux::Window window;
    window.Content(fixture.grid);
    window.Activate();
    PumpXamlMessages();
    Require(IsTextInputElement(muxc::TextBox()) && IsTextInputElement(muxc::RichEditBox()) &&
            IsTextInputElement(muxc::PasswordBox()) && IsTextInputElement(muxc::AutoSuggestBox()) &&
            !IsTextInputElement(muxc::Button()) && !IsTextInputElement(nullptr),
            "XAML editable-control recognition failed");
    std::vector<HWND> controls;
    for (const auto name : {L"Edit", L"RichEditFixture", L"DirectUIHWND", L"SysTreeView32"}) {
        auto control = CreateShellWindow(name, shell.frame);
        controls.push_back(control);
        fixture::focusWindow = control;
        Require(TextInputFocused(shell.frame) == (std::wstring_view(name) == L"Edit" ||
                    std::wstring_view(name) == L"RichEditFixture"),
                "native edit/rename versus file-list/tree recognition failed");
        Require(FileListFocused() == (std::wstring_view(name) == L"DirectUIHWND"),
                "native file-list focus recognition failed");
    }
    fixture::focusWindow = shell.frame;
    Require(!TextInputFocused(shell.frame) && FrameHasInstalledBar(shell.frame),
            "unknown XAML island fallback failed");
    auto key = [&](LPARAM previous = 0) {
        MSG message{};
        message.hwnd = shell.frame;
        message.message = WM_KEYDOWN;
        message.wParam = 'B';
        message.lParam = previous;
        ThreadMessageHook(HC_ACTION, PM_REMOVE, reinterpret_cast<LPARAM>(&message));
        return message;
    };
    fixture::keys[VK_CONTROL] = static_cast<SHORT>(0x8000);
    auto message = key();
    Require(message.message == WM_NULL && g_bookmarkTogglePending == shell.frame,
            "Ctrl+B not consumed/queued for installed bar");
    StopThreadWork();
    Require(key(1LL << 30).message == WM_KEYDOWN && !g_bookmarkTogglePending,
            "repeated Ctrl+B consumed");
    for (const auto modifier : {VK_SHIFT, VK_MENU, VK_LWIN, VK_RWIN}) {
        fixture::keys[modifier] = static_cast<SHORT>(0x8000);
        Require(key().message == WM_KEYDOWN && !g_bookmarkTogglePending,
                "modified Ctrl+B consumed");
        fixture::keys[modifier] = 0;
    }
    fixture::focusWindow = controls[0];
    Require(key().message == WM_KEYDOWN && !g_bookmarkTogglePending,
            "rename/text input Ctrl+B stolen");
    fixture::focusWindow = shell.frame;
    g_bookmarkHotkey = false;
    Require(key().message == WM_KEYDOWN, "disabled Ctrl+B swallowed");
    g_bookmarkHotkey = true;
    fixture::keys[VK_CONTROL] = 0;
    Require(key().message == WM_KEYDOWN, "plain B swallowed");
    fixture.state->barRoot = nullptr;
    fixture::keys[VK_CONTROL] = static_cast<SHORT>(0x8000);
    Require(key().message == WM_KEYDOWN, "frame without installed bar consumed Ctrl+B");
    fixture.state->barRoot = winrt::make_weak(fixture.root);
    g_unloading = true;
    Require(key().message == WM_KEYDOWN, "unloading hook consumed Ctrl+B");
    g_unloading = false;
    window.Content(nullptr);
    window.Close();
    fixture::focusWindow = nullptr;
    for (const auto control : controls) DestroyWindow(control);
    StopThreadWork();
    std::puts("PASS: editable XAML/native controls, Ctrl+B admission, modifiers, repeats and rename focus");
}

#include "drag-drop-checks.h"
#include "pointer-checks.h"
#include "xaml-gap-checks.h"
#include "event-gap-checks.h"
#include "bitmap-failure-checks.h"

void CheckXamlRuntime() {
    XamlRuntime runtime;
    CheckBitmapApiFailureRecovery();
    CheckXamlStructuralLayoutGaps();
    CheckXamlBitmapCacheGaps();
    CheckXamlFolderRefreshGaps();
    CheckXamlRefreshFailureRecovery();
    CheckXamlTeardownOwnershipGaps();
    CheckObservedFolderActionsAndWeakGuards();
    CheckObservedRecentMenuCallbacks();
    CheckObservedRecentOpeningAndClear();
    CheckObservedBackupAndFxFailureCallbacks();
    CheckXamlReflowAndDropMarkers();
    CheckXamlRecentFitting();
    CheckXamlPanelRefresh();
    // Exercise manually arranged trees before closing any top-level Window;
    // CBS stops layout for detached trees after the last Window is closed.
    CheckExternalDropContracts();
    CheckExternalDropCompletionAndFailures();
    CheckExternalDropLimitsAndRealPackage();
    CheckFolderPointerContracts();
    CheckRecentPointerPromotionAndMouseRecovery();
    CheckXamlCommandBarInstallation();
    CheckXamlInstallationDiscoveryGaps();
    CheckLiveWindowSizing();
    CheckXamlMenuActionsAndNotices();
    CheckXamlTextFocusAndHotkeys();
    CheckObservedInstalledRootDelegates();
    pointer_observer::Clear();
    event_observer::Clear();
    g_iconCache.clear();
    g_bars.reset();
}

void PreviewXamlRuntime() {
    XamlRuntime runtime;
    fixture::Reset();
    fixture::integers[L"recent.enabled"] = 1;
    fixture::integers[L"recent.buttons"] = 3;
    fixture::integers[L"recent.history"] = 10;
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture bar;
    std::vector<std::wstring> paths;
    for (const auto& name : {L"Projects", L"Documents", L"Screenshots", L"Research", L"Downloads", L"Archive"}) {
        paths.push_back(std::wstring(L"\\\\fixture.invalid\\") + name);
    }
    SaveBookmarksLocked(paths);
    RecordExplorerRecent(L"\\\\fixture.invalid\\Recent A");
    RecordExplorerRecent(L"\\\\fixture.invalid\\Recent B");
    RecordExplorerRecent(L"\\\\fixture.invalid\\Recent C");
    RefreshPanel(bar.panel);
    bar.root.ColumnDefinitions().Append(muxc::ColumnDefinition());
    muxc::ColumnDefinition recentColumn;
    recentColumn.Width({0, mux::GridUnitType::Auto});
    bar.root.ColumnDefinitions().Append(recentColumn);
    muxc::Grid::SetColumn(bar.recents, 1);
    bar.recents.Orientation(muxc::Orientation::Horizontal);
    bar.recents.Height(kRowHeight);
    bar.recents.VerticalAlignment(mux::VerticalAlignment::Top);
    bar.root.HorizontalAlignment(mux::HorizontalAlignment::Stretch);
    bar.grid.HorizontalAlignment(mux::HorizontalAlignment::Stretch);
    bar.root.Height(RowAllocation(1));
    bar.strip.Height(RowAllocation(1));
    muxc::Grid::SetRow(bar.root, 1);
    bar.strip.HorizontalScrollBarVisibility(muxc::ScrollBarVisibility::Hidden);
    bar.strip.VerticalScrollBarVisibility(muxc::ScrollBarVisibility::Hidden);
    // This preview displays the production visuals with fixture data. Navigation,
    // backup dialogs and edits are disabled; the width control below is local.
    g_unloading = true;
    muxc::Grid background;
    background.RequestedTheme(mux::ElementTheme::Light);
    background.Background(muxm::SolidColorBrush(winrt::Windows::UI::Color{255, 246, 246, 246}));
    muxc::StackPanel content;
    content.Margin({32, 28, 32, 28});
    content.VerticalAlignment(mux::VerticalAlignment::Top);
    background.Children().Append(content);
    muxc::TextBlock title;
    title.Text(L"Explorer bookmarks — XAML test stand");
    title.FontSize(24);
    title.Margin({0, 0, 0, 12});
    content.Children().Append(title);
    muxc::TextBlock description;
    description.Text(L"Real XAML controls · isolated fixture data · resize this window to test wrapping");
    description.Margin({0, 0, 0, 24});
    content.Children().Append(description);
    content.Children().Append(bar.grid);
    muxc::TextBlock widthLabel;
    widthLabel.Margin({0, 24, 0, 8});
    widthLabel.Text(L"Available width: waiting for layout");
    content.Children().Append(widthLabel);
    auto widthToken = bar.root.SizeChanged([&](const auto&, const mux::SizeChangedEventArgs& args) {
        ReflowPanel(bar.panel, args.NewSize().Width);
        widthLabel.Text(L"Available width: " + std::to_wstring(static_cast<int>(args.NewSize().Width)) +
                        L" px · rows: " + std::to_wstring(bar.state->rowCount) +
                        L" · recent buttons: " + std::to_wstring(bar.state->visibleRecents));
    });
    muxc::TextBlock note;
    note.Text(L"This is a standalone preview. Your Explorer and saved bookmarks are untouched.");
    note.Margin({0, 24, 0, 0});
    content.Children().Append(note);
    mux::Window window;
    window.Title(L"Bookmarks XAML test stand");
    window.Content(background);
    window.AppWindow().Resize({1100, 480});
    window.AppWindow().Move({200, 160});
    bool closed = false;
    auto closedToken = window.Closed([&](const auto&, const auto&) { closed = true; PostQuitMessage(0); });
    window.Activate();
    std::puts("PREVIEW: XAML stand is open; close its window to end the preview.");
    MSG message;
    while (!closed && GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    bar.root.SizeChanged(widthToken);
    window.Closed(closedToken);
    g_unloading = false;
}
