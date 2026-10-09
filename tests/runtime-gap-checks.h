#pragma once
// Native integration boundaries, using only hidden owned windows, in-memory
// Windhawk data and isolated TEMP files. No Explorer hooks or navigation.

namespace runtime_gap {
// A dropped-item read that finishes only on its third cancellation.
struct StubbornRead : winrt::implements<StubbornRead, winrt::Windows::Foundation::IAsyncInfo> {
    unsigned cancels = 0;
    uint32_t Id() { return 2; }
    winrt::Windows::Foundation::AsyncStatus Status() {
        return cancels >= 3 ? winrt::Windows::Foundation::AsyncStatus::Canceled
                            : winrt::Windows::Foundation::AsyncStatus::Started;
    }
    winrt::hresult ErrorCode() { return S_OK; }
    void Cancel() {
        if (++cancels == 3) --g_pendingDrops;
    }
    void Close() {}
};
bool SameRecentSettings(const RecentSettings& a, const RecentSettings& b) {
    return a.enabled == b.enabled && a.buttons == b.buttons &&
           a.menuButton == b.menuButton && a.history == b.history &&
           a.rightClickClears == b.rightClickClears;
}
bool SameFxFolders(const std::vector<FxFolder>& a, const std::vector<FxFolder>& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
        [](const FxFolder& x, const FxFolder& y) { return x.path == y.path && x.label == y.label; });
}
// This native fixture owns no live UI/worker at entry or exit. Restore both
// publication and TLS snapshots coherently so initialization faults cannot
// contaminate whichever existing group runs next.
struct SettingsScope {
    RecentSettings sharedRecent;
    std::vector<FxFolder> sharedFx;
    bool sharedHotkey;
    bool sharedParentForDuplicates;
    ULONGLONG generation;
    RecentSettings recent = g_recentSettings;
    std::vector<FxFolder> fx = g_fxFolders;
    bool hotkey = g_bookmarkHotkey;
    bool parentForDuplicates = g_parentForDuplicates;
    ULONGLONG threadGeneration = g_threadSettingsGeneration;
    bool applyPending = g_settingsApplyPending;
    SettingsScope() {
        std::lock_guard lock(g_settingsMutex);
        sharedRecent = g_sharedRecentSettings;
        sharedFx = g_sharedFxFolders;
        sharedHotkey = g_sharedBookmarkHotkey;
        sharedParentForDuplicates = g_sharedParentForDuplicates;
        generation = g_settingsGeneration;
    }
    ~SettingsScope() {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings = sharedRecent;
        g_sharedFxFolders = std::move(sharedFx);
        g_sharedBookmarkHotkey = sharedHotkey;
        g_sharedParentForDuplicates = sharedParentForDuplicates;
        g_settingsGeneration = generation;
        g_recentSettings = recent;
        g_fxFolders = std::move(fx);
        g_bookmarkHotkey = hotkey;
        g_parentForDuplicates = parentForDuplicates;
        g_threadSettingsGeneration = threadGeneration;
        g_settingsApplyPending = applyPending;
    }
};
struct Web : FakeWeb {
    bool webInterface = true;
    bool serviceInterface = true;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if ((iid == IID_IWebBrowser2 && !webInterface) ||
            (iid == IID_IServiceProvider && !serviceInterface)) {
            *out = nullptr;
            return E_NOINTERFACE;
        }
        return FakeWeb::QueryInterface(iid, out);
    }
};
struct VirtualPersist : FakePersist {
    VirtualPersist() : FakePersist(L"") {}
    HRESULT STDMETHODCALLTYPE GetCurFolder(PIDLIST_ABSOLUTE* out) override {
        // This PC is a real virtual Shell location with no filesystem path.
        return SHGetSpecialFolderLocation(nullptr, CSIDL_DRIVES, out);
    }
};
struct EmptyItems : FakeFolderView {
    HRESULT STDMETHODCALLTYPE Items(UINT, REFIID, void** out) override {
        *out = nullptr;
        return S_OK;
    }
};
struct EmptyName : FakeItem {
    EmptyName() : FakeItem(L"") {}
    HRESULT STDMETHODCALLTYPE GetDisplayName(SIGDN, LPWSTR* out) override {
        *out = nullptr;
        return S_OK;
    }
};
}

