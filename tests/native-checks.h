#pragma once
HRESULT sizeResult = S_OK;
LONG sizeHeight = 136;
HRESULT WINAPI TestDesiredSize(void*, SIZE* size) {
    if (size) *size = {200, sizeHeight};
    return sizeResult;
}
unsigned nativeCalls = 0;
void CountNativeCall() { ++nativeCalls; }
HMODULE WINAPI TestLibraryLoad(LPCWSTR, HANDLE, DWORD) { return GetModuleHandleW(nullptr); }
void WINAPI TestCommandSetter(void*, void*) { ++nativeCalls; }

void CheckHookCoordinators() {
    fixture::Reset();
    g_desiredSizeOriginal = TestDesiredSize;
    g_frameRows = 0;
    SIZE size{};
    Require(DesiredSizeHook(nullptr, &size) == S_OK && size.cy == 136, "zero-row size hook changed stock height");
    for (unsigned rows = 1; rows <= 6; ++rows) {
        g_frameRows = rows;
        sizeHeight = 136;
        DesiredSizeHook(nullptr, &size);
        Require(size.cy == 136 + HostExtraAt96Dpi(std::min(rows, 4u)), "size hook row clamp changed confirmed geometry");
    }
    for (const auto height : {79, 80, 600, 601}) {
        sizeHeight = height;
        g_frameRows = 1;
        DesiredSizeHook(nullptr, &size);
        Require(size.cy == height + ((height >= 80 && height <= 600) ? MulDiv(height, 44, 136) : 0),
                "size hook eligible height boundary failed");
    }
    sizeResult = E_FAIL;
    Require(DesiredSizeHook(nullptr, &size) == E_FAIL && size.cy == 601, "failed size getter result changed");
    sizeResult = S_OK;
    Require(DesiredSizeHook(nullptr, nullptr) == S_OK, "null size output mishandled");
    g_unloading = true;
    sizeHeight = 136;
    DesiredSizeHook(nullptr, &size);
    Require(size.cy == 136, "size hook active during unload");
    g_unloading = false;
    g_frameRows = 0;
    g_frameHooked = false;
    g_extensionHooked = false;
    g_frameHookAttempted = false;
    g_extensionHookAttempted = false;
    fixture::modules[L"Windows.UI.FileExplorer.dll"] = nullptr;
    fixture::modules[L"FileExplorerExtensions.dll"] = nullptr;
    Require(HookExplorerFrame(true) && HookExplorerExtension(true) && fixture::symbolCalls == 0,
            "absent modules attempted symbol lookup");
    fixture::modules[L"Windows.UI.FileExplorer.dll"] = GetModuleHandleW(nullptr);
    fixture::modules[L"FileExplorerExtensions.dll"] = GetModuleHandleW(nullptr);
    Require(!HookExplorerFrame(true) && !HookExplorerExtension(true) && fixture::symbolCalls == 2,
            "failed symbol lookup reported installed hooks");
    Require(!HookExplorerFrame(true) && !HookExplorerExtension(true) && fixture::symbolCalls == 2,
            "failed symbol resolution retried");
    g_frameHookAttempted = false;
    g_extensionHookAttempted = false;
    fixture::symbolSuccess = true;
    g_commandBarSetterOriginal = TestCommandSetter;
    Require(HookExplorerFrame(true) && HookExplorerExtension(false) && g_frameHooked &&
            g_extensionHooked && fixture::hookApplications == 1,
            "successful symbol hooks did not publish/apply correctly");
    const auto resolved = fixture::symbolCalls;
    Require(HookExplorerFrame(true) && HookExplorerExtension(true) && fixture::symbolCalls == resolved,
            "installed hooks looked up symbols again");
    g_frameHooked = false;
    g_extensionHooked = false;
    g_frameHookAttempted = false;
    g_extensionHookAttempted = false;
    g_loadLibraryOriginal = TestLibraryLoad;
    LoadLibraryExWHook(L"C:\\test\\Windows.UI.FileExplorer.dll", nullptr, 0);
    LoadLibraryExWHook(L"C:/test/FileExplorerExtensions.dll", nullptr, 0);
    Require(g_frameHooked && g_extensionHooked, "library observation did not recognize both path separators");
    LoadLibraryExWHook(nullptr, nullptr, 0);
    g_unloading = true;
    LoadLibraryExWHook(L"Windows.UI.FileExplorer.dll", nullptr, 0);
    g_unloading = false;
    g_commandBarSetterOriginal = TestCommandSetter;
    nativeCalls = 0;
    CommandBarSetterHook(nullptr, nullptr);
    Require(nativeCalls == 1, "command setter wrapper skipped original");
    fixture::hookSuccess = true;
    Require(Wh_ModInit() && g_storageMutex.processMutex, "stubbed successful initialization failed");
    Wh_ModAfterInit();
    Wh_ModUninit();
    Require(!g_storageMutex.processMutex, "successful init/uninit leaked named storage mutex");
    g_unloading = false;
    g_frameHooked = false;
    g_extensionHooked = false;
    g_frameHookAttempted = false;
    g_extensionHookAttempted = false;
    g_desiredSizeOriginal = nullptr;
    g_commandBarSetterOriginal = nullptr;
    fixture::Reset();
    std::puts("PASS: desired-size boundaries, one-attempt symbol hooks, library observation and init/uninit");
}

