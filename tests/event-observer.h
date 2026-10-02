#pragma once

// Included only by the verified TEMP source copy. Capture the concrete WinRT
// delegate passed to the original event registration, then observe it after
// registration succeeds. Handler bodies and GuardHandler remain unchanged.
namespace event_observer {
namespace wf = winrt::Windows::Foundation;
using Opening = wf::EventHandler<wf::IInspectable>;
using Context = wf::TypedEventHandler<mux::UIElement, muxi::ContextRequestedEventArgs>;
struct Registration {
    winrt::weak_ref<wf::IInspectable> target;
    std::wstring name;
    mux::RoutedEventHandler click{nullptr};
    Context context{nullptr};
    Opening opening{nullptr};
    muxi::PointerEventHandler pointer{nullptr};
    mux::DragEventHandler drag{nullptr};
};
inline thread_local std::vector<Registration> registrations;

template<typename Element, typename Delegate>
void Observe(const Element& element, const wchar_t* name, const Delegate& handler) {
    std::erase_if(registrations, [](const auto& entry) { return !entry.target.get(); });
    Registration entry{winrt::make_weak(element.template as<wf::IInspectable>()), name};
    if constexpr (std::is_same_v<Delegate, mux::RoutedEventHandler>) entry.click = handler;
    else if constexpr (std::is_same_v<Delegate, Context>) entry.context = handler;
    else if constexpr (std::is_same_v<Delegate, Opening>) entry.opening = handler;
    else if constexpr (std::is_same_v<Delegate, muxi::PointerEventHandler>) entry.pointer = handler;
    else if constexpr (std::is_same_v<Delegate, mux::DragEventHandler>) entry.drag = handler;
    else static_assert(sizeof(Delegate) == 0, "unrecognized observed event type");
    registrations.push_back(std::move(entry));
}

template<typename Element>
Registration Find(const Element& element, const wchar_t* name) {
    auto target = element.template as<wf::IInspectable>();
    for (auto it = registrations.rbegin(); it != registrations.rend(); ++it) {
        if (it->target.get() == target && it->name == name) return *it;
    }
    return {};
}
inline void Clear() { registrations.clear(); }
}

template<typename Delegate, typename Handler>
Delegate CaptureObserved(Delegate& captured, Handler&& handler) {
    captured = Delegate{std::forward<Handler>(handler)};
    return captured;
}

template<typename Element, typename Delegate>
void ObserveEvent(const Element& element, const wchar_t* name, const Delegate& handler) {
    event_observer::Observe(element, name, handler);
}
