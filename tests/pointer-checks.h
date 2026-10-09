#pragma once
#include "pointer-observer.h"

namespace pointer_fixture {
namespace mi = winrt::Microsoft::UI::Input;
namespace wf = winrt::Windows::Foundation;

struct Properties : winrt::implements<Properties, mi::IPointerPointProperties> {
    bool left = false;
    bool middle = false;
    wf::Rect ContactRect() { return {}; }
    bool IsBarrelButtonPressed() { return false; }
    bool IsCanceled() { return false; }
    bool IsEraser() { return false; }
    bool IsHorizontalMouseWheel() { return false; }
    bool IsInRange() { return true; }
    bool IsInverted() { return false; }
    bool IsLeftButtonPressed() { return left; }
    bool IsMiddleButtonPressed() { return middle; }
    bool IsPrimary() { return true; }
    bool IsRightButtonPressed() { return false; }
    bool IsXButton1Pressed() { return false; }
    bool IsXButton2Pressed() { return false; }
    int32_t MouseWheelDelta() { return 0; }
    float Orientation() { return 0; }
    mi::PointerUpdateKind PointerUpdateKind() { return mi::PointerUpdateKind::Other; }
    float Pressure() { return 0; }
    bool TouchConfidence() { return true; }
    float Twist() { return 0; }
    float XTilt() { return 0; }
    float YTilt() { return 0; }
};
struct Point : winrt::implements<Point, mi::IPointerPoint> {
    uint32_t id = 17;
    mi::PointerDeviceType type = mi::PointerDeviceType::Touch;
    wf::Point position{5, 5};
    winrt::com_ptr<pointer_fixture::Properties> properties = winrt::make_self<pointer_fixture::Properties>();
    uint32_t FrameId() { return 1; }
    bool IsInContact() { return true; }
    mi::PointerDeviceType PointerDeviceType() { return type; }
    uint32_t PointerId() { return id; }
    wf::Point Position() { return position; }
    mi::PointerPointProperties Properties() { return properties.as<mi::PointerPointProperties>(); }
    uint64_t Timestamp() { return 0; }
    mi::PointerPoint GetTransformedPoint(const mi::IPointerPointTransform&) { throw winrt::hresult_not_implemented(); }
};
struct Pointer : winrt::implements<Pointer, muxi::IPointer> {
    uint32_t id = 17;
    mi::PointerDeviceType type = mi::PointerDeviceType::Touch;
    winrt::hstring GetRuntimeClassName() const { return L"Bookmarks.Tests.Pointer"; }
    uint32_t PointerId() { return id; }
    mi::PointerDeviceType PointerDeviceType() { return type; }
    bool IsInContact() { return true; }
    bool IsInRange() { return true; }
};
struct Args : winrt::implements<Args, muxi::IPointerRoutedEventArgs> {
    winrt::com_ptr<Point> point = winrt::make_self<Point>();
    winrt::com_ptr<pointer_fixture::Pointer> pointer = winrt::make_self<pointer_fixture::Pointer>();
    mux::UIElement strip{nullptr};
    wf::Point stripPosition{5, 5};
    bool handled = false;
    bool failPoint = false;
    muxi::Pointer Pointer() { return pointer.as<muxi::Pointer>(); }
    winrt::Windows::System::VirtualKeyModifiers KeyModifiers() { return winrt::Windows::System::VirtualKeyModifiers::None; }
    bool Handled() { return handled; }
    void Handled(bool value) { handled = value; }
    bool IsGenerated() { return false; }
    mi::PointerPoint GetCurrentPoint(const mux::UIElement& relativeTo) {
        if (failPoint) throw winrt::hresult_error(E_FAIL);
        auto result = winrt::make_self<Point>();
        result->id = point->id;
        result->type = pointer->type;
        result->properties = point->properties;
        result->position = relativeTo == strip ? stripPosition : point->position;
        return result.as<mi::PointerPoint>();
    }
    wf::Collections::IVector<mi::PointerPoint> GetIntermediatePoints(const mux::UIElement&) { throw winrt::hresult_not_implemented(); }
    muxi::PointerRoutedEventArgs Event() { return get_strong().as<muxi::PointerRoutedEventArgs>(); }
};

inline void Deliver(const muxc::Button& registeredButton, const mux::RoutedEvent& event,
                    const wf::IInspectable& sender, const winrt::com_ptr<Args>& args) {
    auto handler = pointer_observer::Handler(registeredButton, event);
    Require(static_cast<bool>(handler), "production pointer delegate was not observed");
    args->handled = false;
    handler(sender, args->Event());
}
inline void Press(const muxc::Button& button, const winrt::com_ptr<Args>& args) {
    Deliver(button, mux::UIElement::PointerPressedEvent(), button, args);
}
inline void Move(const muxc::Button& button, const winrt::com_ptr<Args>& args) {
    Deliver(button, mux::UIElement::PointerMovedEvent(), button, args);
}
inline void Release(const muxc::Button& button, const winrt::com_ptr<Args>& args) {
    Deliver(button, mux::UIElement::PointerReleasedEvent(), button, args);
}
inline void Cancel(const muxc::Button& button, const winrt::com_ptr<Args>& args) {
    Deliver(button, mux::UIElement::PointerCanceledEvent(), button, args);
}
}