void CheckShellDiscoveryAndSelectionGaps() {
    runtime_gap::Scope faultScope;
    BackupFixture scratch;
    ShellFixture shell;
    auto web = NewFake<runtime_gap::Web>();
    web->frame = shell.frame;
    web->service = shell.service.as<IServiceProvider>();
    web->container = shell.tabContainer.as<IConnectionPointContainer>();
    fakeWindows->windows = {web.as<IDispatch>()};
    auto persist = NewFake<FakePersist>(scratch.directory);
    shell.folder->persist = persist.as<IPersistFolder2>();
    for (auto* allowed : {&web->webInterface, &web->serviceInterface}) {
        *allowed = false;
        Require(!ActiveShellBrowser(shell.frame),
                "unsupported browser/provider interface yielded an active tab");
        *allowed = true;
    }
    web->frame = shell.tab;
    Require(!ActiveShellBrowser(shell.frame), "browser from another frame selected");
    web->frame = shell.frame;
    fakeWindows->windows.insert(fakeWindows->windows.begin(), nullptr);
    Require(ActiveShellBrowser(shell.frame).get() == shell.browser.get(),
            "null ShellWindows entry prevented later matching tab discovery");
    fakeWindows->windows.erase(fakeWindows->windows.begin());
    DestroyWindow(shell.tab);
    shell.tab = nullptr;
    const HWND fallback = CreateShellWindow(L"BookmarksVisibilityFallback", shell.frame);
    shell.browser->window = fallback;
    Require(!ActiveShellBrowser(shell.frame), "hidden fallback browser selected");
    runtime_gap::faults.overrideVisibility = true;
    runtime_gap::faults.visibleTab = fallback; // Model Shell's visible active tab; window stays hidden.
    Require(ActiveShellBrowser(shell.frame).get() == shell.browser.get(),
            "visible fallback browser rejected without ShellTabWindowClass");
    runtime_gap::faults.overrideVisibility = false;
    // Restore the normal active-tab route for the selection boundaries.
    shell.tab = CreateShellWindow(L"ShellTabWindowClass", shell.frame);
    shell.browser->window = shell.tab;
    shell.browser->view = nullptr;
    Require(ShellBrowserFolder(shell.browser.get()).empty() &&
            SelectedFolders(shell.frame).empty(), "empty active Shell view exposed folders");
    shell.browser->view = shell.view.as<IShellView>();
    shell.view->folder = nullptr;
    Require(CurrentFolder(shell.frame).empty() && SelectedFolders(shell.frame).empty(),
            "view without IFolderView exposed folders");
    shell.view->folder = shell.folder.as<IFolderView>();
    auto virtualPersist = NewFake<runtime_gap::VirtualPersist>();
    shell.folder->persist = virtualPersist.as<IPersistFolder2>();
    Require(CurrentFolder(shell.frame).empty(), "virtual Shell PIDL exposed a filesystem folder");
    shell.folder->persist = persist.as<IPersistFolder2>();
    auto emptyItems = NewFake<runtime_gap::EmptyItems>();
    shell.view->folder = emptyItems.as<IFolderView>();
    Require(SelectedFolders(shell.frame).empty(), "null selection array exposed a folder");
    shell.view->folder = shell.folder.as<IFolderView>();
    auto item = NewFake<FakeItem>(scratch.directory);
    shell.items->items = {item.as<IShellItem>()};
    shell.items->itemResult = E_FAIL;
    Require(SelectedFolders(shell.frame).empty(), "failed individual selection item accepted");
    shell.items->itemResult = S_OK;
    item->attributeResult = E_FAIL;
    Require(SelectedFolders(shell.frame).empty(), "failed individual selection attributes accepted");
    item->attributeResult = S_OK;
    auto emptyName = NewFake<runtime_gap::EmptyName>();
    shell.items->items = {emptyName.as<IShellItem>()};
    Require(SelectedFolders(shell.frame).empty(), "null filesystem display name accepted");
    // A bookmark can remain after its local directory is deleted. Parse it
    // through the real Shell, but route any navigation to our fake browser.
    shell.browser->navigated.clear();
    NavigateToFolder(shell.frame, scratch.directory + L"\\deleted-bookmark");
    Require(shell.browser->navigated.empty(), "missing bookmark navigated the active tab");
    NavigateToFolder(shell.frame, scratch.directory);
    Require(shell.browser->navigated == std::vector<std::wstring>{scratch.directory},
            "valid bookmark failed after a missing target");
    shell.browser->navigated.clear();
    fakeWindows->windows.clear();
    Require(CurrentFolder(shell.frame).empty(), "absent browser produced current folder");
    NavigateToFolder(shell.frame, scratch.directory);
    Require(shell.browser->navigated.empty(), "absent browser caused navigation");
    DestroyWindow(fallback);
    std::puts("PASS: Shell QI/frame/null entries, visibility fallback, virtual PIDL and selection failure edges");
}

