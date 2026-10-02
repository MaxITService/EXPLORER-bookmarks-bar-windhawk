#pragma once
#include <appmodel.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>

// The existing system framework is added only to this process's dependency
// graph. No package installation, Explorer injection or mod activation occurs.
struct XamlRuntime {
    winrt::Microsoft::UI::Dispatching::DispatcherQueueController queue{nullptr};
    winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager manager{nullptr};
    XamlRuntime() {
        using CreateDependency = HRESULT(WINAPI*)(PSID, PCWSTR, PACKAGE_VERSION, unsigned, unsigned, PCWSTR, unsigned, PWSTR*);
        using AddDependency = HRESULT(WINAPI*)(PCWSTR, INT32, unsigned, void**, PWSTR*);
        auto kernel = GetModuleHandleW(L"kernelbase.dll");
        auto create = reinterpret_cast<CreateDependency>(GetProcAddress(kernel, "TryCreatePackageDependency"));
        auto add = reinterpret_cast<AddDependency>(GetProcAddress(kernel, "AddPackageDependency"));
        if (!create || !add) throw winrt::hresult_error(E_NOTIMPL);
        PWSTR dependency = nullptr;
        winrt::check_hresult(create(nullptr, L"Microsoft.WindowsAppRuntime.CBS.1.6_8wekyb3d8bbwe",
                                    PACKAGE_VERSION{}, 4 /* x64 */, 0 /* process lifetime */,
                                    nullptr, 0, &dependency));
        void* context = nullptr;
        PWSTR fullName = nullptr;
        const HRESULT added = add(dependency, 0, 0, &context, &fullName);
        HeapFree(GetProcessHeap(), 0, dependency);
        if (fullName) HeapFree(GetProcessHeap(), 0, fullName);
        winrt::check_hresult(added);
        queue = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();
        manager = winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
    }
    ~XamlRuntime() { if (manager) manager.Close(); }
};

// Measured structure/vertical geometry from diagnostics/layout.md (96 DPI).
// Public controls stand in for Explorer's private controls; this is not a
// captured Explorer tree or a reproduction of its templates/theme resources.
struct ExplorerHeaderFixture {
    muxc::ScrollViewer root;
    muxc::Border border;
    muxc::Grid host;
    muxc::Grid tabs;
    muxc::Grid nav;
    muxc::Grid navigation;
    muxc::Grid commandControl;
    muxc::Grid commandRoot;
    muxc::CommandBar command;
    ExplorerHeaderFixture() {
        root.Name(L"RootScrollViewer");
        root.HorizontalScrollBarVisibility(muxc::ScrollBarVisibility::Disabled);
        root.VerticalScrollBarVisibility(muxc::ScrollBarVisibility::Disabled);
        root.Content(border);
        border.Child(host);
        host.VerticalAlignment(mux::VerticalAlignment::Top);
        host.Height(134);
        for (unsigned i = 0; i < 3; ++i) host.RowDefinitions().Append(muxc::RowDefinition());
        host.RowDefinitions().GetAt(0).Height({0, mux::GridUnitType::Auto});
        tabs.Name(L"FileExplorerTabControl");
        tabs.Height(38);
        nav.Name(L"NavigationBarControl");
        navigation.Name(L"NavigationBarControlGrid");
        navigation.Padding({3, 3, 3, 3});
        navigation.VerticalAlignment(mux::VerticalAlignment::Center);
        muxc::RowDefinition originalRow;
        originalRow.Height({0, mux::GridUnitType::Auto});
        navigation.RowDefinitions().Append(originalRow);
        for (auto name : {L"NavigationCommands", L"FileExplorerAddressBarGrid", L"FileExplorerSearchBox"}) {
            muxc::Grid child;
            child.Name(name);
            child.Height(name == std::wstring_view(L"FileExplorerSearchBox") ? 32 : 48);
            child.VerticalAlignment(mux::VerticalAlignment::Center);
            navigation.Children().Append(child);
        }
        nav.Children().Append(navigation);
        commandControl.Name(L"CommandBarControl");
        commandRoot.Name(L"CommandBarControlRootGrid");
        command.Name(L"FileExplorerCommandBar");
        command.Height(48);
        commandRoot.Children().Append(command);
        commandControl.Children().Append(commandRoot);
        muxc::Grid::SetRow(nav, 1);
        muxc::Grid::SetRow(commandControl, 2);
        host.Children().Append(tabs);
        host.Children().Append(nav);
        host.Children().Append(commandControl);
    }
};

struct XamlBarFixture {
    muxc::Grid grid;
    muxc::Grid host;
    muxc::Grid nav;
    muxc::Grid root;
    muxc::ScrollViewer strip;
    muxc::StackPanel panel;
    muxc::StackPanel recents;
    BarState* state;
    XamlBarFixture() {
        if (!g_bars) g_bars.emplace();
        g_bars->emplace_back();
        state = &g_bars->back();
        state->grid = winrt::make_weak(grid);
        state->hostGrid = winrt::make_weak(host);
        state->navControl = winrt::make_weak<mux::FrameworkElement>(nav);
        state->barRoot = winrt::make_weak(root);
        state->strip = winrt::make_weak(strip);
        state->buttons = winrt::make_weak(panel);
        state->recents = winrt::make_weak(recents);
        state->addedRow = muxc::RowDefinition();
        grid.RowDefinitions().Append(muxc::RowDefinition());
        grid.RowDefinitions().Append(state->addedRow);
        for (unsigned i = 0; i < 3; ++i) host.RowDefinitions().Append(muxc::RowDefinition());
        state->commandRow = host.RowDefinitions().GetAt(2);
        state->commandRow.Height({0, mux::GridUnitType::Auto});
        strip.Content(panel);
        root.Children().Append(strip);
        root.Children().Append(recents);
        grid.Children().Append(root);
        g_threadClosing = false;
    }
    ~XamlBarFixture() {
        RemoveBarVisuals(*state);
        g_bars->remove_if([&](const auto& bar) { return &bar == state; });
        StopThreadWork();
    }
};