void CheckNativeThreadRouting() {
    HWND frame = CreateShellWindow(L"CabinetWClass");
    nativeCalls = 0;
    RunOnWindowThread(nullptr, CountNativeCall);
    Require(nativeCalls == 0, "null native window callback executed");
    RunOnWindowThread(frame, CountNativeCall);
    Require(nativeCalls == 1, "same-thread native callback not executed");
    ForExplorerWindows(CountNativeCall);
    Require(nativeCalls == 2, "own-process Explorer-class frame enumeration missed fixture");
    RelayoutThreadFrames();
    PostFrameRelayout();
    std::atomic<DWORD> callbackOwner = 0;
    HANDLE ready = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    std::atomic<HWND> foreignFrame = nullptr;
    std::thread thread([&] {
        const auto window = CreateShellWindow(L"CabinetWClass");
        foreignFrame = window;
        SetEvent(ready);
        MSG message;
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (message.message == WM_APP) { DestroyWindow(window); break; }
            DispatchMessageW(&message);
        }
    });
    Require(WaitForSingleObject(ready, 5000) == WAIT_OBJECT_0, "native route thread not ready");
    static std::atomic<DWORD>* owner;
    owner = &callbackOwner;
    RunOnWindowThread(foreignFrame, [] { *owner = GetCurrentThreadId(); });
    Require(callbackOwner == GetWindowThreadProcessId(foreignFrame, nullptr),
            "cross-thread callback ran on wrong owner");
    PostThreadMessageW(callbackOwner, WM_APP, 0, 0);
    thread.join();
    CloseHandle(ready);
    DestroyWindow(frame);
    std::puts("PASS: own-process enumeration and same/cross-thread synchronous callback routing");
}

void CheckStorageAbandonmentAndFailure() {
    StorageMutex mutex;
    mutex.processMutex = CreateMutexW(nullptr, FALSE, nullptr);
    Require(mutex.processMutex != nullptr, "storage test mutex creation failed");
    std::thread owner([&] { Require(WaitForSingleObject(mutex.processMutex, 5000) == WAIT_OBJECT_0,
                                 "abandoned mutex acquisition failed"); });
    owner.join(); // Deliberately abandon only this unnamed test mutex.
    mutex.lock();
    mutex.unlock();
    // A mutex that another owner keeps holding times out instead of freezing
    // the caller, and the timed-out wait releases the local mutex.
    HANDLE held = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE release = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    Require(held && release, "storage timeout test events failed");
    std::thread holder([&] {
        Require(WaitForSingleObject(mutex.processMutex, 5000) == WAIT_OBJECT_0,
                "storage timeout holder did not take the mutex");
        SetEvent(held);
        WaitForSingleObject(release, 15000);
        ReleaseMutex(mutex.processMutex);
    });
    Require(WaitForSingleObject(held, 5000) == WAIT_OBJECT_0, "storage timeout holder did not start");
    const auto started = GetTickCount64();
    winrt::hresult timedOut;
    try { mutex.lock(); } catch (const winrt::hresult_error& error) { timedOut = error.code(); }
    const auto waited = GetTickCount64() - started;
    SetEvent(release);
    holder.join();
    CloseHandle(held);
    CloseHandle(release);
    Require(timedOut == HRESULT_FROM_WIN32(ERROR_TIMEOUT) &&
            waited + 100 >= StorageMutex::kWaitMs && waited < StorageMutex::kWaitMs + 2000,
            "held storage mutex did not time out with ERROR_TIMEOUT");
    Require(mutex.local.try_lock(), "timed-out storage wait kept the local mutex");
    mutex.local.unlock();
    CloseHandle(mutex.processMutex);
    mutex.processMutex = CreateEventW(nullptr, FALSE, TRUE, nullptr);
    CloseHandle(mutex.processMutex); // Deliberately invalid test handle.
    bool threw = false;
    try { mutex.lock(); } catch (const winrt::hresult_error&) { threw = true; }
    Require(threw, "invalid storage handle did not fail");
    mutex.processMutex = nullptr;
    mutex.lock(); // A failed named wait must release the local mutex.
    mutex.unlock();
    std::puts("PASS: abandoned storage mutex recovery, held-mutex timeout and failed-wait local-lock release");
}