void CheckNavigationEventAndTrackerGaps() {
    BackupFixture scratch;
    ShellFixture shell;
    auto persist = NewFake<FakePersist>(scratch.directory);
    shell.folder->persist = persist.as<IPersistFolder2>();
    auto noProvider = NewFake<runtime_gap::Web>();
    noProvider->serviceInterface = false;
    Require(!RecordNavigation(noProvider.get(), kDispidDocumentComplete),
            "navigation recorded without a service provider");
    shell.service->result = E_FAIL;
    Require(!RecordNavigation(shell.web.get(), kDispidNavigateComplete2),
            "failed navigation browser service changed history");
    shell.service->result = S_OK;
    const auto oldRecent = g_sharedRecentSettings;
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings.enabled = true;
        ++g_settingsGeneration;
    }
    UpdateThreadSettings();
    StopThreadWork();
    VARIANT arguments[2]{};
    DISPPARAMS parameters{arguments, nullptr, 2, 0};
    arguments[1].vt = VT_I4;
    OnShellEvent(kDispidDocumentComplete, &parameters);
    Require(!g_refreshPending && fixture::writes == 0,
            "navigation with a nondispatch first argument wrote history");
    arguments[1].vt = VT_DISPATCH;
    arguments[1].pdispVal = nullptr;
    OnShellEvent(kDispidNavigateComplete2, &parameters);
    Require(!g_refreshPending && fixture::writes == 0,
            "navigation with null dispatch wrote history");
    arguments[1].pdispVal = shell.web.get();
    OnShellEvent(kDispidNavigateComplete2, &parameters);
    Require(g_refreshPending && !g_recentSyncPending && g_workTimer &&
            fixture::storage[g_recentsKey] == scratch.directory,
            "valid navigation event lost deferred refresh or recorded wrong argument");
    StopThreadWork();
    const auto writes = fixture::writes;
    OnShellEvent(kDispidDocumentComplete, &parameters);
    Require(!g_refreshPending && fixture::writes == writes,
            "duplicate completion event rewrote history or requested a refresh");
    OnShellEvent(-4567, &parameters);
    Require(!g_workTimer, "unrelated Shell event requested work");
    OnShellEvent(kDispidWindowRevoked, nullptr);
    Require(g_recentSyncPending && !g_refreshPending && g_workTimer,
            "revocation event failed to defer subscription sync");
    StopThreadWork();
    g_recentSettings.enabled = false;
    OnShellEvent(kDispidDocumentComplete, &parameters);
    Require(!g_workTimer && fixture::writes == writes,
            "disabled recents consumed navigation event");
    g_recentSettings.enabled = true;
    {
        allocation_fixture::Scope allocation(32768 * sizeof(wchar_t));
        OnShellEvent(kDispidDocumentComplete, &parameters);
        Require(allocation.Triggered() && !g_refreshPending && !g_workTimer &&
                fixture::writes == writes,
                "navigation path-allocation failure escaped the event guard or changed history");
    }
    auto noContainer = NewFake<runtime_gap::Web>();
    winrt::com_ptr<IConnectionPoint> point;
    DWORD cookie = 0;
    auto sink = NewFake<ShellEventSink>();
    Require(!ConnectEvents(noContainer.get(), kWebBrowserEvents2, sink.get(), point, cookie) &&
            !point, "missing connection-point container retained subscription");
    Require(EnsureRecentTracker(), "navigation fixture tracker startup failed");
    fakeWindows->itemResult = E_FAIL;
    SyncBrowserConnections(*g_recentTracker);
    Require(g_recentTracker->browsers.empty(), "failed ShellWindows item subscribed");
    fakeWindows->itemResult = S_OK;
    fakeWindows->windows.insert(fakeWindows->windows.begin(), nullptr);
    shell.web->windowResult = E_FAIL;
    SyncBrowserConnections(*g_recentTracker);
    Require(g_recentTracker->browsers.empty(), "failed browser HWND subscribed");
    shell.web->windowResult = S_OK;
    shell.web->frame = nullptr;
    SyncBrowserConnections(*g_recentTracker);
    Require(g_recentTracker->browsers.empty(), "browser without owner thread subscribed");
    shell.web->frame = shell.frame;
    shell.tabPoint->adviseResult = E_FAIL;
    SyncBrowserConnections(*g_recentTracker);
    Require(g_recentTracker->browsers.empty() && !shell.tabPoint->sink,
            "failed tab subscription retained connection or sink");
    shell.tabPoint->adviseResult = S_OK;
    SyncBrowserConnections(*g_recentTracker);
    Require(g_recentTracker->browsers.size() == 1,
            "later successful tab subscription failed after prior errors");
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings = oldRecent;
        ++g_settingsGeneration;
    }
    UpdateThreadSettings();
    std::puts("PASS: real Shell event argument/duplicate/disabled paths and subscription discovery failure recovery");
}

