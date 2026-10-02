#pragma once

// Included solely by the generated TEMP mod copy. Observe exactly the guarded
// delegate which production registers, without replacing AddHandler or changing
// its handler. The repository's mod source is never edited.
namespace pointer_observer {
struct Registration {
    winrt::weak_ref<muxc::Button> button;
    mux::RoutedEvent event{nullptr};
    muxi::PointerEventHandler handler{nullptr};
};
inline thread_local std::vector<Registration> registrations;
inline muxi::PointerEventHandler Handler(const muxc::Button& button, const mux::RoutedEvent& event) {
    for (auto current = registrations.rbegin(); current != registrations.rend(); ++current) {
        if (current->button.get() == button && current->event == event) return current->handler;
    }
    return nullptr;
}
inline void Clear() { registrations.clear(); }

// Public IPointer doubles are valid callback arguments, but they are not
// native WinUI input contacts. Never pass the named fixture into WinUI's
// internal capture machinery. Real routed pointers retain native behavior.
inline bool CapturePointer(const muxc::Button& button, const muxi::Pointer& pointer) {
    if (winrt::get_class_name(pointer) == L"Bookmarks.Tests.Pointer") return false;
    return button.CapturePointer(pointer);
}
}

inline void ObservePointer(const muxc::Button& button, const mux::RoutedEvent& event,
                           const winrt::Windows::Foundation::IInspectable& boxed) {
    std::erase_if(pointer_observer::registrations, [](const auto& entry) { return !entry.button.get(); });
    pointer_observer::registrations.push_back({winrt::make_weak(button), event,
                                              winrt::unbox_value<muxi::PointerEventHandler>(boxed)});
}
