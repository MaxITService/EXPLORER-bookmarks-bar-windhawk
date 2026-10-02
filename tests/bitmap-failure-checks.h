#pragma once
#include <roapi.h>
#include <winstring.h>

namespace bitmap_failure {
namespace streams = winrt::Windows::Storage::Streams;
struct State {
    bool factoryError = false, pixelError = false, lengthError = false;
    bool noByteAccess = false, nullDestination = false, invalidateError = false;
    HRESULT bufferStatus = S_OK;
    unsigned length = 16, live = 0, creates = 0, accesses = 0, invalidates = 0;
    std::array<BYTE, 16> bytes{};
    State() { bytes.fill(0xcc); }
};
struct PlainBuffer : winrt::implements<PlainBuffer, streams::IBuffer> {
    State& state;
    explicit PlainBuffer(State& value) : state(value) { ++state.live; }
    ~PlainBuffer() { --state.live; }
    uint32_t Capacity() { return 16; }
    uint32_t Length() { if (state.lengthError) winrt::throw_hresult(E_FAIL); return state.length; }
    void Length(uint32_t value) { if (value > Capacity()) winrt::throw_hresult(E_BOUNDS); state.length = value; }
};
struct PixelBuffer : winrt::implements<PixelBuffer, streams::IBuffer,
        ::Windows::Storage::Streams::IBufferByteAccess> {
    State& state;
    explicit PixelBuffer(State& value) : state(value) { ++state.live; }
    ~PixelBuffer() { --state.live; }
    uint32_t Capacity() { return 16; }
    uint32_t Length() { if (state.lengthError) winrt::throw_hresult(E_FAIL); return state.length; }
    void Length(uint32_t value) { if (value > Capacity()) winrt::throw_hresult(E_BOUNDS); state.length = value; }
    HRESULT STDMETHODCALLTYPE Buffer(BYTE** destination) noexcept override {
        ++state.accesses;
        *destination = nullptr;
        if (FAILED(state.bufferStatus)) return state.bufferStatus;
        if (!state.nullDestination) *destination = state.bytes.data();
        return S_OK;
    }
};
struct Bitmap : winrt::implements<Bitmap, muxmi::IWriteableBitmap> {
    State& state;
    streams::IBuffer buffer{nullptr};
    explicit Bitmap(State& value) : state(value) {
        buffer = state.noByteAccess ? winrt::make<PlainBuffer>(state) : winrt::make<bitmap_failure::PixelBuffer>(state);
        ++state.live;
    }
    ~Bitmap() { --state.live; }
    streams::IBuffer PixelBuffer() { if (state.pixelError) winrt::throw_hresult(E_FAIL); return buffer; }
    void Invalidate() {
        ++state.invalidates;
        if (state.invalidateError) winrt::throw_hresult(E_FAIL);
    }
};
struct Factory : winrt::implements<Factory, muxmi::IWriteableBitmapFactory, winrt::non_agile> {
    State& state;
    explicit Factory(State& value) : state(value) {}
    muxmi::WriteableBitmap CreateInstanceWithDimensions(int32_t width, int32_t height) {
        ++state.creates;
        if (state.factoryError) winrt::throw_hresult(E_OUTOFMEMORY);
        if (width != 2 || height != 2) winrt::throw_hresult(E_INVALIDARG);
        return winrt::make<Bitmap>(state).as<muxmi::WriteableBitmap>();
    }
};

// The local C++/WinRT SDK exposes this activation handler. Replace only this
// class's factory and preserve the prior handler for every other class. Run
// this group before the first production bitmap activation in the process.
// Non-agile factories bypass the SDK cache, so restoration does not require
// flushing unrelated runtime factories.
struct Scope {
    static inline Scope* active = nullptr;
    decltype(winrt_activation_handler) prior = winrt_activation_handler;
    muxmi::IWriteableBitmapFactory factory;
    explicit Scope(State& state) : factory(winrt::make<Factory>(state)) {
        Require(!active, "nested bitmap factory fault scope");
        active = this;
        winrt_activation_handler = Activate;
    }
    ~Scope() {
        winrt_activation_handler = prior;
        active = nullptr;
    }
    static int32_t __stdcall Activate(void* classId, const winrt::guid& iid, void** result) noexcept {
        const auto name = WindowsGetStringRawBuffer(static_cast<HSTRING>(classId), nullptr);
        if (std::wcscmp(name, L"Microsoft.UI.Xaml.Media.Imaging.WriteableBitmap") == 0) {
            return reinterpret_cast<IUnknown*>(winrt::get_abi(active->factory))->QueryInterface(iid, result);
        }
        if (active->prior) return active->prior(classId, iid, result);
        return RoGetActivationFactory(static_cast<HSTRING>(classId), iid, result);
    }
};
}

void CheckBitmapApiFailureRecovery() {
    const IconPixels pixels{2, 2, std::vector<BYTE>(16, 0x5a)};
    using bitmap_failure::State;
    for (unsigned scenario = 0; scenario < 8; ++scenario) {
        State state;
        switch (scenario) {
        case 0: state.factoryError = true; break;
        case 1: state.pixelError = true; break;
        case 2: state.lengthError = true; break;
        case 3: state.length = 15; break;
        case 4: state.noByteAccess = true; break;
        case 5: state.bufferStatus = E_FAIL; break;
        case 6: state.nullDestination = true; break;
        case 7: state.invalidateError = true; break;
        }
        {
            bitmap_failure::Scope scope(state);
            Require(!BitmapFromPixels(pixels) && state.creates == 1 && !state.live,
                    "bitmap API failure retained bitmap/buffer ownership or escaped its guard");
            Require(state.accesses == (scenario >= 5 ? 1u : 0u) &&
                    state.invalidates == (scenario == 7 ? 1u : 0u),
                    "bitmap failure continued beyond the failed API stage");
            const auto expected = scenario == 7 ? BYTE{0x5a} : BYTE{0xcc};
            Require(std::all_of(state.bytes.begin(), state.bytes.end(),
                               [&](BYTE value) { return value == expected; }),
                    "bitmap failure wrote pixels before receiving a valid destination");
            state.factoryError = state.pixelError = state.lengthError = false;
            state.noByteAccess = state.nullDestination = state.invalidateError = false;
            state.bufferStatus = S_OK;
            state.length = 16;
            auto recovered = BitmapFromPixels(pixels);
            Require(recovered && state.live == 2 && state.creates == 2 &&
                    std::all_of(state.bytes.begin(), state.bytes.end(), [](BYTE value) { return value == 0x5a; }),
                    "bitmap creation failed to recover or copy exact pixels after an API fault");
            recovered = nullptr;
            Require(!state.live, "recovered bitmap retained its buffer after release");
        }
    }
    auto real = BitmapFromPixels(pixels);
    Require(real && real.PixelWidth() == 2 && real.PixelHeight() == 2,
            "bitmap factory fault scope failed to restore real WinUI activation");
    std::puts("PASS: bitmap factory/buffer/interface/access/invalidation errors release ownership and recover");
}
