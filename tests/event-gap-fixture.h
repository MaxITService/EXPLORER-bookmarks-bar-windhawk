// Include a BOOKMARK_EVENT_PLATFORM_ONLY pass after the native fixtures and
// before the unchanged source. The ordinary later pass adds the XAML helpers.
// Separate guards let this one header supply both test-only fixture layers.
#ifndef BOOKMARK_EVENT_DRIVE_PLATFORM_DEFINED
#define BOOKMARK_EVENT_DRIVE_PLATFORM_DEFINED
#include <optional>

namespace event_gap_platform {
struct Faults {
    std::optional<DWORD> logicalMask;
    std::map<std::wstring, std::pair<DWORD, DWORD>> attributes;
    unsigned logicalCalls = 0;
    std::vector<std::wstring> attributeQueries;
};
inline Faults faults;
struct Scope {
    Faults old = std::move(faults);
    std::map<std::wstring, UINT> oldDrives = fixture::drives;
    Scope() { faults = {}; }
    ~Scope() { faults = std::move(old); fixture::drives = std::move(oldDrives); }
};
}

inline auto EventPriorLogicalDrives = &GetLogicalDrives;
inline DWORD WINAPI EventGetLogicalDrives() {
    auto& faults = event_gap_platform::faults;
    if (!faults.logicalMask) return EventPriorLogicalDrives();
    ++faults.logicalCalls;
    if (*faults.logicalMask == 0) SetLastError(ERROR_NOT_READY);
    return *faults.logicalMask;
}
inline auto EventPriorFileAttributes = &GetFileAttributesW;
#undef GetFileAttributesW
inline DWORD WINAPI EventGetFileAttributesW(LPCWSTR path) {
    auto& faults = event_gap_platform::faults;
    if (faults.attributes.empty()) return EventPriorFileAttributes(path);
    const std::wstring value = path ? path : L"";
    faults.attributeQueries.push_back(value);
    const auto found = faults.attributes.find(value);
    if (found == faults.attributes.end()) return EventPriorFileAttributes(path);
    SetLastError(found->second.second);
    return found->second.first;
}
#undef GetLogicalDrives
#define GetLogicalDrives EventGetLogicalDrives
#define GetFileAttributesW EventGetFileAttributesW
#endif

#if !defined(BOOKMARK_EVENT_PLATFORM_ONLY) && !defined(BOOKMARK_EVENT_XAML_FIXTURE_DEFINED)
#define BOOKMARK_EVENT_XAML_FIXTURE_DEFINED
#include "event-observer.h"
#include "xaml-fixture.h"

namespace event_gap {
namespace wf = winrt::Windows::Foundation;
struct SuspendWindowsRecentScan {
    bool prior = g_windowsRecentStopping.exchange(true);
    ~SuspendWindowsRecentScan() { g_windowsRecentStopping = prior; }
};
struct ContextArgs : winrt::implements<ContextArgs, muxi::IContextRequestedEventArgs> {
    bool handled = false;
    bool Handled() { return handled; }
    void Handled(bool value) { handled = value; }
    bool TryGetPosition(const mux::UIElement&, wf::Point& position) { position = {}; return false; }
    muxi::ContextRequestedEventArgs Event() { return get_strong().as<muxi::ContextRequestedEventArgs>(); }
};
struct UnloadingDialog : FakeDialog {
    HRESULT STDMETHODCALLTYPE GetResult(IShellItem** result) override {
        const auto status = FakeDialog::GetResult(result);
        if (SUCCEEDED(status)) g_unloading = true;
        return status;
    }
};

template<typename Element>
mux::RoutedEventHandler Click(const Element& element) {
    auto handler = event_observer::Find(element, L"Click").click;
    Require(static_cast<bool>(handler), "original registered Click delegate was not observed");
    return handler;
}
inline event_observer::Opening Opening(const muxc::MenuFlyout& menu) {
    auto handler = event_observer::Find(menu, L"Opening").opening;
    Require(static_cast<bool>(handler), "original registered Opening delegate was not observed");
    return handler;
}
inline event_observer::Context Context(const muxc::Button& button) {
    auto handler = event_observer::Find(button, L"ContextRequested").context;
    Require(static_cast<bool>(handler), "original registered ContextRequested delegate was not observed");
    return handler;
}
inline void ConfigureRecents(bool rightClears = true) {
    fixture::integers[L"recent.enabled"] = 1;
    fixture::integers[L"recent.buttons"] = 1;
    fixture::integers[L"recent.history"] = 10;
    fixture::strings[L"recent.rcButton"] = rightClears ? L"clear" : L"menu";
    LoadSettings();
    UpdateThreadSettings();
}
inline void ResetObservers() {
    event_observer::Clear();
    pointer_observer::Clear();
}

// The fixture owns a hidden standalone WinUI window with the recognized header
// shape. It never creates Explorer hooks or opens RC's default radio template.
struct InstalledBar {
    muxc::Grid host;
    muxc::Grid nav;
    muxc::Grid navigation;
    muxc::CommandBar command;
    HWND window = nullptr;
    mux::Hosting::DesktopWindowXamlSource source;
    BarState* state = nullptr;
    InstalledBar() {
        if (!g_bars) g_bars.emplace();
        g_threadClosing = false;
        g_frameHooked = true;
        for (unsigned i = 0; i < 3; ++i) host.RowDefinitions().Append(muxc::RowDefinition());
        host.RowDefinitions().GetAt(0).Height({0, mux::GridUnitType::Auto});
        muxc::Grid::SetRow(nav, 1);
        host.Children().Append(nav);
        navigation.Name(L"NavigationBarControlGrid");
        nav.Children().Append(navigation);
        command.Name(L"FileExplorerCommandBar");
        navigation.Children().Append(command);
        TrackCommandBar(command);
        window = CreateWindowExW(0, L"STATIC", L"Hidden callback fixture", WS_OVERLAPPED,
                                  0, 0, 800, 400, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        Require(window != nullptr, "hidden callback host creation failed");
        source.Initialize(winrt::Microsoft::UI::WindowId{reinterpret_cast<uint64_t>(window)});
        source.Content(host);
        source.SiteBridge().MoveAndResize({0, 0, 800, 400});
        PumpXamlMessages(30);
        // The native host remains hidden throughout the callback checks.
        TryInstallBar(command);
        for (auto& bar : *g_bars) if (bar.commandBar.get() == command) state = &bar;
        Require(state && state->barRoot.get() && state->buttons.get(),
                "hidden recognized header did not install real root delegates");
        host.Measure({800, 400});
        host.Arrange({0, 0, 800, 400});
    }
    ~InstalledBar() {
        if (state) {
            RemoveBarVisuals(*state);
            g_bars->remove_if([&](const auto& bar) { return &bar == state; });
        }
        source.Content(nullptr);
        source.Close();
        DestroyWindow(window);
        StopThreadWork();
        g_frameHooked = false;
    }
};
}
#endif
