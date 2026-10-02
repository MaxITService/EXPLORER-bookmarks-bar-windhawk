#pragma once
// Include after platform-failure-checks.h and the helpers in xaml-checks.h.
// Detached-tree checks must precede the first top-level Window's closure.
#include "xaml-fixture.h"

namespace xaml_gap {
inline BYTE iconByte = 0x5a;
struct ChangingIconConverter : platform_failure::IconConverter {
    HRESULT STDMETHODCALLTYPE CopyPixels(const WICRect* rectangle, UINT stride, UINT count, BYTE* bytes) override {
        const auto result = platform_failure::IconConverter::CopyPixels(rectangle, stride, count, bytes);
        if (SUCCEEDED(result)) std::fill_n(bytes, count, iconByte);
        return result;
    }
};
struct ChangingIconFactory : platform_failure::IconFactory {
    HRESULT STDMETHODCALLTYPE CreateFormatConverter(IWICFormatConverter** result) override {
        const auto status = platform_failure::icon.converter;
        *result = SUCCEEDED(status) ? new ChangingIconConverter : nullptr;
        return status;
    }
};
inline HRESULT IconCreation(REFCLSID clsid, LPUNKNOWN outer, DWORD context, REFIID iid, LPVOID* out) {
    if (clsid != CLSID_WICImagingFactory) return realComCreation(clsid, outer, context, iid, out);
    *out = nullptr;
    if (platform_failure::icon.creation) return E_NOINTERFACE;
    auto factory = NewFake<ChangingIconFactory>();
    return factory->QueryInterface(iid, out);
}

inline muxc::Button SizedButton(double width) {
    auto button = MakeLabelButton(L"fixture");
    button.Width(width);
    button.Margin({0, 0, 0, 0});
    return button;
}

inline std::vector<BYTE> BitmapBytes(const muxmi::WriteableBitmap& bitmap) {
    auto buffer = bitmap.PixelBuffer();
    BYTE* bytes = nullptr;
    winrt::check_hresult(buffer.as<::Windows::Storage::Streams::IBufferByteAccess>()->Buffer(&bytes));
    Require(bytes != nullptr, "real XAML bitmap exposed no pixels");
    return {bytes, bytes + buffer.Length()};
}

struct IconScope {
    platform_failure::Scope platform;
    fixture::ComCreation previous = fixture::creation;
    IconScope() {
        platform_failure::icon = {};
        iconByte = 0x5a;
        platform_failure::faults.shellIcon = platform_failure::ShellIcon::OwnedIcon;
        fixture::creation = IconCreation;
        g_iconCache.clear();
    }
    ~IconScope() {
        g_iconCache.clear();
        fixture::creation = previous;
    }
};

inline IconCacheEntry& Cache(const std::wstring& path) {
    auto entry = std::find_if(g_iconCache.begin(), g_iconCache.end(),
        [&](const auto& item) { return SamePath(item.path, path); });
    Require(entry != g_iconCache.end(), "XAML icon fixture lost cache entry");
    return *entry;
}

inline void Expire(const std::wstring& path) {
    auto& entry = Cache(path);
    entry.loadedAt = GetTickCount64() - (entry.pixels ? kIconCacheLifetimeMs : kFailedIconCacheLifetimeMs);
}
}