void CheckDeferredWorkAndSettingsGaps() {
    runtime_gap::Scope faultScope;
    BackupFixture scratch;
    ShellFixture shell;
    auto persist = NewFake<FakePersist>(scratch.directory);
    shell.folder->persist = persist.as<IPersistFolder2>();
    Require(g_bars && g_bars->empty() && !g_activeOperations,
            "deferred fixture began with live bars or operations");
    const auto oldRecent = g_sharedRecentSettings;
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings.enabled = false;
        ++g_settingsGeneration;
    }
    // Let the coordinator itself observe/apply a new settings generation.
    g_bookmarkTogglePending = shell.frame;
    g_bookmarkToggleFromFileList = false;
    RunThreadWork();
    Require(!g_settingsApplyPending && !g_bookmarkTogglePending && !g_refreshPending &&
            !g_recentSyncPending && g_pendingNotice.kind == BookmarkNotice::None &&
            !g_threadWorking && !g_activeOperations &&
            fixture::storage[g_bookmarksKey] == scratch.directory,
            "settings/bookmark/refresh/notice deferred work did not drain completely");
    g_bookmarkTogglePending = shell.frame;
    g_bookmarkToggleFromFileList = true;
    RunThreadWork();
    Require(ReadStorageLocked().empty() && !g_bookmarkTogglePending && !g_refreshPending &&
            g_pendingNotice.kind == BookmarkNotice::None,
            "deferred file-list fallback failed to toggle existing bookmark");
    fakeWindows->windows.clear();
    const auto writes = fixture::writes;
    g_bookmarkTogglePending = shell.frame;
    RunThreadWork();
    Require(!g_bookmarkTogglePending && fixture::writes == writes &&
            g_pendingNotice.kind == BookmarkNotice::None,
            "virtual/absent-current-folder deferred toggle wrote data or notice");
    g_recentSyncPending = true;
    g_refreshPending = true;
    g_threadClosing = true;
    RunThreadWork();
    Require(g_recentSyncPending && g_refreshPending && !g_threadWorking && !g_activeOperations,
            "closing UI thread consumed pending deferred work or leaked operation");
    g_threadClosing = false;
    StopThreadWork();
    // Real StorageMutex failure throws before work starts. The coordinator must
    // release its operation and allow a later run after the handle is repaired.
    Require(!g_storageMutex.processMutex, "deferred fixture unexpectedly owns storage handle");
    const HANDLE failedMutex = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(failedMutex && CloseHandle(failedMutex), "deferred invalid-handle fixture setup failed");
    g_storageMutex.processMutex = failedMutex;
    RunThreadWork();
    Require(!g_threadWorking && !g_activeOperations,
            "deferred storage-lock exception leaked worker or operation state");
    g_storageMutex.processMutex = nullptr;
    const auto oldFx = g_sharedFxFolders;
    std::vector<FxFolder> previousThreadFx;
    previousThreadFx.swap(g_fxFolders);
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedFxFolders = {{L"Recovery", L"C:\\allocation-recovery"}};
        ++g_settingsGeneration;
    }
    {
        allocation_fixture::Scope allocation(sizeof(FxFolder));
        RunThreadWork();
        Require(allocation.Triggered() && !g_threadWorking && !g_activeOperations &&
                g_threadSettingsGeneration != g_settingsGeneration.load(),
                "settings-copy allocation failure published its generation or leaked deferred operation");
    }
    RunThreadWork();
    Require(g_threadSettingsGeneration == g_settingsGeneration.load() &&
            g_fxFolders.size() == 1 && g_fxFolders.front().path == L"C:\\allocation-recovery" &&
            !g_settingsApplyPending, "settings-copy allocation recovery failed to apply pending generation");
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedFxFolders = oldFx;
        ++g_settingsGeneration;
    }
    UpdateThreadSettings();
    runtime_gap::faults.timer = true;
    ScheduleThreadWork();
    Require(!g_workTimer, "failed native work timer was published");
    runtime_gap::faults.timer = false;
    ScheduleThreadWork();
    Require(g_workTimer, "timer retry failed after native allocation fault");
    StopThreadWork();
    {
        std::lock_guard lock(g_settingsMutex);
        g_sharedRecentSettings = oldRecent;
        ++g_settingsGeneration;
    }
    UpdateThreadSettings();
    std::puts("PASS: deferred settings, bookmark toggles, refresh/notices, close guard, lock exception and timer retry");
}