void CheckFolderPointerContracts() {
    fixture::Reset();
    pointer_observer::Clear();
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture bar;
    const std::wstring a = L"\\\\pointer.invalid\\A";
    const std::wstring b = L"\\\\pointer.invalid\\B";
    const std::wstring c = L"\\\\pointer.invalid\\C";
    SaveBookmarksLocked({a, b, c});
    RefreshPanel(bar.panel);
    bar.strip.Width(600);
    bar.strip.Height(kRowHeight);
    bar.grid.Measure({600, 200});
    bar.grid.Arrange({0, 0, 600, 200});
    auto buttons = BarButtons(bar.panel);
    auto button = buttons[2];
    auto args = winrt::make_self<pointer_fixture::Args>();
    args->strip = bar.strip;
    args->point->properties->left = true;
    g_unloading = true;
    pointer_fixture::Press(button, args);
    g_unloading = false;
    Require(bar.state->dragPath.empty(), "pointer press started drag during unload");
    pointer_fixture::Deliver(button, mux::UIElement::PointerPressedEvent(), bar.panel, args);
    Require(bar.state->dragPath.empty(), "non-button sender started drag");
    bar.state->suppressClick = a;
    button.Opacity(0.81);
    pointer_fixture::Press(button, args);
    Require(bar.state->dragPath == a && bar.state->dragButton.get() == button &&
            bar.state->dragPointerId == 17 && !bar.state->dragIsMouse &&
            !bar.state->dragging && bar.state->suppressClick.empty() &&
            std::fabs(bar.state->dragOldOpacity - 0.81) < 0.001,
            "left pointer press did not initialize drag identity/start/opacity");
    auto wrongSender = buttons[3];
    pointer_fixture::Deliver(button, mux::UIElement::PointerMovedEvent(), wrongSender, args);
    args->point->id = 18;
    pointer_fixture::Move(button, args);
    args->point->id = 17;
    args->point->properties->left = false;
    pointer_fixture::Move(button, args);
    args->point->properties->left = true;
    args->point->position = {5 + kDragThreshold - 1, 5};
    pointer_fixture::Move(button, args);
    Require(!bar.state->dragging && !args->handled && std::fabs(button.Opacity() - 0.81) < 0.001,
            "wrong sender/id/button state or below-threshold movement started drag");
    args->point->position = {5 + kDragThreshold, 5};
    pointer_fixture::Move(button, args);
    Require(bar.state->dragging && args->handled && bar.state->suppressClick == a &&
            std::fabs(button.Opacity() - 0.55) < 0.001 && bar.state->dropTarget.get(),
            "threshold movement did not begin drag, suppress click and show marker");
    args->stripPosition = {-1, 5};
    pointer_fixture::Move(button, args);
    Require(args->handled && !bar.state->dropTarget.get(), "pointer leaving strip retained marker");
    args->point->id = 18;
    pointer_fixture::Release(button, args);
    Require(bar.state->dragging, "another pointer release canceled drag");
    args->point->id = 17;
    const auto original = ReadStorageLocked();
    pointer_fixture::Release(button, args);
    Require(args->handled && bar.state->dragPath.empty() && !bar.state->dragging &&
            std::fabs(button.Opacity() - 0.81) < 0.001 && ReadStorageLocked() == original,
            "outside release modified storage or failed to restore drag appearance");

    args->stripPosition = {5, 5};
    args->point->position = {5, 5};
    pointer_fixture::Press(button, args);
    pointer_fixture::Release(button, args);
    Require(!args->handled && bar.state->dragPath.empty() && ReadStorageLocked() == original,
            "short press suppressed normal click or reordered bookmarks");
    pointer_fixture::Press(button, args);
    args->point->position = {10000, 5};
    pointer_fixture::Move(button, args);
    pointer_fixture::Release(button, args);
    Require(args->handled && SplitBookmarks(ReadStorageLocked()) == std::vector<std::wstring>{b, c, a} &&
            bar.state->dragPath.empty(), "inside release did not reorder bookmark to end");
    // A cancellation from the wrong source leaves the drag untouched; the
    // registered source's cancellation restores opacity and all drag state.
    button = BarButtons(bar.panel).back();
    args->point->position = {5, 5};
    pointer_fixture::Press(button, args);
    pointer_fixture::Deliver(button, mux::UIElement::PointerCanceledEvent(), wrongSender, args);
    Require(bar.state->dragPath == a, "unrelated cancellation cleared drag");
    pointer_fixture::Cancel(button, args);
    Require(bar.state->dragPath.empty() && !bar.state->dragButton.get() &&
            bar.state->dragPointerId == 0 && !bar.state->dragFromRecents,
            "pointer cancellation leaked drag state");
    args->point->properties->left = false;
    args->point->properties->middle = true;
    pointer_fixture::Press(button, args);
    Require(args->handled && SplitBookmarks(ReadStorageLocked()) == std::vector<std::wstring>{b, c},
            "middle press did not remove bookmark through real pointer handler");
    const auto stable = ReadStorageLocked();
    pointer_fixture::Press(button, args);
    Require(ReadStorageLocked() == stable, "middle press of removed folder changed storage");
    args->point->properties->middle = false;
    args->failPoint = true;
    pointer_fixture::Press(button, args);
    Require(ReadStorageLocked() == stable && bar.state->dragPath.empty(),
            "guarded pointer property failure changed storage or drag state");
    args->failPoint = false;
    bar.state->strip = nullptr;
    Require(!PointerInsideStrip(*bar.state, args->Event()), "missing strip counted as inside pointer target");
    bar.state->strip = winrt::make_weak(bar.strip);
    for (const pointer_fixture::wf::Point position : {pointer_fixture::wf::Point{5, -1}, {601, 5}, {5, kRowHeight + 1}}) {
        args->stripPosition = position;
        Require(!PointerInsideStrip(*bar.state, args->Event()), "out-of-strip pointer was accepted");
    }
    args->stripPosition = {600, kRowHeight};
    Require(PointerInsideStrip(*bar.state, args->Event()), "strip's inclusive right/bottom pointer boundary rejected");
    pointer_observer::Clear();
    std::puts("PASS: production pointer delegates, drag threshold/identity, inside/outside release, reorder, cancel and middle removal");
}