void CheckXamlStructuralLayoutGaps() {
    XamlBarFixture bar;
    auto& state = *bar.state;
    muxc::Border unrelated;
    bar.panel.Children().Append(unrelated);
    muxc::StackPanel first;
    muxc::StackPanel second;
    muxc::Border nonbutton;
    first.Children().Append(nonbutton);
    std::vector<muxc::Button> expected;
    for (unsigned i = 0; i < 11; ++i) {
        auto button = xaml_gap::SizedButton(50);
        expected.push_back(button);
        (i < 4 ? first : second).Children().Append(button);
    }
    bar.panel.Children().Append(first);
    bar.panel.Children().Append(second);
    Require(BarButtons(bar.panel) == expected,
            "button discovery included malformed rows/nonbuttons or reordered controls");
    Require(BookmarkDropIndex(bar.panel, {10000, 0}) == 9 &&
            BookmarkDropIndex(bar.panel, {10000, static_cast<float>(kRowHeight + kInterRowGap)}) == 2,
            "drop indexing over malformed rows/nonbuttons lost the bookmark count or targeted the wrong row");
    ReflowPanel(bar.panel, 100);
    Require(state.rowCount == kMaxRows && BarButtons(bar.panel) == expected &&
            bar.panel.Children().GetAt(3).as<muxc::StackPanel>().Children().Size() == 5,
            "four-row overflow lost controls or exceeded the supported row count");
    const auto lastRow = bar.panel.Children().GetAt(3);
    for (const auto width : {-1.0, 0.999, 100001.0, std::numeric_limits<double>::infinity(),
                             -std::numeric_limits<double>::infinity()}) {
        ReflowPanel(bar.panel, width);
        Require(state.lastLayoutWidth == 100 && bar.panel.Children().GetAt(3) == lastRow,
                "invalid reflow width mutated the current rows");
    }
    state.reflowing = true;
    ReflowPanel(bar.panel, 1000);
    Require(state.reflowing && state.rowCount == kMaxRows,
            "nested reflow changed rows or stole the outer guard");
    state.reflowing = false;
    muxc::StackPanel detached;
    detached.Children().Append(xaml_gap::SizedButton(50));
    ReflowPanel(detached, 200);
    Require(detached.Children().Size() == 1 && detached.Children().GetAt(0).try_as<muxc::Button>(),
            "reflow modified a panel with no tracked state");
    bar.panel.Children().Clear();
    ReflowPanel(bar.panel, 100);
    Require(!state.reflowing && state.lastLayoutWidth == 100 && bar.panel.Children().Size() == 0,
            "empty tracked panel leaked its reflow guard");
    muxc::Border zero;
    zero.Width(0);
    zero.Height(0);
    Require(MeasuredWidth(zero) == 1, "zero-size element did not receive the safe measurement floor");

    // A malformed row must not displace bookmark indices, and a repeated mark
    // must retain the original brush/thickness needed for later restoration.
    muxc::StackPanel row;
    bar.panel.Children().Append(muxc::Border());
    bar.panel.Children().Append(row);
    for (unsigned i = 0; i < 3; ++i) row.Children().Append(xaml_gap::SizedButton(50));
    auto target = row.Children().GetAt(2).as<muxc::Button>();
    muxm::SolidColorBrush original(winrt::Windows::UI::Color{255, 1, 2, 3});
    target.BorderBrush(original);
    target.BorderThickness({1, 2, 4, 5});
    ShowInsertionMark(state, bar.panel, 0);
    const auto marked = target.BorderBrush();
    ShowInsertionMark(state, bar.panel, 0);
    Require(target.BorderBrush() == marked && target.Tag().as<muxm::Brush>() == original,
            "repeated marker overwrote the saved border or recreated its brush");
    ShowInsertionMark(state, bar.panel, 1);
    Require(state.dropAfter && target.BorderThickness().Right == 3,
            "same bookmark did not switch from before to after insertion");
    row.Children().RemoveAtEnd();
    ShowInsertionMark(state, bar.panel, 0);
    Require(!state.dropTarget.get() && target.BorderBrush() == original && !target.Tag() &&
            target.BorderThickness().Left == 1 && target.BorderThickness().Top == 2 &&
            target.BorderThickness().Right == 4 && target.BorderThickness().Bottom == 5,
            "fixed-only strip retained a marker or lost the original border");
    row.Children().Clear();
    ShowInsertionMark(state, bar.panel, 0);
    Require(!state.dropTarget.get(), "empty strip gained an insertion mark");

    BarState noRecents;
    noRecents.visibleRecents = 9;
    Require(FitRecents(noRecents, 100, {50, 50}) == 0 && noRecents.visibleRecents == 0,
            "expired recents reference retained visible counts");
    muxc::StackPanel expired;
    noRecents.recents = winrt::make_weak(expired);
    expired = nullptr;
    Require(FitRecents(noRecents, 100, {50, 50}) == 0,
            "expired recent panel consumed space");
    for (const auto width : {10.0, 20.0, 20.0}) {
        muxc::Border item;
        item.Width(width);
        bar.recents.Children().Append(item);
    }
    state.recentButtonCount = 1;
    state.hasRecentMenuButton = true;
    Require(FitRecents(state, 100, std::vector<double>(9, 30)) == 30 && state.visibleRecents == 0,
            "RC fitting ignored overflow beyond the fourth bookmark row");
    std::puts("PASS: malformed XAML rows, reflow admission/overflow, marker restoration and expired recents");
}