void CheckNativeFrameAndInitializationGaps() {
    runtime_gap::SettingsScope settingsScope;
    runtime_gap::Scope faultScope;
    fixture::Reset();
    Require(g_frames.empty() && !g_messageHook && !g_storageMutex.processMutex,
            "native-fault fixture began with tracked frames/hooks/storage");
    if (!g_bars) g_bars.emplace();
    const HWND frame = CreateShellWindow(L"CabinetWClass");
    runtime_gap::faults.subclass = true;
    Require(!TrackFrame(frame) && g_frames.empty() && !GetPropW(frame, kFrameProperty),
            "failed native subclass left lifetime tracking or refresh property");
    runtime_gap::faults.subclass = false;
    runtime_gap::faults.property = true;
    Require(TrackFrame(frame) && g_frames == std::vector<HWND>{frame} &&
            !GetPropW(frame, kFrameProperty), "property failure lost working lifetime subclass");
    runtime_gap::faults.property = false;
    runtime_gap::faults.hook = true;
    InstallMessageHook();
    Require(!g_messageHook, "failed native thread hook was published");
    runtime_gap::faults.hook = false;
    InstallMessageHook();
    Require(g_messageHook, "native hook retry failed");
    RemoveMessageHook();
    HANDLE otherReady = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE otherFinished = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(otherReady && otherFinished, "native cross-thread failure gates failed");
    std::atomic<HWND> otherFrame = nullptr;
    std::thread other([&] {
        const HWND window = CreateShellWindow(L"BookmarksRuntimeOtherThread");
        otherFrame = window;
        SetEvent(otherReady);
        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (message.message == WM_APP) break;
            DispatchMessageW(&message);
        }
        DestroyWindow(window);
        SetEvent(otherFinished);
    });
    Require(WaitForSingleObject(otherReady, 5000) == WAIT_OBJECT_0,
            "native cross-thread failure fixture failed to initialize");
    const auto beforeFailedCall = nativeCalls;
    runtime_gap::faults.hook = true;
    RunOnWindowThread(otherFrame.load(), CountNativeCall);
    Require(nativeCalls == beforeFailedCall, "failed cross-thread hook executed callback");
    runtime_gap::faults.hook = false;
    Require(PostThreadMessageW(GetWindowThreadProcessId(otherFrame.load(), nullptr), WM_APP, 0, 0),
            "native cross-thread failure fixture exit message failed");
    Require(WaitForSingleObject(otherFinished, 5000) == WAIT_OBJECT_0,
            "native cross-thread failure fixture did not finish after exit message");
    other.join();
    CloseHandle(otherReady);
    CloseHandle(otherFinished);
    BookmarkChange notice;
    notice.noRoom = 1;
    SendMessageW(frame, RefreshBarsMessage(), EncodeNotice(notice), 0);
    Require(g_refreshPending && g_pendingNotice.kind == notice.Notice() && g_workTimer,
            "frame subclass refresh failed to transfer encoded notice");
    StopThreadWork();
    g_unloading = true;
    SendMessageW(frame, RefreshBarsMessage(), EncodeNotice(notice), 0);
    Require(!g_refreshPending && !g_workTimer, "unloading frame subclass queued refresh");
    g_unloading = false;
    // A real low-memory push failure while WM_NCDESTROY is in progress must be
    // contained by the native subclass procedure. The frame/property have
    // already been removed; this fixture owns no bar visuals requiring repair.
    std::vector<HWND>().swap(g_closedFrames);
    {
        allocation_fixture::Scope allocation(sizeof(HWND));
        DestroyWindow(frame);
        Require(allocation.Triggered() && g_frames.empty() && g_closedFrames.empty(),
                "frame-close allocation failure escaped its guard or retained frame tracking");
    }
    Require(g_frames.empty() && g_bars->empty(), "native-fault frame close leaked tracking");
    g_threadClosing = false;
    CWPSTRUCT unrelated{};
    unrelated.message = WM_NULL;
    const auto calls = nativeCalls;
    ThreadCallHook(HC_ACTION, 0, reinterpret_cast<LPARAM>(&unrelated));
    ThreadCallHook(-1, 0, 0);
    Require(nativeCalls == calls, "unrelated/negative native call hook invoked callback");
    // Unload may arrive while native frames are tracked but before XAML has
    // installed any bar. Exercise the real removal of both live subclasses.
    const HWND cleanupFirst = CreateShellWindow(L"CabinetWClass");
    const HWND cleanupSecond = CreateShellWindow(L"CabinetWClass");
    Require(TrackFrame(cleanupFirst) && TrackFrame(cleanupSecond) &&
            GetPropW(cleanupFirst, kFrameProperty) && GetPropW(cleanupSecond, kFrameProperty),
            "native cleanup fixture failed to track both live frames");
    g_refreshPending = true;
    ScheduleThreadWork();
    Require(g_workTimer, "native cleanup fixture failed to schedule pending work");
    g_unloading = true;
    CleanupCurrentThread();
    Require(g_frames.empty() && !g_bars && !g_workTimer && !g_refreshPending &&
            !g_recentTracker && !g_messageHook &&
            !GetPropW(cleanupFirst, kFrameProperty) && !GetPropW(cleanupSecond, kFrameProperty),
            "unload before bar installation retained native tracking or pending work");
    g_unloading = false;
    // With unloading admission open again, a stale subclass would schedule work.
    SendMessageW(cleanupFirst, RefreshBarsMessage(), 0, 0);
    SendMessageW(cleanupSecond, RefreshBarsMessage(), 0, 0);
    Require(!g_workTimer && !g_refreshPending,
            "native cleanup left a live frame refresh subclass installed");
    CleanupCurrentThread();
    g_bars.emplace();
    Require(TrackFrame(cleanupFirst), "frame tracking failed after native cleanup");
    DestroyWindow(cleanupFirst);
    DestroyWindow(cleanupSecond);
    Require(g_frames.empty() && g_bars->empty() && g_closedFrames.empty(),
            "reused native frame retained state after destruction");
    g_threadClosing = false;
    runtime_gap::faults.mutex = true;
    Require(!Wh_ModInit() && !g_storageMutex.processMutex, "failed init mutex published storage handle");
    runtime_gap::faults.mutex = false;
    fixture::modules[L"kernelbase.dll"] = nullptr;
    Require(!Wh_ModInit() && !g_storageMutex.processMutex, "absent kernelbase init leaked storage handle");
    fixture::modules.erase(L"kernelbase.dll");
    runtime_gap::faults.procedure = true;
    Require(!Wh_ModInit() && !g_storageMutex.processMutex, "absent LoadLibrary entry init leaked storage handle");
    runtime_gap::faults.procedure = false;
    fixture::strings[L"fxCustomFolders[0].path"] = L"C:\\settings-recovery";
    fixture::strings[L"fxCustomFolders[0].label"] = L"Recovery";
    LoadSettings(); // Populate setting-double keys before faulting real allocations.
    const auto settingsGeneration = g_settingsGeneration.load();
    const auto settingsFx = g_sharedFxFolders;
    const auto settingsRecent = g_sharedRecentSettings;
    const auto settingsHotkey = g_sharedBookmarkHotkey;
    {
        allocation_fixture::Scope allocation(sizeof(FxFolder));
        Wh_ModSettingsChanged();
        Require(allocation.Triggered() && g_settingsGeneration == settingsGeneration &&
                runtime_gap::SameFxFolders(g_sharedFxFolders, settingsFx) &&
                runtime_gap::SameRecentSettings(g_sharedRecentSettings, settingsRecent) &&
                g_sharedBookmarkHotkey == settingsHotkey,
                "settings callback allocation failure published partial settings");
    }
    fixture::hookSuccess = true;
    fixture::modules[L"Windows.UI.FileExplorer.dll"] = nullptr;
    fixture::modules[L"FileExplorerExtensions.dll"] = nullptr;
    runtime_gap::faults.library = true;
    g_frameHooked = g_extensionHooked = false;
    g_frameHookAttempted = g_extensionHookAttempted = false;
    Require(Wh_ModInit() && g_storageMutex.processMutex && runtime_gap::faults.libraryCalls == 1,
            "initialization did not survive unavailable Explorer library");
    Wh_ModUninit();
    g_unloading = false;
    g_frameHookAttempted = g_extensionHookAttempted = false;
    g_loadLibraryOriginal = nullptr;
    g_threadClosing = false;
    fixture::Reset();
    std::puts("PASS: frame subclass/property/hook/timer failure recovery and mutex/module/procedure/init library faults");
}