void CheckRecentPointerPromotionAndMouseRecovery() {
    fixture::Reset();
    pointer_observer::Clear();
    fixture::integers[L"recent.enabled"] = 1;
    fixture::integers[L"recent.buttons"] = 2;
    fixture::integers[L"recent.history"] = 10;
    LoadSettings();
    UpdateThreadSettings();
    XamlBarFixture bar;
    const std::wstring a = L"\\\\pointer.invalid\\bookmark";
    const std::wstring recent = L"\\\\pointer.invalid\\recent";
    const std::wstring removable = L"\\\\pointer.invalid\\remove";
    SaveBookmarksLocked({a});
    RecordExplorerRecent(recent);
    RecordExplorerRecent(removable);
    RefreshPanel(bar.panel);
    bar.strip.Width(600);
    bar.strip.Height(kRowHeight);
    bar.grid.Measure({600, 200});
    bar.grid.Arrange({0, 0, 600, 200});
    auto recentButton = bar.recents.Children().GetAt(1).as<muxc::Button>();
    auto args = winrt::make_self<pointer_fixture::Args>();
    args->strip = bar.strip;
    args->point->properties->left = true;
    pointer_fixture::Press(recentButton, args);
    Require(bar.state->dragPath == recent && bar.state->dragFromRecents,
            "recent button press did not mark promotion drag");
    args->point->position = {20, 5};
    pointer_fixture::Move(recentButton, args);
    pointer_fixture::Release(recentButton, args);
    Require(args->handled && SplitBookmarks(ReadStorageLocked()) == std::vector<std::wstring>{recent, a} &&
            bar.state->recentFolders == std::vector<std::wstring>{removable},
            "recent drag did not insert bookmark and remove its recent visual");
    recentButton = bar.recents.Children().GetAt(1).as<muxc::Button>();
    args->point->properties->left = false;
    args->point->properties->middle = true;
    pointer_fixture::Press(recentButton, args);
    Require(args->handled && SplitPaths(ReadValueLocked(g_recentsKey.c_str()), kMaxStoredRecents) ==
            std::vector<std::wstring>{recent} && bar.state->recentFolders.empty(),
            "middle press did not remove recent history and visual");
    auto button = BarButtons(bar.panel)[2];
    args->point->properties->middle = false;
    args->point->properties->left = true;
    args->pointer->type = pointer_fixture::mi::PointerDeviceType::Mouse;
    pointer_fixture::Press(button, args);
    Require(bar.state->dragIsMouse && !bar.state->dragPath.empty(),
            "mouse press did not set physical-button recovery mode");
    const int physicalButton = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
    fixture::keys[physicalButton] = static_cast<SHORT>(0x8000);
    Require(DragInProgress(*bar.state), "physical mouse button down did not retain drag");
    fixture::keys[physicalButton] = 0;
    Require(!DragInProgress(*bar.state) && bar.state->dragPath.empty() && !bar.state->dragIsMouse,
            "lost mouse release did not clear drag after physical button up");
    pointer_observer::Clear();
    fixture::Reset();
    std::puts("PASS: recent pointer promotion/removal and physical mouse release recovery");
}