void CheckXamlBitmapCacheGaps() {
    xaml_gap::IconScope isolated;
    const std::wstring path = L"C:\\XamlBitmapFixture";
    auto initial = CachedFolderBitmap(path);
    Require(initial && initial.PixelWidth() == 2 && initial.PixelHeight() == 2 &&
            xaml_gap::BitmapBytes(initial) == std::vector<BYTE>(16, 0x5a) &&
            platform_failure::icon.live == 0,
            "cached bitmap lost exact BGRA pixels or retained WIC resources");
    const auto copied = platform_failure::icon.copies;
    Require(CachedFolderBitmap(L"c:\\xamlbitmapfixture") == initial &&
            platform_failure::icon.copies == copied && g_iconCache.size() == 1,
            "case-equivalent cache hit decoded/recreated the live bitmap");
    xaml_gap::Expire(path);
    Require(CachedFolderBitmap(path) == initial && platform_failure::icon.copies == copied + 1,
            "expired unchanged pixels replaced their live bitmap");
    auto weak = winrt::make_weak(initial);
    initial = nullptr;
    Require(!weak.get(), "bitmap cache held a strong XAML bitmap reference");
    const auto decoded = platform_failure::icon.copies;
    auto revived = CachedFolderBitmap(path);
    Require(revived && xaml_gap::BitmapBytes(revived) == std::vector<BYTE>(16, 0x5a) &&
            platform_failure::icon.copies == decoded,
            "expired bitmap wrapper could not be recreated from cached pixels without Shell I/O");
    xaml_gap::iconByte = 0x33;
    xaml_gap::Expire(path);
    auto changed = CachedFolderBitmap(path);
    Require(changed && changed != revived && xaml_gap::BitmapBytes(changed) == std::vector<BYTE>(16, 0x33) &&
            xaml_gap::BitmapBytes(revived) == std::vector<BYTE>(16, 0x5a),
            "changed icon bytes retained a stale bitmap");
    platform_failure::icon.width = 3;
    xaml_gap::Expire(path);
    auto wider = CachedFolderBitmap(path);
    Require(wider != changed && wider.PixelWidth() == 3 && wider.PixelHeight() == 2 &&
            xaml_gap::BitmapBytes(wider).size() == 24,
            "changed icon width retained stale bitmap dimensions");
    platform_failure::icon.height = 3;
    xaml_gap::Expire(path);
    auto taller = CachedFolderBitmap(path);
    Require(taller != wider && taller.PixelWidth() == 3 && taller.PixelHeight() == 3,
            "changed icon height retained stale bitmap dimensions");
    platform_failure::icon.copy = E_FAIL;
    xaml_gap::Expire(path);
    Require(!CachedFolderBitmap(path) && !xaml_gap::Cache(path).pixels &&
            !xaml_gap::Cache(path).bitmap.get() && platform_failure::icon.live == 0,
            "failed icon reload retained old cache pixels/bitmap or leaked WIC objects");
    xaml_gap::Expire(path);
    Require(!CachedFolderBitmap(path), "repeated failed icon reload invented pixels");
    platform_failure::icon.copy = S_OK;
    xaml_gap::Expire(path);
    Require(CachedFolderBitmap(path) != nullptr, "failed icon cache could not recover after its retry interval");
    Require(!BitmapFromPixels({2, 2, std::vector<BYTE>(17, 1)}),
            "oversized pixel buffer was copied beyond the real bitmap capacity");
    Require(!BitmapFromPixels({-1, 2, {}}), "invalid bitmap dimensions did not fail safely");
    std::puts("PASS: real bitmap bytes, weak cache ownership, unchanged/changed reload identity and recovery");
}