void CheckUninitPendingOperationAndDropGaps() {
    fixture::Reset();
    Require(!g_unloading && !g_activeOperations && !g_pendingDrops &&
            !g_storageMutex.processMutex, "uninit fixture began with active lifecycle state");
    HANDLE entered = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE release = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE finished = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(entered && release && finished, "uninit operation gates failed");
    std::thread operation([&] {
        OperationScope scope;
        Require(scope.active, "pending uninit operation was not admitted");
        SetEvent(entered);
        Require(WaitForSingleObject(release, 5000) == WAIT_OBJECT_0,
                "pending uninit operation was not released");
    });
    Require(WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0,
            "pending operation did not enter before uninit");
    std::thread unloading([&] { Wh_ModUninit(); SetEvent(finished); });
    const auto deadline = GetTickCount64() + 5000;
    while (!g_unloading && GetTickCount64() < deadline) Sleep(1);
    Require(g_unloading && WaitForSingleObject(finished, 20) == WAIT_TIMEOUT,
            "uninit returned before an admitted operation completed");
    SetEvent(release);
    operation.join();
    Require(WaitForSingleObject(finished, 5000) == WAIT_OBJECT_0,
            "uninit did not wake when its final operation completed");
    unloading.join();
    CloseHandle(entered);
    CloseHandle(release);
    CloseHandle(finished);
    Require(!g_activeOperations, "uninit retained finished operation count");
    g_unloading = false;
    // Model an already-admitted asynchronous item read without involving XAML.
    // The real drop handler's counter is intentionally kept pending until the
    // independent completion arrives, matching its coordinator-facing contract.
    g_pendingDrops = 1;
    std::thread completion([] {
        const auto deadline = GetTickCount64() + 5000;
        while (!g_unloading && GetTickCount64() < deadline) Sleep(1);
        Require(g_unloading, "pending-read completion did not observe unloading admission");
        Sleep(150);
        --g_pendingDrops;
    });
    const auto started = GetTickCount64();
    Wh_ModUninit();
    // Observe the return before joining: the join itself waits for completion
    // and would otherwise hide an early-return regression in Wh_ModUninit.
    const auto returnedAfter = GetTickCount64() - started;
    const auto pendingAtReturn = g_pendingDrops.load();
    completion.join();
    Require(pendingAtReturn == 0 && !g_pendingDrops && returnedAfter >= 100,
            "uninit failed to wait for asynchronous item-read completion");
    g_unloading = false;
    auto stubborn = winrt::make_self<runtime_gap::StubbornRead>();
    g_dropReads = {stubborn.as<winrt::Windows::Foundation::IAsyncInfo>()};
    g_pendingDrops = 1;
    HANDLE stubbornFinished = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(stubbornFinished, "stubborn-read completion gate failed");
    std::thread stubbornUnloading([&] { Wh_ModUninit(); SetEvent(stubbornFinished); });
    Require(WaitForSingleObject(stubbornFinished, 5000) == WAIT_OBJECT_0,
            "uninit stopped cancelling a read that ignored the first cancellations");
    stubbornUnloading.join();
    CloseHandle(stubbornFinished);
    Require(stubborn->cancels == 3 && g_pendingDrops == 0 && g_dropReads.empty(),
            "uninit did not cancel until the read finished or kept the finished read");
    g_unloading = false;
    if (!g_bars) g_bars.emplace();
    g_threadClosing = false;
    fixture::Reset();
    std::puts("PASS: final uninit waits for admitted operations and pending reads, and cancels a read until it finishes");
}
