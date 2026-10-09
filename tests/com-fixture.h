#pragma once
template<typename Interface>
struct TestComObject : Interface {
    std::atomic<ULONG> refs{1};
    virtual ~TestComObject() = default;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == __uuidof(Interface) ||
            (std::is_base_of_v<IDispatch, Interface> && iid == IID_IDispatch)) {
            *out = static_cast<Interface*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = --refs;
        if (!remaining) delete this;
        return remaining;
    }
};
#include "com-stubs.generated.h"

struct FakeItem : StubIShellItem {
    std::wstring path;
    SFGAOF attributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
    HRESULT attributeResult = S_OK;
    HRESULT nameResult = S_OK;
    explicit FakeItem(std::wstring value) : path(std::move(value)) {}
    HRESULT STDMETHODCALLTYPE GetAttributes(SFGAOF mask, SFGAOF* out) override {
        *out = attributes & mask;
        return attributeResult;
    }
    HRESULT STDMETHODCALLTYPE GetDisplayName(SIGDN, LPWSTR* out) override {
        *out = nullptr;
        if (FAILED(nameResult)) return nameResult;
        *out = static_cast<LPWSTR>(CoTaskMemAlloc((path.size() + 1) * sizeof(wchar_t)));
        if (!*out) return E_OUTOFMEMORY;
        std::copy(path.begin(), path.end(), *out);
        (*out)[path.size()] = 0;
        return S_OK;
    }
};
struct FakeItems : StubIShellItemArray {
    std::vector<winrt::com_ptr<IShellItem>> items;
    DWORD calls = 0;
    HRESULT countResult = S_OK;
    HRESULT itemResult = S_OK;
    HRESULT STDMETHODCALLTYPE GetCount(DWORD* count) override {
        *count = static_cast<DWORD>(items.size());
        return countResult;
    }
    HRESULT STDMETHODCALLTYPE GetItemAt(DWORD index, IShellItem** item) override {
        ++calls;
        *item = nullptr;
        if (FAILED(itemResult)) return itemResult;
        if (index >= items.size()) return E_BOUNDS;
        items[index].copy_to(item);
        return S_OK;
    }
};
struct FakePersist : StubIPersistFolder2 {
    std::wstring path;
    HRESULT folderResult = S_OK;
    bool empty = false;
    explicit FakePersist(std::wstring value) : path(std::move(value)) {}
    HRESULT STDMETHODCALLTYPE GetCurFolder(PIDLIST_ABSOLUTE* pidl) override {
        *pidl = nullptr;
        if (FAILED(folderResult) || empty) return folderResult;
        return SHParseDisplayName(path.c_str(), nullptr, pidl, 0, nullptr);
    }
};
struct FakeFolderView : StubIFolderView {
    winrt::com_ptr<IPersistFolder2> persist;
    winrt::com_ptr<IShellItemArray> items;
    HRESULT folderResult = S_OK;
    HRESULT itemsResult = S_OK;
    HRESULT STDMETHODCALLTYPE GetFolder(REFIID iid, void** out) override {
        *out = nullptr;
        if (FAILED(folderResult)) return folderResult;
        return persist ? persist->QueryInterface(iid, out) : E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Items(UINT flags, REFIID iid, void** out) override {
        Require(flags == SVGIO_SELECTION, "selection requested wrong Shell view mode");
        *out = nullptr;
        if (FAILED(itemsResult)) return itemsResult;
        return items ? items->QueryInterface(iid, out) : E_NOINTERFACE;
    }
};
struct FakeView : StubIShellView {
    winrt::com_ptr<IFolderView> folder;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (iid == IID_IFolderView && folder) return folder->QueryInterface(iid, out);
        return StubIShellView::QueryInterface(iid, out);
    }
};
struct FakeBrowser : StubIShellBrowser {
    HWND window = nullptr;
    winrt::com_ptr<IShellView> view;
    HRESULT windowResult = S_OK;
    HRESULT viewResult = S_OK;
    HRESULT browseResult = S_OK;
    std::vector<std::wstring> navigated;
    HRESULT STDMETHODCALLTYPE GetWindow(HWND* out) override { *out = window; return windowResult; }
    HRESULT STDMETHODCALLTYPE QueryActiveShellView(IShellView** out) override {
        *out = nullptr;
        if (SUCCEEDED(viewResult) && view) view.copy_to(out);
        return viewResult;
    }
    HRESULT STDMETHODCALLTYPE BrowseObject(PCUIDLIST_RELATIVE pidl, UINT flags) override {
        Require(flags == (SBSP_ABSOLUTE | SBSP_SAMEBROWSER), "navigation used wrong flags");
        wchar_t path[MAX_PATH]{};
        Require(SHGetPathFromIDListW(pidl, path), "navigation PIDL not filesystem");
        navigated.emplace_back(path);
        return browseResult;
    }
};
struct FakeService : TestComObject<IServiceProvider> {
    winrt::com_ptr<IShellBrowser> browser;
    HRESULT result = S_OK;
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID service, REFIID iid, void** out) override {
        Require(service == kTopLevelBrowser, "wrong browser service requested");
        *out = nullptr;
        if (FAILED(result)) return result;
        return browser ? browser->QueryInterface(iid, out) : E_NOINTERFACE;
    }
};
struct FakePoint : TestComObject<IConnectionPoint> {
    HRESULT adviseResult = S_OK;
    unsigned advised = 0;
    unsigned removed = 0;
    winrt::com_ptr<IUnknown> sink;
    HRESULT STDMETHODCALLTYPE GetConnectionInterface(IID*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetConnectionPointContainer(IConnectionPointContainer**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Advise(IUnknown* value, DWORD* cookie) override {
        if (FAILED(adviseResult)) return adviseResult;
        sink.copy_from(value);
        *cookie = ++advised;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Unadvise(DWORD) override { ++removed; sink = nullptr; return S_OK; }
    HRESULT STDMETHODCALLTYPE EnumConnections(IEnumConnections**) override { return E_NOTIMPL; }
};
struct FakeContainer : TestComObject<IConnectionPointContainer> {
    winrt::com_ptr<IConnectionPoint> point;
    HRESULT result = S_OK;
    HRESULT STDMETHODCALLTYPE EnumConnectionPoints(IEnumConnectionPoints**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE FindConnectionPoint(REFIID, IConnectionPoint** out) override {
        *out = nullptr;
        if (SUCCEEDED(result) && point) point.copy_to(out);
        return result;
    }
};
struct FakeWeb : StubIWebBrowser2 {
    HWND frame = nullptr;
    winrt::com_ptr<IServiceProvider> service;
    winrt::com_ptr<IConnectionPointContainer> container;
    HRESULT windowResult = S_OK;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (iid == IID_IServiceProvider && service) return service->QueryInterface(iid, out);
        if (iid == IID_IConnectionPointContainer && container) return container->QueryInterface(iid, out);
        return StubIWebBrowser2::QueryInterface(iid, out);
    }
    HRESULT STDMETHODCALLTYPE get_HWND(SHANDLE_PTR* out) override {
        *out = reinterpret_cast<SHANDLE_PTR>(frame);
        return windowResult;
    }
};
struct FakeWindows : StubIShellWindows {
    winrt::com_ptr<IConnectionPointContainer> container;
    std::vector<winrt::com_ptr<IDispatch>> windows;
    HRESULT countResult = S_OK;
    HRESULT itemResult = S_OK;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (iid == IID_IConnectionPointContainer && container) return container->QueryInterface(iid, out);
        return StubIShellWindows::QueryInterface(iid, out);
    }
    HRESULT STDMETHODCALLTYPE get_Count(LONG* out) override {
        *out = static_cast<LONG>(windows.size());
        return countResult;
    }
    HRESULT STDMETHODCALLTYPE Item(VARIANT index, IDispatch** out) override {
        *out = nullptr;
        if (FAILED(itemResult)) return itemResult;
        if (index.vt != VT_I4 || index.lVal < 0 || static_cast<size_t>(index.lVal) >= windows.size()) return E_BOUNDS;
        windows[index.lVal].copy_to(out);
        return S_OK;
    }
};
inline winrt::com_ptr<FakeWindows> fakeWindows;
inline HRESULT ShellCreation(REFCLSID clsid, LPUNKNOWN outer, DWORD context, REFIID iid, LPVOID* out) {
    if (clsid == kShellWindows) return fakeWindows ? fakeWindows->QueryInterface(iid, out) : E_FAIL;
    return realComCreation(clsid, outer, context, iid, out);
}

template<typename T, typename... Args>
winrt::com_ptr<T> NewFake(Args&&... args) {
    winrt::com_ptr<T> object;
    object.attach(new T(std::forward<Args>(args)...));
    return object;
}

HWND CreateShellWindow(const wchar_t* name, HWND parent = nullptr) {
    WNDCLASSW type{};
    type.lpfnWndProc = DefWindowProcW;
    type.hInstance = GetModuleHandleW(nullptr);
    type.lpszClassName = name;
    if (!RegisterClassW(&type)) Require(GetLastError() == ERROR_CLASS_ALREADY_EXISTS,
                                      "test Shell class registration failed");
    const auto window = CreateWindowExW(0, name, L"Test Shell fixture",
        parent ? WS_CHILD : WS_OVERLAPPED, 0, 0, 40, 40, parent, nullptr, type.hInstance, nullptr);
    Require(window != nullptr, "test Shell window creation failed");
    return window;
}

struct ShellFixture {
    HWND frame = CreateShellWindow(L"CabinetWClass");
    HWND tab = CreateShellWindow(L"ShellTabWindowClass", frame);
    winrt::com_ptr<FakeBrowser> browser = NewFake<FakeBrowser>();
    winrt::com_ptr<FakeView> view = NewFake<FakeView>();
    winrt::com_ptr<FakeFolderView> folder = NewFake<FakeFolderView>();
    winrt::com_ptr<FakeItems> items = NewFake<FakeItems>();
    winrt::com_ptr<FakeService> service = NewFake<FakeService>();
    winrt::com_ptr<FakeWeb> web = NewFake<FakeWeb>();
    winrt::com_ptr<FakePoint> windowPoint = NewFake<FakePoint>();
    winrt::com_ptr<FakePoint> tabPoint = NewFake<FakePoint>();
    winrt::com_ptr<FakeContainer> windowContainer = NewFake<FakeContainer>();
    winrt::com_ptr<FakeContainer> tabContainer = NewFake<FakeContainer>();
    ShellFixture() {
        fixture::Reset();
        browser->window = tab;
        folder->items = items.as<IShellItemArray>();
        view->folder = folder.as<IFolderView>();
        browser->view = view.as<IShellView>();
        service->browser = browser.as<IShellBrowser>();
        web->service = service.as<IServiceProvider>();
        web->frame = frame;
        tabContainer->point = tabPoint.as<IConnectionPoint>();
        web->container = tabContainer.as<IConnectionPointContainer>();
        windowContainer->point = windowPoint.as<IConnectionPoint>();
        fakeWindows = NewFake<FakeWindows>();
        fakeWindows->container = windowContainer.as<IConnectionPointContainer>();
        fakeWindows->windows.push_back(web.as<IDispatch>());
        fixture::creation = ShellCreation;
        g_threadClosing = false;
    }
    ~ShellFixture() {
        StopRecentTracking();
        StopThreadWork();
        fakeWindows = nullptr;
        fixture::Reset();
        DestroyWindow(frame);
    }
};

void CheckShellSelectionAndNavigation() {
    BackupFixture scratch;
    ShellFixture shell;
    auto persist = NewFake<FakePersist>(scratch.directory);
    shell.folder->persist = persist.as<IPersistFolder2>();
    Require(ActiveShellBrowser(shell.frame).get() == shell.browser.get() &&
            CurrentFolder(shell.frame) == scratch.directory,
            "active tab/current folder selection failed");
    Require(IsExplorerFrame(shell.frame) && !IsExplorerFrame(nullptr) && !IsExplorerFrame(shell.tab),
            "native Explorer frame classification failed");
    for (auto* failure : {&fakeWindows->countResult, &fakeWindows->itemResult,
                          &shell.web->windowResult, &shell.service->result, &shell.browser->windowResult}) {
        *failure = E_FAIL;
        Require(!ActiveShellBrowser(shell.frame), "failed Shell discovery step returned active browser");
        *failure = S_OK;
    }
    shell.browser->window = shell.frame;
    Require(!ActiveShellBrowser(shell.frame), "inactive tab selected");
    shell.browser->window = nullptr;
    Require(!ActiveShellBrowser(shell.frame), "null tab selected");
    shell.browser->window = shell.tab;
    shell.browser->viewResult = E_FAIL;
    Require(CurrentFolder(shell.frame).empty() && SelectedFolders(shell.frame).empty(),
            "failed active view yielded folders");
    shell.browser->viewResult = S_OK;
    shell.folder->folderResult = E_FAIL;
    Require(CurrentFolder(shell.frame).empty(), "failed persist folder query yielded path");
    shell.folder->folderResult = S_OK;
    persist->folderResult = E_FAIL;
    Require(CurrentFolder(shell.frame).empty(), "failed PIDL query yielded path");
    persist->folderResult = S_OK;
    persist->empty = true;
    Require(CurrentFolder(shell.frame).empty(), "empty PIDL yielded path");
    persist->empty = false;
    auto add = [&](std::wstring path, SFGAOF attributes) {
        auto item = NewFake<FakeItem>(std::move(path));
        item->attributes = attributes;
        shell.items->items.push_back(item.as<IShellItem>());
        return item;
    };
    add(L"C:\\file.txt", SFGAO_FILESYSTEM);
    add(L"C:\\archive.zip", SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_STREAM);
    add(L"C:\\virtual", SFGAO_FOLDER);
    auto badName = add(L"C:\\bad", SFGAO_FILESYSTEM | SFGAO_FOLDER);
    badName->nameResult = E_FAIL;
    add(L"C:\\valid\\", SFGAO_FILESYSTEM | SFGAO_FOLDER);
    Require(SelectedFolders(shell.frame) == std::vector<std::wstring>{L"C:\\valid"},
            "selection included files, archives or virtual paths, or lost normalization");
    Require(ToggleBookmarksForHotkey(shell.frame, true).added == 1 && ReadStorageLocked() == L"C:\\valid",
            "Ctrl+B did not use eligible selected folder");
    shell.items->items.clear();
    Require(ToggleBookmarksForHotkey(shell.frame, true).added == 1,
            "empty selection did not fall back to current folder");
    Require(ToggleBookmarksForHotkey(shell.frame, false).changed,
            "non-file-list Ctrl+B did not toggle current folder");
    for (unsigned i = 0; i < 25; ++i) add(L"C:\\selected" + std::to_wstring(i), SFGAO_FOLDER | SFGAO_FILESYSTEM);
    shell.items->calls = 0;
    Require(SelectedFolders(shell.frame).size() == kMaxFoldersPerAction &&
            shell.items->calls == kMaxFoldersPerAction, "selection exceeded action limit");
    shell.items->items.clear();
    for (unsigned i = 0; i < kMaxSelectionScanned; ++i) add(L"C:\\file", SFGAO_FILESYSTEM);
    add(L"C:\\after-limit", SFGAO_FOLDER | SFGAO_FILESYSTEM);
    shell.items->calls = 0;
    Require(SelectedFolders(shell.frame).empty() && shell.items->calls == kMaxSelectionScanned,
            "selection scanned beyond limit");
    shell.folder->itemsResult = E_FAIL;
    Require(SelectedFolders(shell.frame).empty(), "failed selected-items query returned folders");
    shell.folder->itemsResult = S_OK;
    shell.items->countResult = E_FAIL;
    Require(SelectedFolders(shell.frame).empty(), "failed item count returned folders");
    shell.items->countResult = S_OK;
    NavigateToFolder(shell.frame, scratch.directory);
    Require(shell.browser->navigated == std::vector<std::wstring>{scratch.directory},
            "navigation did not browse selected PIDL");
    shell.browser->browseResult = E_FAIL;
    NavigateToFolder(shell.frame, scratch.directory);
    Require(shell.browser->navigated.size() == 2, "failed browse not attempted");
    OpenFolderInNewTab(shell.frame, L"C:\\folder");
    OpenFolderInNewWindow(shell.frame, L"C:\\folder");
    fixture::launchResult = 5;
    OpenFolderInNewTab(shell.frame, L"C:\\folder");
    OpenFolderInNewWindow(shell.frame, L"C:\\folder");
    Require(fixture::launches.size() == 4 && fixture::launches[0].first == L"opennewtab" &&
            fixture::launches[1].first == L"opennewwindow", "navigation used wrong Shell verbs");
    Require((fixture::launchMask & SEE_MASK_FLAG_NO_UI) && (fixture::launchMask & SEE_MASK_NOASYNC),
            "new tab or window launch can show Windows' modal error box");
    Require(!RecordNavigation(nullptr, 0) && RecordNavigation(shell.web.get(), kDispidDocumentComplete) &&
            !RecordNavigation(shell.web.get(), kDispidDocumentComplete),
            "navigation event recording lost path or duplicate suppression");
    std::puts("PASS: isolated active tab, selection filtering/limits, Ctrl+B fallback and navigation verbs");
}

void CheckShellConnections() {
    ShellFixture shell;
    auto sink = NewFake<ShellEventSink>();
    winrt::com_ptr<IConnectionPoint> point;
    DWORD cookie = 0;
    shell.tabContainer->result = E_FAIL;
    Require(!ConnectEvents(shell.web.get(), kWebBrowserEvents2, sink.get(), point, cookie) && !point,
            "failed FindConnectionPoint leaked connection");
    shell.tabContainer->result = S_OK;
    shell.tabPoint->adviseResult = E_FAIL;
    Require(!ConnectEvents(shell.web.get(), kWebBrowserEvents2, sink.get(), point, cookie) && !point,
            "failed Advise retained connection");
    shell.tabPoint->adviseResult = S_OK;
    Require(EnsureRecentTracker() && EnsureRecentTracker() && shell.windowPoint->advised == 1,
            "ShellWindows tracker duplicated subscriptions");
    SyncBrowserConnections(*g_recentTracker);
    Require(g_recentTracker->browsers.size() == 1 && shell.tabPoint->advised == 1,
            "tab event subscription missing");
    SyncBrowserConnections(*g_recentTracker);
    Require(shell.tabPoint->advised == 1, "tab resync duplicated subscription");
    fakeWindows->countResult = E_FAIL;
    SyncBrowserConnections(*g_recentTracker);
    Require(g_recentTracker->browsers.size() == 1, "count error discarded known tab");
    fakeWindows->countResult = S_OK;
    fakeWindows->windows.clear();
    SyncBrowserConnections(*g_recentTracker);
    Require(g_recentTracker->browsers.empty() && shell.tabPoint->removed == 1,
            "revoked tab retained subscription");
    g_recentSettings.enabled = true;
    SyncRecentTracking();
    Require(!g_recentTracker && shell.windowPoint->removed == 1,
            "empty thread retained ShellWindows subscription");
    shell.windowContainer->result = E_FAIL;
    Require(!EnsureRecentTracker() && !g_recentTracker, "failed tracker startup published tracker");
    shell.windowContainer->result = S_OK;
    fixture::creation = [](REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*) -> HRESULT { return E_FAIL; };
    Require(!EnsureRecentTracker() && !ActiveShellBrowser(shell.frame), "failed COM startup ignored");
    fixture::creation = ShellCreation;
    g_recentSettings.enabled = false;
    SyncRecentTracking();
    std::puts("PASS: Shell connection startup failures, subscription deduplication and tab revocation");
}

struct FakeDialog : StubIFileDialog {
    HRESULT optionsResult = S_OK;
    HRESULT setOptionsResult = S_OK;
    HRESULT showResult = S_OK;
    HRESULT selectionResult = S_OK;
    FILEOPENDIALOGOPTIONS options = 0;
    std::wstring fileName;
    std::wstring extension;
    unsigned filters = 0;
    unsigned shows = 0;
    unsigned closed = 0;
    bool defaultFolder = false;
    winrt::com_ptr<IShellItem> item;
    HRESULT STDMETHODCALLTYPE GetOptions(FILEOPENDIALOGOPTIONS* out) override { *out = options; return optionsResult; }
    HRESULT STDMETHODCALLTYPE SetOptions(FILEOPENDIALOGOPTIONS value) override { options = value; return setOptionsResult; }
    HRESULT STDMETHODCALLTYPE SetFileTypes(UINT count, const COMDLG_FILTERSPEC* spec) override {
        Require(count == 2 && std::wstring(spec[0].pszSpec) == L"*.json", "backup dialog filter incorrect");
        filters = count;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SetDefaultExtension(LPCWSTR value) override { extension = value; return S_OK; }
    HRESULT STDMETHODCALLTYPE SetFileName(LPCWSTR value) override { fileName = value; return S_OK; }
    HRESULT STDMETHODCALLTYPE SetDefaultFolder(IShellItem*) override { defaultFolder = true; return S_OK; }
    HRESULT STDMETHODCALLTYPE Show(HWND) override {
        Require(g_threadFileDialog == this, "active dialog pointer not set while showing");
        ++shows;
        CloseActiveDialogCurrentThread();
        return showResult;
    }
    HRESULT STDMETHODCALLTYPE Close(HRESULT reason) override {
        Require(reason == HRESULT_FROM_WIN32(ERROR_CANCELLED), "dialog closed with wrong reason");
        ++closed;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetResult(IShellItem** out) override {
        *out = nullptr;
        if (SUCCEEDED(selectionResult) && item) item.copy_to(out);
        return selectionResult;
    }
};
struct NamelessDialogItem : FakeItem {
    NamelessDialogItem() : FakeItem(L"") {}
    HRESULT STDMETHODCALLTYPE GetDisplayName(SIGDN, LPWSTR* out) override {
        *out = nullptr;
        return S_OK;
    }
};
inline winrt::com_ptr<FakeDialog> fakeDialog;
inline CLSID lastDialogClass;
inline HRESULT DialogCreation(REFCLSID clsid, LPUNKNOWN outer, DWORD context, REFIID iid, LPVOID* out) {
    if (clsid == CLSID_FileOpenDialog || clsid == CLSID_FileSaveDialog) {
        lastDialogClass = clsid;
        return fakeDialog ? fakeDialog->QueryInterface(iid, out) : E_FAIL;
    }
    return ShellCreation(clsid, outer, context, iid, out);
}

void CheckBackupDialogContracts() {
    fixture::Reset();
    BackupFixture scratch;
    fakeDialog = NewFake<FakeDialog>();
    auto item = NewFake<FakeItem>(scratch.file);
    fakeDialog->item = item.as<IShellItem>();
    fixture::creation = DialogCreation;
    Require(ChooseBackupPath(nullptr, true, scratch.directory + L"\\suggested.json") == scratch.file &&
            lastDialogClass == CLSID_FileSaveDialog && fakeDialog->fileName == L"suggested.json" &&
            fakeDialog->defaultFolder && fakeDialog->filters == 2 && fakeDialog->extension == L"json" &&
            (fakeDialog->options & (FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT)) ==
                (FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT) &&
            fakeDialog->shows == 1 && fakeDialog->closed == 1 && !g_threadFileDialog,
            "save dialog options, suggestion, active pointer or result failed");
    Require(ChooseBackupPath(nullptr, false, L"backup.json") == scratch.file &&
            lastDialogClass == CLSID_FileOpenDialog && (fakeDialog->options & FOS_FILEMUSTEXIST),
            "open dialog class or existence option failed");
    {
        platform_failure::Scope failures;
        platform_failure::faults.coInitialize = true;
        const auto showsBeforeFailure = fakeDialog->shows;
        Require(ChooseBackupPath(nullptr, true, scratch.file).empty() &&
                fakeDialog->shows == showsBeforeFailure && !g_threadFileDialog,
                "failed dialog COM apartment showed a picker or retained active dialog");
    }
    // A suggestion can outlive its parent folder. Keep the valid selection
    // usable even when Shell cannot resolve the suggested default directory.
    fakeDialog->defaultFolder = false;
    Require(ChooseBackupPath(nullptr, true, scratch.directory +
                L"\\missing-parent\\suggested.json") == scratch.file &&
            fakeDialog->fileName == L"suggested.json" && !fakeDialog->defaultFolder &&
            !g_threadFileDialog,
            "missing suggested folder prevented a valid backup selection");
    auto nameless = NewFake<NamelessDialogItem>();
    fakeDialog->item = nameless.as<IShellItem>();
    Require(ChooseBackupPath(nullptr, false, scratch.file).empty() && !g_threadFileDialog,
            "dialog item without filesystem name returned a path or retained active dialog");
    fakeDialog->item = item.as<IShellItem>();
    Require(ChooseBackupPath(nullptr, false, scratch.file) == scratch.file,
            "valid dialog selection failed after null filesystem name");
    for (auto* error : {&fakeDialog->optionsResult, &fakeDialog->setOptionsResult,
                        &fakeDialog->showResult, &fakeDialog->selectionResult, &item->nameResult}) {
        *error = E_FAIL;
        Require(ChooseBackupPath(nullptr, false, scratch.file).empty() && !g_threadFileDialog,
                "failed dialog operation returned path or leaked active pointer");
        *error = S_OK;
    }
    g_unloading = true;
    const auto shows = fakeDialog->shows;
    Require(ChooseBackupPath(nullptr, true, scratch.file).empty() && fakeDialog->shows == shows,
            "unloading mod showed backup dialog");
    g_unloading = false;
    fakeDialog = nullptr;
    Require(ChooseBackupPath(nullptr, true, L"backup.json").empty(), "failed dialog COM creation ignored");
    CloseActiveDialogCurrentThread();
    fixture::Reset();
    std::puts("PASS: backup dialog classes/options/results, cancellation, failures and unload guard");
}

void CheckDriveAndDeferredNativePaths() {
    fixture::Reset();
    const std::wstring root = L"Z:\\";
    for (const auto type : {DRIVE_REMOVABLE, DRIVE_REMOTE, DRIVE_CDROM, DRIVE_FIXED, DRIVE_RAMDISK, DRIVE_UNKNOWN}) {
        fixture::drives[root] = type;
        Require(!DriveMenuLabel(root).empty(), "drive type omitted menu label");
        if (type != DRIVE_FIXED && type != DRIVE_RAMDISK) {
            Require(CheckFolderStatus(L"Z:\\folder") == FolderStatus::Unknown,
                    "nonlocal drive path probed as missing");
        }
    }
    fixture::drives[root] = DRIVE_NO_ROOT_DIR;
    Require(CheckFolderStatus(L"Z:\\folder") == FolderStatus::Missing, "absent drive not missing");
    fixture::drives[root] = DRIVE_FIXED;
    for (auto error : {ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_INVALID_DRIVE, ERROR_ACCESS_DENIED}) {
        fixture::attributes[L"Z:\\folder"] = {INVALID_FILE_ATTRIBUTES, error};
        Require(CheckFolderStatus(L"Z:\\folder") == (error == ERROR_ACCESS_DENIED ? FolderStatus::Unknown : FolderStatus::Missing),
                "folder errors misclassified");
    }
    BarState drag;
    drag.dragPath = L"Z:\\folder";
    drag.dragIsMouse = true;
    const auto button = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
    fixture::keys[button] = static_cast<SHORT>(0x8000);
    Require(DragInProgress(drag), "held mouse drag cancelled");
    fixture::keys[button] = 0;
    Require(!DragInProgress(drag) && drag.dragPath.empty(), "released mouse drag remained active");
    if (!g_bars) g_bars.emplace();
    g_threadClosing = false;
    g_settingsApplyPending = false;
    g_recentSettings.enabled = false;
    RunThreadWork();
    Require(!g_threadWorking && !g_activeOperations, "empty deferred run leaked state");
    g_threadWorking = true;
    RunThreadWork();
    Require(g_threadWorking, "reentrant deferred run cleared outer working flag");
    g_threadWorking = false;
    g_unloading = true;
    RunThreadWork();
    Require(!g_threadWorking, "deferred run executed during unload");
    g_unloading = false;
    ScheduleThreadWork();
    ThreadWorkTimerProc(nullptr, 0, g_workTimer, 0);
    Require(!g_workTimer && !g_threadWorking, "work timer failed to clear");
    BookmarkChange change;
    change.added = 1;
    change.noRoom = 2;
    const auto notice = MakeNotice(nullptr, change);
    Require(notice.kind == BookmarkNotice::Partial && notice.total == 3, "notice maker lost counts");
    ShowBookmarkNotice(notice); // No loaded bar: must safely drop it.
    ShowBookmarkNotice({});
    HideNotices();
    NoticeTimerProc(nullptr, 0, SetTimer(nullptr, 0, 1000, nullptr), 0);
    Wh_ModSettingsChanged();
    Wh_ModAfterInit(); // WH_EDITING doesn't install hooks.
    Require(!Wh_ModInit() && !g_storageMutex.processMutex, "editor-stub hook failure leaked storage mutex");
    Wh_ModBeforeUninit();
    Require(g_unloading, "before-uninit did not close operation admission");
    Wh_ModUninit();
    g_unloading = false;
    fixture::Reset();
    std::puts("PASS: drive types/errors, mouse drag release, deferred work and mod lifecycle failure cleanup");
}