void CheckXamlFolderRefreshGaps() {
    fixture::Reset();
    xaml_gap::IconScope isolated;
    const std::wstring path = L"C:\\XamlStatusFixture";
    fixture::drives[L"C:\\"] = DRIVE_FIXED;
    fixture::attributes[path] = {FILE_ATTRIBUTE_DIRECTORY, ERROR_SUCCESS};
    auto button = MakeFolderButton(path, FolderStatus::Available,
                                   FolderTooltip(path, FolderStatus::Available, false));
    auto content = button.Content().as<muxc::StackPanel>();
    auto label = content.Children().GetAt(1).as<muxc::TextBlock>();
    auto icon = content.Children().GetAt(0);
    std::vector<FolderButtonState> tracked;
    RememberFolderButton(tracked, path, button, FolderStatus::Available);
    muxc::Button dead;
    RememberFolderButton(tracked, L"C:\\ExpiredFixture", dead, FolderStatus::Missing);
    dead = nullptr;
    const auto copies = platform_failure::icon.copies;
    Require(!RefreshFolderButtons(tracked, false, GetTickCount64()) &&
            content.Children().GetAt(0) == icon && platform_failure::icon.copies == copies,
            "unchanged refresh replaced visuals, decoded again or failed on an expired button");
    xaml_gap::Expire(path);
    Require(!RefreshFolderButtons(tracked, false, GetTickCount64()) && content.Children().GetAt(0) == icon,
            "same-pixel refresh replaced the image control or requested layout");
    auto oldSource = icon.as<muxc::Image>().Source();
    ForgetCachedIcon(path);
    Require(!RefreshFolderButtons(tracked, false, GetTickCount64()) &&
            content.Children().GetAt(0) != icon &&
            content.Children().GetAt(0).as<muxc::Image>().Source() != oldSource &&
            content.Children().GetAt(1) == label && button.Content() == content,
            "evicted available icon failed to recover or replaced label/button content");
    fixture::attributes[path] = {INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND};
    Require(RefreshFolderButtons(tracked, true, GetTickCount64()) &&
            tracked[0].status == FolderStatus::Missing && std::fabs(label.Opacity() - 0.7) < 0.001 &&
            content.Children().GetAt(0).as<muxc::FontIcon>().Glyph() == L"\uE7BA" &&
            winrt::unbox_value<winrt::hstring>(muxc::ToolTipService::GetToolTip(button)) ==
                FolderTooltip(path, FolderStatus::Missing, true) && FolderIconLoadedAt(path) == 0,
            "missing status failed to replace icon, dim label, explain recent actions or clear cached pixels");
    fixture::attributes[path] = {INVALID_FILE_ATTRIBUTES, ERROR_ACCESS_DENIED};
    Require(RefreshFolderButtons(tracked, false, GetTickCount64()) &&
            tracked[0].status == FolderStatus::Unknown && label.Opacity() == 1.0 &&
            content.Children().GetAt(0).as<muxc::FontIcon>().Glyph() == L"\uE8B7" &&
            winrt::unbox_value<winrt::hstring>(muxc::ToolTipService::GetToolTip(button)) ==
                FolderTooltip(path, FolderStatus::Unknown, false),
            "unknown status retained a misleading missing badge/tooltip");
    icon = content.Children().GetAt(0);
    Require(!RefreshFolderButtons(tracked, false, GetTickCount64()) && content.Children().GetAt(0) == icon,
            "unchanged unknown status replaced its generic glyph");
    fixture::attributes[path] = {FILE_ATTRIBUTE_DIRECTORY, ERROR_SUCCESS};
    platform_failure::icon.copy = E_FAIL;
    Require(RefreshFolderButtons(tracked, false, GetTickCount64()) &&
            tracked[0].status == FolderStatus::Available && content.Children().GetAt(0) == icon &&
            content.Children().GetAt(1) == label,
            "restored status with failed icon decoding lost the existing generic glyph/label");
    platform_failure::icon.copy = S_OK;
    xaml_gap::Expire(path);
    Require(RefreshFolderButtons(tracked, false, GetTickCount64()) &&
            content.Children().GetAt(0).try_as<muxc::Image>() && content.Children().GetAt(1) == label,
            "available folder icon did not recover from the generic glyph without replacing its label");
    fixture::Reset();
    std::puts("PASS: incremental folder visual identity, expired buttons, missing/unknown/restored status and icon recovery");
}

void CheckXamlRefreshFailureRecovery() {
    fixture::Reset();
    fixture::integers[L"recent.enabled"] = 1;
    fixture::integers[L"recent.buttons"] = 1;
    fixture::integers[L"recent.history"] = 10;
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture bar;
    const std::wstring path = L"C:\\RefreshFailureFixture";
    fixture::drives[L"C:\\"] = DRIVE_FIXED;
    fixture::attributes[path] = {INVALID_FILE_ATTRIBUTES, ERROR_FILE_NOT_FOUND};
    SaveBookmarksLocked({path});
    RecordExplorerRecent(L"\\\\fixture.invalid\\Recovery recent");
    RefreshPanel(bar.panel);
    auto oldButtons = BarButtons(bar.panel);
    Require(bar.state->renderComplete && bar.state->recentButtonCount == 1,
            "refresh failure fixture did not render initial controls");
    const auto checkedAt = bar.state->lastFolderCheckAt;
    g_unloading = true;
    RefreshPanel(bar.panel);
    g_unloading = false;
    g_threadClosing = true;
    RefreshPanel(bar.panel);
    g_threadClosing = false;
    Require(BarButtons(bar.panel) == oldButtons && bar.state->lastFolderCheckAt == checkedAt,
            "refresh during unload/thread closure touched the rendered controls");
    // Simulate externally corrupted content during the next status refresh.
    // The public refresh boundary must catch it and schedule a full rebuild.
    oldButtons[2].Content(muxc::TextBlock());
    fixture::attributes[path] = {INVALID_FILE_ATTRIBUTES, ERROR_ACCESS_DENIED};
    bar.state->lastFolderCheckAt = 0;
    RefreshPanel(bar.panel);
    Require(!bar.state->renderComplete && bar.state->lastFolderCheckAt == 0 &&
            bar.state->recentFolders.empty() && bar.state->recentButtonCount == 0 &&
            !bar.state->hasRecentMenuButton && bar.recents.Children().Size() == 0,
            "failed refresh retained stale recents or claimed a complete render");
    RefreshPanel(bar.panel);
    auto restored = BarButtons(bar.panel);
    Require(bar.state->renderComplete && restored.size() == 3 && restored != oldButtons &&
            bar.state->folderButtons[0].status == FolderStatus::Unknown &&
            bar.state->recentButtonCount == 1 && !bar.state->panelHandlers.empty(),
            "hover retry did not rebuild usable bookmarks/recent controls after a refresh exception");
    muxc::StackPanel untracked;
    RefreshPanel(untracked);
    Require(untracked.Children().Size() == 0, "refresh added controls to an untracked panel");
    fixture::Reset();
    std::puts("PASS: refresh exception containment, stale-recent cleanup and full hover recovery");
}

void CheckXamlTeardownOwnershipGaps() {
    {
        XamlBarFixture bar;
        auto& state = *bar.state;
        muxc::RowDefinition outsider;
        bar.grid.RowDefinitions().Append(outsider);
        muxc::Border sibling;
        bar.grid.Children().Append(sibling);
        bar.grid.MinHeight(999);
        bar.nav.MinHeight(888);
        state.commandRow.Height({31, mux::GridUnitType::Pixel});
        RemoveBarVisuals(state);
        Require(bar.grid.RowDefinitions().Size() == 3 && bar.grid.RowDefinitions().GetAt(2) == outsider &&
                bar.grid.Children().Size() == 1 && bar.grid.Children().GetAt(0) == sibling &&
                bar.grid.MinHeight() == 999 && bar.nav.MinHeight() == 888 &&
                bar.host.RowDefinitions().GetAt(2).Height().Value == 31,
                "teardown removed externally appended rows/siblings or overwrote external sizing");
        RemoveBarVisuals(state);
        Require(bar.grid.RowDefinitions().Size() == 3 && bar.grid.Children().Size() == 1,
                "repeated teardown removed external controls");
    }
    {
        XamlBarFixture bar;
        bar.state->createdFirstRow = true;
        bar.grid.MinHeight(44);
        bar.nav.MinHeight(44);
        bar.state->appliedGridMinHeight = 44;
        bar.state->appliedNavMinHeight = 44;
        bar.state->oldGridMinHeight = 7;
        bar.state->oldNavMinHeight = 8;
        bar.host.RowDefinitions().InsertAt(0, muxc::RowDefinition());
        RemoveBarVisuals(*bar.state);
        Require(bar.grid.RowDefinitions().Size() == 0 && bar.grid.MinHeight() == 7 &&
                bar.nav.MinHeight() == 8 && bar.host.RowDefinitions().Size() == 4 &&
                bar.host.RowDefinitions().GetAt(3).Height().GridUnitType == mux::GridUnitType::Auto,
                "teardown failed to remove its synthetic first row or restored a changed host layout");
    }
    {
        XamlBarFixture bar;
        const auto replacement = muxc::RowDefinition();
        bar.host.RowDefinitions().SetAt(2, replacement);
        bar.grid.Children().Clear();
        muxc::Border sibling;
        bar.grid.Children().Append(sibling);
        RemoveBarVisuals(*bar.state);
        Require(bar.host.RowDefinitions().GetAt(2) == replacement &&
                bar.grid.Children().Size() == 1 && bar.grid.Children().GetAt(0) == sibling,
                "teardown restored an obsolete command row or removed a sibling after external root removal");
    }
    BarState expired;
    {
        muxc::Grid grid;
        muxc::Grid nav;
        muxc::ScrollViewer strip;
        expired.grid = winrt::make_weak(grid);
        expired.hostGrid = winrt::make_weak(grid);
        expired.barRoot = winrt::make_weak(grid);
        expired.navControl = winrt::make_weak<mux::FrameworkElement>(nav);
        expired.strip = winrt::make_weak(strip);
    }
    expired.addedRow = muxc::RowDefinition();
    expired.commandRow = muxc::RowDefinition();
    expired.recentFolders = {L"C:\\expired"};
    expired.dragPath = L"C:\\expired";
    expired.suppressClick = L"C:\\expired";
    expired.rowCount = 4;
    expired.renderComplete = true;
    SetBarRowCount(expired, 2);
    Require(expired.rowCount == 2 && expired.addedRow.Height().Value == RowAllocation(2),
            "expired visual references prevented owned row allocation from updating safely");
    RemoveBarVisuals(expired);
    Require(!expired.grid.get() && !expired.addedRow && !expired.commandRow &&
            expired.recentFolders.empty() && expired.dragPath.empty() && expired.suppressClick.empty() &&
            expired.rowCount == 1 && !expired.renderComplete,
            "expired XAML references prevented teardown from releasing owned state");
    std::puts("PASS: XAML teardown ownership, synthetic first row, changed host rows and expired references");
}

void CheckXamlInstallationDiscoveryGaps() {
    fixture::Reset();
    LoadSettings();
    UpdateThreadSettings();
    if (!g_bars) g_bars.emplace();
    Require(g_bars->empty(), "loaded installation gap fixture requires no prior bars");
    g_threadClosing = false;
    g_frameHooked = true;
    muxc::Grid host;
    for (unsigned i = 0; i < 3; ++i) host.RowDefinitions().Append(muxc::RowDefinition());
    host.RowDefinitions().GetAt(0).Height({1, mux::GridUnitType::Auto});
    muxc::Grid nav;
    muxc::Grid::SetRow(nav, 1);
    host.Children().Append(nav);
    muxc::Grid navigation;
    navigation.Name(L"NavigationBarControlGrid");
    nav.Children().Append(navigation);
    muxc::CommandBar command;
    command.Name(L"FileExplorerCommandBar");
    navigation.Children().Append(command);
    mux::Window window;
    window.Content(host);
    window.AppWindow().Resize({800, 400});
    window.Activate();
    PumpXamlMessages();
    TryInstallBar(command);
    Require(g_bars->empty() && navigation.RowDefinitions().Size() == 0,
            "untracked command bar modified the header");
    // Direct state registration isolates installation admission from Loaded.
    g_bars->emplace_back();
    auto& state = g_bars->back();
    state.commandBar = winrt::make_weak(command);
    g_unloading = true;
    TryInstallBar(command);
    ScanCurrentThreadForCommandBars();
    g_unloading = false;
    g_frameHooked = false;
    TryInstallBar(command);
    ScanCurrentThreadForCommandBars();
    g_frameHooked = true;
    Require(!state.strip.get(), "unloading/unhooked installation or discovery changed visuals");
    muxc::Grid ownedElsewhere;
    ownedElsewhere.Name(kBarName);
    navigation.Children().Append(ownedElsewhere);
    TryInstallBar(command);
    Require(!state.strip.get() && navigation.RowDefinitions().Size() == 0,
            "another tab's named bar received duplicate visuals");
    navigation.Children().RemoveAtEnd();
    navigation.Name(L"UnknownNavigationGrid");
    TryInstallBar(command);
    navigation.Name(L"NavigationBarControlGrid");
    Require(!state.strip.get(), "missing navigation name received a bar");
    nav.Children().Clear();
    window.Content(nullptr);
    PumpXamlMessages();
    window.Content(navigation);
    PumpXamlMessages();
    TryInstallBar(command);
    Require(!state.strip.get(), "navigation grid without measured host received a bar");
    window.Content(nullptr);
    nav.Children().Append(navigation);
    window.Content(host);
    PumpXamlMessages();
    for (unsigned mismatch = 0; mismatch < 4; ++mismatch) {
        if (mismatch == 0) host.RowDefinitions().Append(muxc::RowDefinition());
        if (mismatch == 1) muxc::Grid::SetRow(nav, 0);
        if (mismatch == 2) host.RowDefinitions().GetAt(1).Height({12, mux::GridUnitType::Pixel});
        if (mismatch == 3) host.RowDefinitions().GetAt(2).Height({12, mux::GridUnitType::Pixel});
        TryInstallBar(command);
        Require(!state.strip.get() && navigation.RowDefinitions().Size() == 0,
                "unknown host shape received inserted bookmark rows");
        if (mismatch == 0) host.RowDefinitions().RemoveAtEnd();
        if (mismatch == 1) muxc::Grid::SetRow(nav, 1);
        if (mismatch == 2) host.RowDefinitions().GetAt(1).Height({1, mux::GridUnitType::Star});
        if (mismatch == 3) host.RowDefinitions().GetAt(2).Height({1, mux::GridUnitType::Star});
    }
    state.addedRow = muxc::RowDefinition(); // stale installation has to be discarded before reinstalling
    TryInstallBar(command);
    Require(state.strip.get() && state.renderComplete && state.createdFirstRow,
            "valid host did not recover from stale installation state");
    const auto root = state.barRoot.get();
    muxc::CommandBar second;
    second.Name(L"FileExplorerCommandBar");
    navigation.Children().Append(second);
    muxc::CommandBar unrelated;
    unrelated.Name(L"UnrelatedCommandBar");
    navigation.Children().Append(unrelated);
    std::vector<muxc::CommandBar> found;
    CollectCommandBars(host, 0, found);
    Require(found == std::vector<muxc::CommandBar>{command, second},
            "recursive command discovery included unrelated controls or lost another tab");
    muxc::CommandBar detached;
    detached.Name(L"FileExplorerCommandBar");
    ScanXamlRootForCommandBars(nullptr);
    ScanXamlRootForCommandBars(detached);
    g_bars->emplace_back(); // an expired known bar must not abort discovery
    ScanCurrentThreadForCommandBars();
    Require(g_bars->size() == 2 && state.barRoot.get() == root && navigation.RowDefinitions().Size() == 2,
            "thread discovery retained expired state, duplicated visuals or missed the second command bar");
    Require(g_bars->back().commandBar.get() == second && !g_bars->back().strip.get(),
            "second tab was not tracked or duplicated the existing header row");
    RemoveBarVisuals(state);
    window.Content(nullptr);
    PumpXamlMessages();
    window.Close();
    CleanupCurrentThread();
    Require(!g_bars && !g_workTimer && !g_messageHook, "loaded discovery fixture leaked thread XAML state");
    g_bars.emplace();
    g_frameHooked = false;
    fixture::Reset();
    std::puts("PASS: XAML installation admission, changed/stale headers, duplicate tab ownership and thread discovery");
}
