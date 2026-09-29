// ==WindhawkMod==
// @id              explorer-folder-bookmarks-bar
// @name            Explorer Folder Bookmarks Bar (personal build)
// @description     Adds an adaptive folder bookmarks bar to newly opened Windows 11 File Explorer windows, and optionally recent folders to Open and Save dialogs of all programs.
// @version         0.8.5
// @author          Maxim Fomin
// @github          https://github.com/MaxITService
// @include         *
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -lshell32 -luuid -lruntimeobject -lwindowscodecs
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Explorer Folder Bookmarks Bar

For the Windows 11 WinUI File Explorer. A scrollable bookmark bar appears below
the address bar. Click **+** or press **Ctrl+B** to bookmark the current filesystem
folder; Ctrl+B removes the bookmark when the folder already has one. With
folders selected in the file list, Ctrl+B bookmarks all of them, or removes
them when all are bookmarked already. You can also drag one or more folders
from File Explorer onto the bar; they are inserted where you drop them. For
safety, one Ctrl+B or drop handles at most 20 folders; the rest are ignored.
Click a
bookmark to navigate the active tab to it. Ctrl+click asks Explorer to open it
in a new tab; right-click opens it in a new window. Drag a bookmark onto another to reorder the row. Middle-click a
bookmark to remove it from the bar. Removing a bookmark never deletes its
target folder.

![Explorer Folder Bookmarks Bar in File Explorer](https://raw.githubusercontent.com/MaxITService/EXPLORER-bookmarks-bar-windhawk/main/Promo/How-it-works.gif)

After enabling or updating the mod, open a new File Explorer window to use the
bar. Windows that were already open may remain unchanged. Opening a new window
is the supported way to activate the bar without relying on live window updates.

The bar expands from one to four rows as the window narrows. If bookmarks
still exceed the fourth row, the bar can pan sideways.
Left-click **FX**, next to **+**, for the profile (**~**), Desktop, Documents,
Downloads, and the custom folders listed in this mod's Windhawk settings.
Custom shortcuts are empty by default. Add a folder path and optional label in
**Settings → FX custom folders**; blank entries are ignored. Paths must be
absolute, and `%NAME%` environment variables are expanded. Local, network
(UNC or mapped drive) and removable-drive folders all work. A folder missing
from a local disk is hidden from FX until it exists again. Like bookmarks,
network and removable entries are shown without being checked; if one is
unavailable, Explorer reports it when you click it. New settings take effect in
newly opened Explorer windows. Right-click **FX** for all drives with their
labels. The list updates each time the menu opens. Ctrl+click a menu entry to
open it in a new tab.

Right-click **+** for **Save bookmarks** and **Load bookmarks**. The commands
open a file dialog so you can choose the JSON backup. The profile folder is
suggested initially. Loading replaces the current list only after the entire
UTF-8 JSON file passes validation.

Turn on **Recent folders** in settings to show up to three recently opened
folders at the right end of the bar, after a **|** separator. The newest folder
is nearest the right edge, where **RC** sits. Left-click **RC** for the
remembered folders that have no button, up to ten in total, and to choose
the source: folders you open in File Explorer, or Windows recent items. If
Windows is set not to keep recent items, RC says so. Right-click **RC** to
clear the list; clearing Windows recent items only hides them from the bar and
never deletes Windows data. Middle-click a recent folder to remove it, or
drag it onto the bookmarks to keep it. Bookmarked folders and folders missing
from a local disk are never listed as recent. When the window is too narrow,
the recent buttons fold into RC; without RC, the oldest buttons are hidden first.
Folders you open in File Explorer are remembered in this mod's Windhawk local
storage (up to 64 paths) and stay there after Recent folders is turned off. To
erase them, choose *Folders opened in File Explorer* in the RC menu and clear
the list with a right-click on RC.

This personal build loads into every process. With **Recent folders in file
dialogs** on, Open and Save dialogs that programs create through COM list the
same recent folders at the top of their navigation pane. Programs that use a
different picker, such as Store apps, are not changed. It is not intended for
the Windhawk catalog.

The bookmark list lives in this mod's Windhawk local storage. This version
supports up to 32 folders and uses the folder name as the button label. Icons
come from Windows Shell, including desktop.ini custom folder icons. Hovering a
bookmark shows its full path. Virtual locations such as Home are ignored by
**+**. Bookmark removal never touches the target folder or its contents.
Missing folders on local fixed drives show a yellow warning icon. Icons are
cached for up to five minutes and refreshed when the bar redraws.

The XAML insertion point can change in a Windows update. If the bar is not
visible, disable the mod and check the Windhawk log before trying it again.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- fxCustomFolders:
    - - label: ''
        $name: Menu label
        $description: Name shown for this folder in the FX menu. If blank, the folder's own name is used.
      - path: ''
        $name: Folder path
        $description: Full path of the folder, for example C:\Projects, \\server\share\Docs or %USERPROFILE%\Pictures. Leave blank to skip this entry. A folder missing from a local disk is hidden until it exists again.
  $name: FX custom folders
  $description: Adds your own folders to the menu of the FX button (the second button on the bookmarks bar, right after +). Left-click FX to open the menu. It always lists your profile (~), Desktop, Documents and Downloads, followed by the folders from this list in the same order (up to 24). Click an entry to open it in the current tab; Ctrl+click opens it in a new tab. Right-click FX for a list of all drives. Changes apply to Explorer windows opened after you save. Local, network and removable-drive folders all work. A folder missing from a local disk is hidden until it exists again; network and removable entries are always shown, and Explorer reports it if one is unavailable when you click it.
- bookmarkHotkey: true
  $name: Ctrl+B bookmarks the current or selected folders
  $description: Press Ctrl+B in File Explorer to bookmark the current folder, or to remove its bookmark when it already has one. With folders selected in the file list, Ctrl+B bookmarks all of them, or removes them when all are bookmarked already; at most 20 folders are handled at a time. Ctrl+B is ignored while you type in the address bar, the search box or a rename field.
- recentFolders: false
  $name: Recent folders
  $description: Shows recently opened folders at the right end of the bar, after a | separator, with the newest nearest the right edge. Click one to open it; Ctrl+click opens a new tab and right-click a new window. Middle-click removes it from the recent list. Drag it onto the bookmarks to keep it. Bookmarked folders are never listed. Left-click RC to choose between folders opened in File Explorer and Windows recent items. Folders opened in File Explorer are remembered in the mod's local storage (up to 64) until you clear the list.
- recentButtons: 3
  $name: Recent folder buttons
  $description: How many recent folders appear as buttons, from 1 to 3. When the window is too narrow, the buttons fold into RC; without RC, the oldest buttons are hidden first.
- recentMenuButton: true
  $name: RC button
  $description: Shows RC at the right edge. Left-click it for remembered folders that have no button and to choose the source; right-click it to clear the list. RC also appears while no recent folder is available.
- recentHistory: 10
  $name: Remembered recent folders
  $description: How many recent folders the buttons and the RC menu list together, from 1 to 10. It is never less than the number of buttons.
- recentRightClickClears: false
  $name: Right-click RC clears instantly
  $description: When on, right-clicking RC clears the recent list at once instead of showing a Clear command.
- recentInFileDialogs: false
  $name: Recent folders in file dialogs
  $description: Adds the recent folders to the top of the navigation pane in Open and Save dialogs of all programs. Requires Recent folders. Applies to dialogs opened after you save.
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <exdisp.h>
#include <oleauto.h>
#include <servprov.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <robuffer.h>
#include <shellapi.h>
#include <wincodec.h>
#include <windhawk_utils.h>

// winbase.h defines a legacy macro that collides with a WinRT method.
#undef GetCurrentTime

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.h>
#include <winrt/Microsoft.UI.Content.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Input.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <cwctype>
#include <cwchar>
#include <functional>
#include <iterator>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;
namespace muxi = winrt::Microsoft::UI::Xaml::Input;
namespace muxm = winrt::Microsoft::UI::Xaml::Media;
namespace muxmi = winrt::Microsoft::UI::Xaml::Media::Imaging;
namespace wjson = winrt::Windows::Data::Json;

constexpr wchar_t kBarName[] = L"WindhawkExplorerFolderBookmarksBar";
constexpr size_t kMaxStorageChars = 30000;
constexpr size_t kMaxBookmarks = 32;
// A single Ctrl+B or drop handles at most this many folders.
constexpr size_t kMaxFoldersPerAction = 20;
constexpr DWORD kMaxImportBytes = 262144;
constexpr wchar_t kExportFileName[] = L"explorer-folder-bookmarks.json";
constexpr size_t kMaxCustomFxFolders = 24;
constexpr size_t kMaxCachedIcons = 64;
constexpr ULONGLONG kFolderCheckIntervalMs = 10000;
constexpr ULONGLONG kIconCacheLifetimeMs = 300000;
constexpr ULONGLONG kFailedIconCacheLifetimeMs = 30000;
// The stock header at 96 DPI measured 38 + 48 + 48 units. Its centered
// navigation Grid is 54 units high, overhanging the 48-unit navigation row by
// 3 units on each side. An added 38-unit strip therefore needs 38 + 6 = 44
// extra units in the host when the command row stays at its natural 48.
constexpr double kRowHeight = 38.0;
constexpr double kButtonHeight = 32.0;
constexpr double kButtonVerticalInset = (kRowHeight - kButtonHeight) / 2.0;
constexpr double kInterRowGap = 2.0;
constexpr unsigned kMaxRows = 4;
// Lift the visible bar without changing its reserved layout height.
constexpr double kRowOpticalLift = 3.0;
constexpr int kNavigationOverhang = 6;
constexpr int kMeasuredHostHeightAt96Dpi = 136;
constexpr size_t kFixedButtons = 2;
constexpr float kDragThreshold = 6.0f;
constexpr wchar_t kBookmarksKey[] = L"folders";
constexpr wchar_t kRecentsKey[] = L"recentFolders";
constexpr wchar_t kRecentSourceKey[] = L"recentSource";
constexpr wchar_t kWindowsRecentClearedKey[] = L"windowsRecentClearedAt";
constexpr wchar_t kWindowsRecentHiddenKey[] = L"windowsRecentHidden";
// More entries than can be listed are kept, so filtering out bookmarks and
// missing folders still leaves enough recent folders to show.
constexpr size_t kMaxStoredRecents = 64;
constexpr int kMaxRecentButtons = 3;
constexpr int kMaxRecentHistory = 10;
constexpr size_t kMaxRecentLinksScanned = 256;
constexpr ULONGLONG kWindowsRecentCacheMs = 5000;

constexpr double RowAllocation(unsigned rows) {
    return kRowHeight * rows + kInterRowGap * (rows - 1);
}

constexpr int HostExtraAt96Dpi(unsigned rows) {
    return static_cast<int>(RowAllocation(rows)) + kNavigationOverhang;
}

// Public Shell COM identifiers, defined here to avoid SDK import ambiguity.
constexpr CLSID kShellWindows = {
    0x9ba05972, 0xf6a8, 0x11cf,
    {0xa4, 0x42, 0x00, 0xa0, 0xc9, 0x0a, 0x8f, 0x39}};
constexpr GUID kTopLevelBrowser = {
    0x4c96be40, 0x915c, 0x11cf,
    {0x99, 0xd3, 0x00, 0xaa, 0x00, 0x4a, 0xe8, 0x37}};
constexpr IID kWebBrowserEvents2 = {
    0x34a715a0, 0x6587, 0x11d0,
    {0x92, 0x4a, 0x00, 0x20, 0xaf, 0xc7, 0xac, 0x4d}};
constexpr IID kShellWindowsEvents = {
    0xfe4106e0, 0x399a, 0x11d0,
    {0xa4, 0x8c, 0x00, 0xa0, 0xc9, 0x0a, 0x8f, 0x39}};
constexpr DISPID kDispidWindowRegistered = 200;
constexpr DISPID kDispidWindowRevoked = 201;
constexpr DISPID kDispidNavigateComplete2 = 252;
constexpr DISPID kDispidDocumentComplete = 259;

enum class RecentSource { Explorer, Windows };

// Read once in Wh_ModInit; Windhawk reloads the mod when settings change.
struct RecentSettings {
    bool enabled = false;
    size_t buttons = kMaxRecentButtons;
    bool menuButton = true;
    size_t history = kMaxRecentHistory;
    bool rightClickClears = false;
};
RecentSettings g_recentSettings;
bool g_bookmarkHotkey = true;

std::mutex g_storageMutex;
std::atomic<bool> g_unloading = false;
std::atomic<bool> g_extensionHooked = false;
std::atomic<bool> g_frameHooked = false;
std::atomic<bool> g_extensionHookAttempted = false;
std::atomic<bool> g_frameHookAttempted = false;
std::mutex g_operationMutex;
std::condition_variable g_operationFinished;
unsigned g_activeOperations = 0;
thread_local IFileDialog* g_threadFileDialog = nullptr;

struct BarState {
    winrt::weak_ref<muxc::CommandBar> commandBar;
    winrt::event_token loadedToken{};
    winrt::event_token unloadedToken{};
    winrt::weak_ref<muxc::Grid> grid;
    winrt::weak_ref<muxc::Grid> hostGrid;
    winrt::weak_ref<mux::FrameworkElement> navControl;
    // The bar root holds the bookmark strip and, to its right, the recents.
    winrt::weak_ref<muxc::Grid> barRoot;
    winrt::weak_ref<muxc::ScrollViewer> strip;
    winrt::weak_ref<muxc::StackPanel> buttons;
    winrt::weak_ref<muxc::StackPanel> recents;
    winrt::event_token rootPointerToken{};
    winrt::event_token rootSizeToken{};
    winrt::event_token rootLoadedToken{};
    winrt::event_token rootDragOverToken{};
    winrt::event_token rootDragLeaveToken{};
    winrt::event_token rootDropToken{};
    std::vector<std::function<void()>> panelHandlers;
    std::vector<std::function<void()>> driveHandlers;
    std::vector<std::function<void()>> recentMenuHandlers;
    // Newest first. The first visibleRecents entries have buttons; the RC
    // menu lists the rest.
    std::vector<std::wstring> recentFolders;
    RecentSource recentSource = RecentSource::Explorer;
    bool windowsRecentsOff = false;
    size_t recentButtonCount = 0;
    bool hasRecentMenuButton = false;
    size_t visibleRecents = 0;
    // Keep row definitions strong: they are not UIElements, and XAML can
    // discard an unreferenced wrapper so a weak reference no longer resolves.
    muxc::RowDefinition addedRow{nullptr};
    muxc::RowDefinition commandRow{nullptr};
    mux::GridLength oldCommandRowHeight{1.0, mux::GridUnitType::Star};
    bool createdFirstRow = false;
    double oldGridMinHeight = 0;
    double oldNavMinHeight = 0;
    double appliedGridMinHeight = 0;
    double appliedNavMinHeight = 0;
    double originalGridHeight = 0;
    double originalNavHeight = 0;
    unsigned rowCount = 1;
    double lastLayoutWidth = 0;
    bool reflowing = false;
    std::wstring renderedKey;
    ULONGLONG lastFolderCheckAt = 0;
    std::wstring dragPath;
    winrt::weak_ref<muxc::Button> dragButton;
    winrt::Windows::Foundation::Point dragStart{};
    uint32_t dragPointerId = 0;
    bool dragIsMouse = false;
    bool dragging = false;
    bool dragFromRecents = false;
    std::wstring suppressClick;
    double dragOldOpacity = 1.0;
    winrt::weak_ref<muxc::Button> dropTarget;
    mux::Thickness dropOldThickness{};
    bool dropAfter = false;
};

// XAML objects must only be touched by their owning UI thread.
// Stable addresses matter while XAML layout callbacks run and new Explorer
// tabs can register another command bar on the same UI thread.
thread_local std::list<BarState> g_bars;
// Explorer's XAML window and its size hook run on the same UI thread. Use the
// largest active bar on that thread so another tab cannot be clipped.
thread_local unsigned g_frameRows = 0;

void RevokeHandlers(std::vector<std::function<void()>>& handlers) {
    auto pending = std::move(handlers);
    handlers.clear();
    for (auto it = pending.rbegin(); it != pending.rend(); ++it) {
        try {
            (*it)();
        } catch (...) {
            Wh_Log(L"Bookmark handler cleanup failed: %08X",
                   winrt::to_hresult().value);
        }
    }
}

template <typename Element, typename Handler>
void TrackClick(std::vector<std::function<void()>>& handlers,
                const Element& element, Handler&& handler) {
    auto token = element.Click(std::forward<Handler>(handler));
    auto weak = winrt::make_weak(element);
    handlers.emplace_back([weak, token] {
        if (auto current = weak.get()) {
            current.Click(token);
        }
    });
}

template <typename Element, typename Handler>
void TrackContextRequested(std::vector<std::function<void()>>& handlers,
                           const Element& element, Handler&& handler) {
    auto token = element.ContextRequested(std::forward<Handler>(handler));
    auto weak = winrt::make_weak(element);
    handlers.emplace_back([weak, token] {
        if (auto current = weak.get()) {
            current.ContextRequested(token);
        }
    });
}

template <typename Handler>
void TrackOpening(std::vector<std::function<void()>>& handlers,
                  const muxc::MenuFlyout& menu, Handler&& handler) {
    auto token = menu.Opening(std::forward<Handler>(handler));
    auto weak = winrt::make_weak(menu);
    handlers.emplace_back([weak, token] {
        if (auto current = weak.get()) {
            current.Opening(token);
        }
    });
}

void TrackPointer(std::vector<std::function<void()>>& handlers,
                  const muxc::Button& button, const mux::RoutedEvent& event,
                  muxi::PointerEventHandler handler) {
    auto boxed = winrt::box_value(std::move(handler));
    button.AddHandler(event, boxed, true);
    auto weak = winrt::make_weak(button);
    handlers.emplace_back([weak, event, boxed] {
        if (auto current = weak.get()) {
            current.RemoveHandler(event, boxed);
        }
    });
}
struct IconPixels {
    int width = 0;
    int height = 0;
    std::vector<BYTE> bytes;
};

struct IconCacheEntry {
    std::wstring path;
    std::optional<IconPixels> pixels;
    winrt::weak_ref<muxmi::WriteableBitmap> bitmap;
    bool attempted = false;
    ULONGLONG loadedAt = 0;
    ULONGLONG usedAt = 0;
};

// WriteableBitmap is a XAML object, so never share these entries across UI
// threads. Eviction also bounds memory when bookmarks are repeatedly changed.
thread_local std::vector<IconCacheEntry> g_iconCache;

std::wstring NormalizePath(std::wstring path) {
    while (path.size() > 3 && (path.back() == L'\\' || path.back() == L'/')) {
        path.pop_back();
    }
    return path;
}

bool SamePath(const std::wstring& a, const std::wstring& b) {
    return _wcsicmp(a.c_str(), b.c_str()) == 0;
}

enum class FolderStatus { Available, Missing, Unknown };

FolderStatus CheckFolderStatus(const std::wstring& path) {
    // Avoid probing network and removable storage on Explorer's UI thread.
    // An unknown result never produces a misleading missing-folder badge.
    if (path.size() < 3 || path[1] != L':' ||
        (path[2] != L'\\' && path[2] != L'/') ||
        !((path[0] >= L'A' && path[0] <= L'Z') ||
          (path[0] >= L'a' && path[0] <= L'z'))) {
        return FolderStatus::Unknown;
    }
    wchar_t root[] = {path[0], L':', L'\\', L'\0'};
    UINT driveType = GetDriveTypeW(root);
    if (driveType == DRIVE_NO_ROOT_DIR) {
        return FolderStatus::Missing;
    }
    if (driveType != DRIVE_FIXED && driveType != DRIVE_RAMDISK) {
        return FolderStatus::Unknown;
    }
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        return (attributes & FILE_ATTRIBUTE_DIRECTORY)
                   ? FolderStatus::Available
                   : FolderStatus::Missing;
    }
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ||
        error == ERROR_INVALID_DRIVE) {
        return FolderStatus::Missing;
    }
    return FolderStatus::Unknown;
}

std::wstring ReadValueLocked(const wchar_t* key) {
    std::vector<wchar_t> buffer(kMaxStorageChars + 1);
    size_t chars = Wh_GetStringValue(key, buffer.data(), buffer.size());
    if (chars == 0 || chars > kMaxStorageChars) {
        return {};
    }
    return std::wstring(buffer.data(), chars);
}

std::wstring ReadStorageLocked() {
    return ReadValueLocked(kBookmarksKey);
}

std::vector<std::wstring> SplitLines(const std::wstring& storage,
                                     size_t limit) {
    std::vector<std::wstring> lines;
    size_t pos = 0;
    while (pos < storage.size() && lines.size() < limit) {
        size_t end = storage.find(L'\n', pos);
        if (end == std::wstring::npos) {
            end = storage.size();
        }
        if (end > pos) {
            lines.push_back(storage.substr(pos, end - pos));
        }
        pos = end + 1;
    }
    return lines;
}

std::vector<std::wstring> SplitPaths(const std::wstring& storage,
                                     size_t limit) {
    auto paths = SplitLines(storage, limit);
    for (auto& path : paths) {
        path = NormalizePath(std::move(path));
    }
    std::erase_if(paths, [](const auto& path) { return path.empty(); });
    return paths;
}

std::vector<std::wstring> SplitBookmarks(const std::wstring& storage) {
    return SplitPaths(storage, kMaxBookmarks);
}

std::wstring JoinLines(const std::vector<std::wstring>& lines) {
    std::wstring storage;
    for (const auto& line : lines) {
        if (!storage.empty()) {
            storage += L'\n';
        }
        storage += line;
    }
    return storage;
}

bool SaveBookmarksLocked(const std::vector<std::wstring>& folders) {
    std::wstring storage = JoinLines(folders);
    return storage.size() <= kMaxStorageChars &&
           Wh_SetStringValue(kBookmarksKey, storage.c_str());
}

// Drops the oldest entries until the list fits in one storage value.
bool SaveLinesLocked(const wchar_t* key, std::vector<std::wstring> lines) {
    std::wstring storage = JoinLines(lines);
    while (storage.size() > kMaxStorageChars && !lines.empty()) {
        lines.pop_back();
        storage = JoinLines(lines);
    }
    return Wh_SetStringValue(key, storage.c_str());
}

// Suggest the profile location for an export without fixing the user's choice.
std::wstring SuggestedBackupPath() {
    PWSTR profile = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_DEFAULT,
                                    nullptr, &profile)) || !profile) {
        return {};
    }
    std::wstring path(profile);
    CoTaskMemFree(profile);
    if (path.empty()) {
        return {};
    }
    if (path.back() != L'\\') {
        path += L'\\';
    }
    return path + kExportFileName;
}

std::wstring KnownFolderPath(REFKNOWNFOLDERID folderId) {
    PWSTR value = nullptr;
    if (FAILED(SHGetKnownFolderPath(folderId, KF_FLAG_DEFAULT, nullptr,
                                    &value)) || !value) {
        return {};
    }
    std::wstring path(value);
    CoTaskMemFree(value);
    return NormalizePath(std::move(path));
}

std::wstring DriveMenuLabel(const std::wstring& root) {
    if (root.size() < 2 || root[1] != L':') {
        return root;
    }
    std::wstring letter = root.substr(0, 2);
    UINT type = GetDriveTypeW(root.c_str());
    if (type == DRIVE_FIXED || type == DRIVE_RAMDISK) {
        wchar_t volumeName[MAX_PATH + 1]{};
        if (GetVolumeInformationW(root.c_str(), volumeName, MAX_PATH + 1,
                                  nullptr, nullptr, nullptr, nullptr, 0) &&
            volumeName[0]) {
            return letter + L" — " + volumeName;
        }
    }
    const wchar_t* fallback = L"Unavailable";
    switch (type) {
        case DRIVE_FIXED: fallback = L"Local disk"; break;
        case DRIVE_REMOVABLE: fallback = L"Removable drive"; break;
        case DRIVE_REMOTE: fallback = L"Network drive"; break;
        case DRIVE_CDROM: fallback = L"Optical drive"; break;
        case DRIVE_RAMDISK: fallback = L"RAM disk"; break;
    }
    return letter + L" — " + fallback;
}

struct ScopedFile {
    HANDLE value = INVALID_HANDLE_VALUE;
    explicit ScopedFile(HANDLE handle = INVALID_HANDLE_VALUE) : value(handle) {}
    ~ScopedFile() {
        if (value != INVALID_HANDLE_VALUE) {
            CloseHandle(value);
        }
    }
    ScopedFile(const ScopedFile&) = delete;
    ScopedFile& operator=(const ScopedFile&) = delete;
};

bool IsAbsoluteFolderPath(const std::wstring& path) {
    bool drive = path.size() >= 3 &&
                 ((path[0] >= L'A' && path[0] <= L'Z') ||
                  (path[0] >= L'a' && path[0] <= L'z')) &&
                 path[1] == L':' && path[2] == L'\\';
    bool unc = path.size() >= 5 && path[0] == L'\\' &&
               path[1] == L'\\' && path[2] != L'\\' &&
               path.find(L'\\', 2) != std::wstring::npos;
    return (drive || unc) &&
           std::none_of(path.begin(), path.end(), [](wchar_t ch) {
               return ch < 32;
           });
}

bool ValidateImportedFolders(const std::vector<std::wstring>& folders) {
    if (folders.size() > kMaxBookmarks) {
        return false;
    }
    size_t storageChars = 0;
    for (size_t i = 0; i < folders.size(); ++i) {
        const auto& path = folders[i];
        if (!IsAbsoluteFolderPath(path) || path != NormalizePath(path) ||
            path.find(L'\n') != std::wstring::npos ||
            path.find(L'\r') != std::wstring::npos) {
            return false;
        }
        storageChars += path.size() + (i != 0);
        if (storageChars > kMaxStorageChars) {
            return false;
        }
        for (size_t j = 0; j < i; ++j) {
            if (SamePath(path, folders[j])) {
                return false;
            }
        }
    }
    return true;
}

bool WideToUtf8(const std::wstring& wide, std::string& utf8) {
    if (wide.size() > INT_MAX) {
        return false;
    }
    int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                    wide.data(), static_cast<int>(wide.size()),
                                    nullptr, 0, nullptr, nullptr);
    if (!count) {
        return false;
    }
    utf8.resize(count);
    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                               wide.data(), static_cast<int>(wide.size()),
                               utf8.data(), count, nullptr, nullptr) == count;
}

bool Utf8ToWide(const std::string& utf8, std::wstring& wide) {
    if (utf8.empty() || utf8.size() > INT_MAX) {
        return false;
    }
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    utf8.data(), static_cast<int>(utf8.size()),
                                    nullptr, 0);
    if (!count) {
        return false;
    }
    wide.resize(count);
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                               utf8.data(), static_cast<int>(utf8.size()),
                               wide.data(), count) == count;
}

bool SaveBackup(const std::wstring& path) try {
    if (path.empty()) {
        return false;
    }
    std::vector<std::wstring> folders;
    {
        std::lock_guard lock(g_storageMutex);
        folders = SplitBookmarks(ReadStorageLocked());
    }
    wjson::JsonObject root;
    root.SetNamedValue(L"format", wjson::JsonValue::CreateStringValue(
                                       L"explorer-folder-bookmarks-bar"));
    root.SetNamedValue(L"version", wjson::JsonValue::CreateNumberValue(1));
    wjson::JsonArray list;
    for (const auto& folder : folders) {
        list.Append(wjson::JsonValue::CreateStringValue(folder));
    }
    root.SetNamedValue(L"folders", list);
    std::string bytes;
    auto jsonText = root.Stringify();
    if (!WideToUtf8(std::wstring(jsonText.c_str()), bytes) ||
        bytes.size() > kMaxImportBytes) {
        return false;
    }
    std::wstring directory = path.substr(0, path.find_last_of(L'\\'));
    wchar_t temp[MAX_PATH]{};
    if (!GetTempFileNameW(directory.c_str(), L"efb", 0, temp)) {
        Wh_Log(L"Bookmark backup temp file failed: %lu", GetLastError());
        return false;
    }
    bool saved = false;
    {
        ScopedFile file(CreateFileW(temp, GENERIC_WRITE, 0, nullptr,
                                    TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                    nullptr));
        if (file.value != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            saved = WriteFile(file.value, bytes.data(),
                              static_cast<DWORD>(bytes.size()), &written,
                              nullptr) && written == bytes.size() &&
                    FlushFileBuffers(file.value);
        }
    }
    if (saved) {
        saved = MoveFileExW(temp, path.c_str(),
                            MOVEFILE_REPLACE_EXISTING |
                                MOVEFILE_WRITE_THROUGH) != 0;
    }
    if (!saved) {
        Wh_Log(L"Bookmark backup save failed: %lu", GetLastError());
        DeleteFileW(temp);
    }
    return saved;
} catch (...) {
    Wh_Log(L"Bookmark backup serialization failed: %08X",
           winrt::to_hresult().value);
    return false;
}

bool LoadBackup(const std::wstring& path) try {
    if (path.empty()) {
        return false;
    }
    ScopedFile file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                nullptr));
    if (file.value == INVALID_HANDLE_VALUE) {
        Wh_Log(L"Bookmark backup open failed: %lu", GetLastError());
        return false;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file.value, &size) || size.QuadPart <= 0 ||
        size.QuadPart > kMaxImportBytes) {
        return false;
    }
    std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    if (!ReadFile(file.value, bytes.data(), static_cast<DWORD>(bytes.size()),
                  &read, nullptr) || read != bytes.size()) {
        return false;
    }
    std::wstring wide;
    if (!Utf8ToWide(bytes, wide)) {
        return false;
    }
    wjson::JsonObject root{nullptr};
    if (!wjson::JsonObject::TryParse(wide, root) ||
        root.GetNamedString(L"format") != L"explorer-folder-bookmarks-bar" ||
        root.GetNamedNumber(L"version") != 1) {
        return false;
    }
    auto list = root.GetNamedArray(L"folders");
    if (list.Size() > kMaxBookmarks) {
        return false;
    }
    std::vector<std::wstring> folders;
    folders.reserve(list.Size());
    for (unsigned i = 0; i < list.Size(); ++i) {
        if (list.GetAt(i).ValueType() != wjson::JsonValueType::String) {
            return false;
        }
        auto value = list.GetStringAt(i);
        folders.emplace_back(value.c_str());
    }
    if (!ValidateImportedFolders(folders)) {
        return false;
    }
    std::lock_guard lock(g_storageMutex);
    return SaveBookmarksLocked(folders);
} catch (...) {
    Wh_Log(L"Bookmark backup validation failed: %08X",
           winrt::to_hresult().value);
    return false;
}

bool AddBookmark(std::wstring path) {
    path = NormalizePath(std::move(path));
    if (path.empty() || path.find_first_of(L"\r\n") != std::wstring::npos) {
        return false;
    }
    std::lock_guard lock(g_storageMutex);
    auto folders = SplitBookmarks(ReadStorageLocked());
    if (folders.size() >= kMaxBookmarks ||
        std::any_of(folders.begin(), folders.end(),
                    [&](const auto& old) { return SamePath(old, path); })) {
        return false;
    }
    folders.push_back(std::move(path));
    return SaveBookmarksLocked(folders);
}

bool RemoveBookmark(const std::wstring& path) {
    std::lock_guard lock(g_storageMutex);
    auto folders = SplitBookmarks(ReadStorageLocked());
    size_t oldSize = folders.size();
    std::erase_if(folders, [&](const auto& old) { return SamePath(old, path); });
    return oldSize != folders.size() && SaveBookmarksLocked(folders);
}

bool MoveBookmarkToIndex(const std::wstring& source, size_t insertionIndex) {
    std::lock_guard lock(g_storageMutex);
    auto folders = SplitBookmarks(ReadStorageLocked());
    if (insertionIndex > folders.size()) {
        return false;
    }
    auto sourceIt = std::find_if(folders.begin(), folders.end(),
                                 [&](const auto& path) {
                                     return SamePath(path, source);
                                 });
    if (sourceIt == folders.end()) {
        return false;
    }
    size_t sourceIndex = std::distance(folders.begin(), sourceIt);
    if (sourceIndex < insertionIndex) {
        --insertionIndex;
    }
    if (sourceIndex == insertionIndex) {
        return false;
    }
    std::wstring moved = std::move(*sourceIt);
    folders.erase(sourceIt);
    folders.insert(folders.begin() + insertionIndex, std::move(moved));
    return SaveBookmarksLocked(folders);
}

// Inserts the paths that are not bookmarked yet, in their given order, while
// room remains. Returns how many were inserted.
size_t InsertMissingLocked(std::vector<std::wstring>& folders,
                           const std::vector<std::wstring>& paths,
                           size_t insertionIndex) {
    insertionIndex = std::min(insertionIndex, folders.size());
    size_t added = 0;
    for (auto path : paths) {
        path = NormalizePath(std::move(path));
        if (folders.size() >= kMaxBookmarks) {
            break;
        }
        if (!IsAbsoluteFolderPath(path) ||
            std::any_of(folders.begin(), folders.end(),
                        [&](const auto& old) { return SamePath(old, path); })) {
            continue;
        }
        folders.insert(folders.begin() + insertionIndex + added,
                       std::move(path));
        ++added;
    }
    return added;
}

// insertionIndex past the end appends.
bool InsertBookmarks(const std::vector<std::wstring>& paths,
                     size_t insertionIndex) {
    std::lock_guard lock(g_storageMutex);
    auto folders = SplitBookmarks(ReadStorageLocked());
    return InsertMissingLocked(folders, paths, insertionIndex) != 0 &&
           SaveBookmarksLocked(folders);
}

bool InsertBookmarkAt(const std::wstring& path, size_t insertionIndex) {
    return InsertBookmarks({path}, insertionIndex);
}

// Removes the paths when every one of them is bookmarked; otherwise appends
// the missing ones.
bool ToggleBookmarks(const std::vector<std::wstring>& paths) {
    std::vector<std::wstring> normalized;
    for (const auto& path : paths) {
        if (auto value = NormalizePath(path); !value.empty()) {
            normalized.push_back(std::move(value));
        }
    }
    if (normalized.empty()) {
        return false;
    }
    std::lock_guard lock(g_storageMutex);
    auto folders = SplitBookmarks(ReadStorageLocked());
    auto bookmarked = [&](const std::wstring& path) {
        return std::any_of(folders.begin(), folders.end(),
                           [&](const auto& old) { return SamePath(old, path); });
    };
    if (std::all_of(normalized.begin(), normalized.end(), bookmarked)) {
        std::erase_if(folders, [&](const auto& old) {
            return std::any_of(normalized.begin(), normalized.end(),
                               [&](const auto& path) {
                                   return SamePath(old, path);
                               });
        });
        return SaveBookmarksLocked(folders);
    }
    return InsertMissingLocked(folders, normalized, folders.size()) != 0 &&
           SaveBookmarksLocked(folders);
}

std::wstring ButtonLabel(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos || pos + 1 == path.size()) {
        return path;
    }
    return path.substr(pos + 1);
}

struct FxFolder {
    std::wstring label;
    std::wstring path;
};

std::wstring TrimSetting(std::wstring value) {
    auto nonSpace = [](wchar_t ch) { return !iswspace(ch); };
    auto first = std::find_if(value.begin(), value.end(), nonSpace);
    if (first == value.end()) {
        return {};
    }
    auto last = std::find_if(value.rbegin(), value.rend(), nonSpace).base();
    return std::wstring(first, last);
}

std::wstring ReadFxSetting(int index, const wchar_t* field) {
    std::wstring name = L"fxCustomFolders[" + std::to_wstring(index) +
                        L"]." + field;
    auto value = WindhawkUtils::StringSetting::make(name.c_str());
    return TrimSetting(value.get() ? value.get() : L"");
}

std::wstring ExpandFxPath(const std::wstring& raw) {
    if (raw.empty() || raw.size() > 4096) {
        return {};
    }
    DWORD required = ExpandEnvironmentStringsW(raw.c_str(), nullptr, 0);
    if (required == 0 || required > 32768) {
        return {};
    }
    std::wstring expanded(required, L'\0');
    DWORD written = ExpandEnvironmentStringsW(raw.c_str(), expanded.data(),
                                               required);
    if (written == 0 || written > required) {
        return {};
    }
    expanded.resize(written - 1);
    auto path = NormalizePath(TrimSetting(std::move(expanded)));
    return IsAbsoluteFolderPath(path) ? path : std::wstring{};
}

std::vector<FxFolder> LoadFxCustomFolders() {
    std::vector<FxFolder> folders;
    for (int index = 0; index < static_cast<int>(kMaxCustomFxFolders);
         ++index) {
        auto rawPath = ReadFxSetting(index, L"path");
        if (rawPath.empty()) {
            continue;
        }
        auto path = ExpandFxPath(rawPath);
        if (path.empty()) {
            Wh_Log(L"Skipping invalid FX folder setting at index %d", index);
            continue;
        }
        // Like bookmarks, only a folder known to be missing on a local fixed
        // drive is hidden; network and removable paths are never probed.
        if (CheckFolderStatus(path) == FolderStatus::Missing) {
            continue;
        }
        if (std::any_of(folders.begin(), folders.end(),
                        [&](const FxFolder& entry) {
                            return SamePath(entry.path, path);
                        })) {
            continue;
        }
        auto label = ReadFxSetting(index, L"label");
        if (label.empty()) {
            label = ButtonLabel(path);
        }
        if (label.size() > 80 ||
            std::any_of(label.begin(), label.end(), [](wchar_t ch) {
                return ch < 32;
            })) {
            Wh_Log(L"Skipping invalid FX folder label at index %d", index);
            continue;
        }
        folders.push_back({std::move(label), std::move(path)});
    }
    return folders;
}

struct ComScope {
    HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ~ComScope() {
        if (SUCCEEDED(result)) {
            CoUninitialize();
        }
    }
};

void LoadRecentSettings() {
    g_recentSettings.enabled = Wh_GetIntSetting(L"recentFolders") != 0;
    g_recentSettings.buttons = static_cast<size_t>(
        std::clamp(Wh_GetIntSetting(L"recentButtons"), 1, kMaxRecentButtons));
    g_recentSettings.menuButton = Wh_GetIntSetting(L"recentMenuButton") != 0;
    g_recentSettings.history = static_cast<size_t>(std::clamp(
        Wh_GetIntSetting(L"recentHistory"),
        static_cast<int>(g_recentSettings.buttons), kMaxRecentHistory));
    g_recentSettings.rightClickClears =
        Wh_GetIntSetting(L"recentRightClickClears") != 0;
}

ULONGLONG FileTimeValue(const FILETIME& time) {
    return (static_cast<ULONGLONG>(time.dwHighDateTime) << 32) |
           time.dwLowDateTime;
}

ULONGLONG CurrentFileTime() {
    FILETIME now{};
    GetSystemTimeAsFileTime(&now);
    return FileTimeValue(now);
}

ULONGLONG ParseFileTime(const std::wstring& text) {
    return text.empty() ? 0 : wcstoull(text.c_str(), nullptr, 16);
}

std::wstring FormatFileTime(ULONGLONG time) {
    wchar_t text[17]{};
    swprintf(text, ARRAYSIZE(text), L"%016llX", time);
    return text;
}

RecentSource ReadRecentSourceLocked() {
    return ReadValueLocked(kRecentSourceKey) == L"windows"
               ? RecentSource::Windows
               : RecentSource::Explorer;
}

void SaveRecentSource(RecentSource source) {
    std::lock_guard lock(g_storageMutex);
    Wh_SetStringValue(kRecentSourceKey, source == RecentSource::Windows
                                            ? L"windows"
                                            : L"explorer");
}

// Returns true when the stored order changed.
bool RecordExplorerRecent(std::wstring path) {
    path = NormalizePath(std::move(path));
    if (!IsAbsoluteFolderPath(path)) {
        return false;
    }
    std::lock_guard lock(g_storageMutex);
    auto recents = SplitPaths(ReadValueLocked(kRecentsKey), kMaxStoredRecents);
    if (!recents.empty() && SamePath(recents.front(), path)) {
        return false;
    }
    std::erase_if(recents, [&](const auto& old) { return SamePath(old, path); });
    recents.insert(recents.begin(), std::move(path));
    if (recents.size() > kMaxStoredRecents) {
        recents.resize(kMaxStoredRecents);
    }
    return SaveLinesLocked(kRecentsKey, std::move(recents));
}

// A hidden Windows recent item stays hidden until Windows records a newer use.
struct HiddenRecent {
    ULONGLONG hiddenAt = 0;
    std::wstring path;
};

std::vector<HiddenRecent> ReadHiddenWindowsRecentsLocked() {
    std::vector<HiddenRecent> hidden;
    for (const auto& line : SplitLines(ReadValueLocked(kWindowsRecentHiddenKey),
                                       kMaxStoredRecents)) {
        size_t tab = line.find(L'\t');
        if (tab == std::wstring::npos || tab + 1 == line.size()) {
            continue;
        }
        hidden.push_back({ParseFileTime(line.substr(0, tab)),
                          NormalizePath(line.substr(tab + 1))});
    }
    return hidden;
}

void HideWindowsRecentLocked(const std::wstring& path) {
    auto hidden = ReadHiddenWindowsRecentsLocked();
    std::erase_if(hidden, [&](const auto& entry) {
        return SamePath(entry.path, path);
    });
    hidden.insert(hidden.begin(), HiddenRecent{CurrentFileTime(), path});
    if (hidden.size() > kMaxStoredRecents) {
        hidden.resize(kMaxStoredRecents);
    }
    std::vector<std::wstring> lines;
    for (const auto& entry : hidden) {
        lines.push_back(FormatFileTime(entry.hiddenAt) + L'\t' + entry.path);
    }
    SaveLinesLocked(kWindowsRecentHiddenKey, std::move(lines));
}

void RemoveRecent(RecentSource source, const std::wstring& path) {
    std::lock_guard lock(g_storageMutex);
    if (source == RecentSource::Windows) {
        HideWindowsRecentLocked(path);
        return;
    }
    auto recents = SplitPaths(ReadValueLocked(kRecentsKey), kMaxStoredRecents);
    size_t oldSize = recents.size();
    std::erase_if(recents, [&](const auto& old) { return SamePath(old, path); });
    if (recents.size() != oldSize) {
        SaveLinesLocked(kRecentsKey, std::move(recents));
    }
}

// Clearing Windows recent items only hides them from the bar; the shortcuts
// in the user's Recent folder belong to Windows and are left untouched.
void ClearRecents(RecentSource source) {
    std::lock_guard lock(g_storageMutex);
    if (source == RecentSource::Windows) {
        Wh_SetStringValue(kWindowsRecentClearedKey,
                          FormatFileTime(CurrentFileTime()).c_str());
        Wh_SetStringValue(kWindowsRecentHiddenKey, L"");
    } else {
        Wh_SetStringValue(kRecentsKey, L"");
    }
}

bool ReadRegistryDword(HKEY root, const wchar_t* key, const wchar_t* name,
                       DWORD& value) {
    DWORD size = sizeof(value);
    return RegGetValueW(root, key, name, RRF_RT_REG_DWORD, nullptr, &value,
                        &size) == ERROR_SUCCESS;
}

// Windows stops adding recent items when the user turns off recent files in
// Start settings or a policy disables recent document history.
bool WindowsRecentsTurnedOff() {
    constexpr wchar_t kPolicies[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer";
    constexpr wchar_t kAdvanced[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
    DWORD value = 0;
    if ((ReadRegistryDword(HKEY_CURRENT_USER, kPolicies,
                           L"NoRecentDocsHistory", value) && value) ||
        (ReadRegistryDword(HKEY_LOCAL_MACHINE, kPolicies,
                           L"NoRecentDocsHistory", value) && value)) {
        return true;
    }
    return ReadRegistryDword(HKEY_CURRENT_USER, kAdvanced, L"Start_TrackDocs",
                             value) && value == 0;
}

struct TimedFolder {
    std::wstring path;
    ULONGLONG usedAt = 0;
};

// Windows adds a shortcut to the folder of each opened file to the Recent
// folder. The shortcut's stored attributes identify folder targets without
// touching possibly slow network or removable storage.
std::vector<TimedFolder> ScanWindowsRecentFolders() {
    std::vector<TimedFolder> folders;
    auto directory = KnownFolderPath(FOLDERID_Recent);
    if (directory.empty()) {
        return folders;
    }
    struct LinkFile {
        std::wstring name;
        ULONGLONG writtenAt = 0;
    };
    std::vector<LinkFile> links;
    WIN32_FIND_DATAW data{};
    HANDLE find = FindFirstFileExW((directory + L"\\*.lnk").c_str(),
                                   FindExInfoBasic, &data,
                                   FindExSearchNameMatch, nullptr,
                                   FIND_FIRST_EX_LARGE_FETCH);
    if (find == INVALID_HANDLE_VALUE) {
        return folders;
    }
    do {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            links.push_back({data.cFileName, FileTimeValue(data.ftLastWriteTime)});
        }
    } while (links.size() < 4096 && FindNextFileW(find, &data));
    FindClose(find);
    std::sort(links.begin(), links.end(), [](const auto& a, const auto& b) {
        return a.writtenAt > b.writtenAt;
    });
    if (links.size() > kMaxRecentLinksScanned) {
        links.resize(kMaxRecentLinksScanned);
    }
    ComScope com;
    for (const auto& link : links) {
        if (folders.size() >= kMaxStoredRecents) {
            break;
        }
        winrt::com_ptr<IShellLinkW> shellLink;
        if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr,
                                    CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(shellLink.put())))) {
            break;
        }
        auto file = shellLink.try_as<IPersistFile>();
        if (!file ||
            FAILED(file->Load((directory + L"\\" + link.name).c_str(),
                              STGM_READ))) {
            continue;
        }
        wchar_t target[MAX_PATH]{};
        WIN32_FIND_DATAW targetData{};
        if (shellLink->GetPath(target, ARRAYSIZE(target), &targetData, 0) !=
                S_OK ||
            !target[0]) {
            continue;
        }
        auto path = NormalizePath(target);
        if (!IsAbsoluteFolderPath(path) ||
            std::any_of(folders.begin(), folders.end(), [&](const auto& old) {
                return SamePath(old.path, path);
            })) {
            continue;
        }
        bool folder = targetData.dwFileAttributes
                          ? (targetData.dwFileAttributes &
                             FILE_ATTRIBUTE_DIRECTORY) != 0
                          : CheckFolderStatus(path) == FolderStatus::Available;
        if (folder) {
            folders.push_back({std::move(path), link.writtenAt});
        }
    }
    return folders;
}

std::mutex g_windowsRecentsMutex;
std::vector<TimedFolder> g_windowsRecents;
ULONGLONG g_windowsRecentsScannedAt = 0;
bool g_windowsRecentsScanned = false;

std::vector<TimedFolder> CachedWindowsRecentFolders() {
    std::lock_guard lock(g_windowsRecentsMutex);
    ULONGLONG now = GetTickCount64();
    if (!g_windowsRecentsScanned ||
        now - g_windowsRecentsScannedAt >= kWindowsRecentCacheMs) {
        g_windowsRecents = ScanWindowsRecentFolders();
        g_windowsRecentsScannedAt = now;
        g_windowsRecentsScanned = true;
    }
    return g_windowsRecents;
}

struct RecentView {
    RecentSource source = RecentSource::Explorer;
    bool windowsRecentsOff = false;
    // Newest first, at most the configured history length.
    std::vector<std::wstring> folders;
};

RecentView LoadRecentView(const std::vector<std::wstring>& bookmarks) {
    RecentView view;
    std::vector<std::wstring> candidates;
    ULONGLONG clearedAt = 0;
    std::vector<HiddenRecent> hidden;
    {
        std::lock_guard lock(g_storageMutex);
        view.source = ReadRecentSourceLocked();
        if (view.source == RecentSource::Explorer) {
            candidates =
                SplitPaths(ReadValueLocked(kRecentsKey), kMaxStoredRecents);
        } else {
            clearedAt = ParseFileTime(ReadValueLocked(kWindowsRecentClearedKey));
            hidden = ReadHiddenWindowsRecentsLocked();
        }
    }
    if (view.source == RecentSource::Windows) {
        if (WindowsRecentsTurnedOff()) {
            view.windowsRecentsOff = true;
            return view;
        }
        for (auto& entry : CachedWindowsRecentFolders()) {
            bool hide = entry.usedAt <= clearedAt ||
                        std::any_of(hidden.begin(), hidden.end(),
                                    [&](const auto& item) {
                                        return entry.usedAt <= item.hiddenAt &&
                                               SamePath(item.path, entry.path);
                                    });
            if (!hide) {
                candidates.push_back(std::move(entry.path));
            }
        }
    }
    for (auto& path : candidates) {
        if (view.folders.size() >= g_recentSettings.history) {
            break;
        }
        auto samePath = [&](const auto& other) { return SamePath(other, path); };
        if (std::any_of(bookmarks.begin(), bookmarks.end(), samePath) ||
            std::any_of(view.folders.begin(), view.folders.end(), samePath) ||
            CheckFolderStatus(path) == FolderStatus::Missing) {
            continue;
        }
        view.folders.push_back(std::move(path));
    }
    return view;
}

// File dialogs and recent-folder tracking can run a nested message loop on an
// Explorer thread; unloading waits until no such operation is on any stack.
struct OperationScope {
    bool active = false;
    OperationScope() {
        std::lock_guard lock(g_operationMutex);
        if (!g_unloading) {
            ++g_activeOperations;
            active = true;
        }
    }
    ~OperationScope() {
        if (active) {
            std::lock_guard lock(g_operationMutex);
            --g_activeOperations;
            g_operationFinished.notify_all();
        }
    }
};

std::wstring ChooseBackupPath(HWND owner, bool save,
                              const std::wstring& suggestedPath) {
    ComScope com;
    if (FAILED(com.result)) {
        return {};
    }
    winrt::com_ptr<IFileDialog> dialog;
    const CLSID& dialogClass = save ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    if (FAILED(CoCreateInstance(dialogClass, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(dialog.put())))) {
        return {};
    }
    FILEOPENDIALOGOPTIONS options = 0;
    if (FAILED(dialog->GetOptions(&options)) ||
        FAILED(dialog->SetOptions(
            options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST |
            (save ? FOS_OVERWRITEPROMPT : FOS_FILEMUSTEXIST)))) {
        return {};
    }
    const COMDLG_FILTERSPEC filters[] = {{L"JSON files", L"*.json"},
                                         {L"All files", L"*.*"}};
    dialog->SetFileTypes(ARRAYSIZE(filters), filters);
    dialog->SetDefaultExtension(L"json");
    auto separator = suggestedPath.find_last_of(L"\\/");
    if (separator != std::wstring::npos) {
        dialog->SetFileName(suggestedPath.c_str() + separator + 1);
        winrt::com_ptr<IShellItem> folder;
        if (SUCCEEDED(SHCreateItemFromParsingName(
                suggestedPath.substr(0, separator).c_str(), nullptr,
                IID_PPV_ARGS(folder.put())))) {
            dialog->SetDefaultFolder(folder.get());
        }
    }
    if (g_unloading) {
        return {};
    }
    g_threadFileDialog = dialog.get();
    HRESULT showResult = dialog->Show(owner);
    g_threadFileDialog = nullptr;
    if (FAILED(showResult)) {
        return {};
    }
    winrt::com_ptr<IShellItem> selected;
    if (FAILED(dialog->GetResult(selected.put()))) {
        return {};
    }
    PWSTR rawPath = nullptr;
    if (FAILED(selected->GetDisplayName(SIGDN_FILESYSPATH, &rawPath)) ||
        !rawPath) {
        CoTaskMemFree(rawPath);
        return {};
    }
    std::wstring path(rawPath);
    CoTaskMemFree(rawPath);
    return path;
}

winrt::com_ptr<IShellBrowser> ActiveShellBrowser(HWND explorerWindow) {
    winrt::com_ptr<IShellWindows> shellWindows;
    if (FAILED(CoCreateInstance(kShellWindows, nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(shellWindows.put())))) {
        return nullptr;
    }

    HWND activeTab = FindWindowExW(explorerWindow, nullptr,
                                   L"ShellTabWindowClass", nullptr);
    long count = 0;
    if (FAILED(shellWindows->get_Count(&count))) {
        return nullptr;
    }

    for (long i = 0; i < count; ++i) {
        VARIANT index{};
        index.vt = VT_I4;
        index.lVal = i;
        winrt::com_ptr<IDispatch> dispatch;
        if (FAILED(shellWindows->Item(index, dispatch.put())) || !dispatch) {
            continue;
        }
        auto webBrowser = dispatch.try_as<IWebBrowser2>();
        SHANDLE_PTR windowValue = 0;
        if (!webBrowser || FAILED(webBrowser->get_HWND(&windowValue)) ||
            reinterpret_cast<HWND>(windowValue) != explorerWindow) {
            continue;
        }
        auto serviceProvider = dispatch.try_as<IServiceProvider>();
        if (!serviceProvider) {
            continue;
        }
        winrt::com_ptr<IShellBrowser> browser;
        if (FAILED(serviceProvider->QueryService(
                kTopLevelBrowser, IID_PPV_ARGS(browser.put())))) {
            continue;
        }
        HWND tabWindow = nullptr;
        if (FAILED(browser->GetWindow(&tabWindow)) || !tabWindow) {
            continue;
        }
        if ((activeTab && tabWindow != activeTab) ||
            (!activeTab && !IsWindowVisible(tabWindow))) {
            continue;
        }
        return browser;
    }
    return nullptr;
}

bool IsExplorerFrame(HWND window) {
    wchar_t className[64]{};
    return window &&
           GetClassNameW(window, className, ARRAYSIZE(className)) &&
           wcscmp(className, L"CabinetWClass") == 0;
}

// The Explorer frame that hosts this element's XAML island, or null.
HWND IslandWindow(const mux::UIElement& element) {
    try {
        if (auto root = element.XamlRoot()) {
            if (auto environment = root.ContentIslandEnvironment()) {
                HWND window = GetAncestor(
                    reinterpret_cast<HWND>(static_cast<uintptr_t>(
                        environment.AppWindowId().Value)),
                    GA_ROOT);
                return IsExplorerFrame(window) ? window : nullptr;
            }
        }
    } catch (...) {
        Wh_Log(L"Could not locate Explorer window: %08X",
               winrt::to_hresult().value);
    }
    return nullptr;
}

HWND ExplorerWindowForElement(const mux::FrameworkElement& element) {
    if (HWND window = IslandWindow(element)) {
        return window;
    }
    HWND window = GetActiveWindow();
    window = window ? GetAncestor(window, GA_ROOT) : nullptr;
    return IsExplorerFrame(window) ? window : nullptr;
}

std::wstring ShellBrowserFolder(IShellBrowser* browser) {
    winrt::com_ptr<IShellView> view;
    if (FAILED(browser->QueryActiveShellView(view.put())) || !view) {
        return {};
    }
    auto folderView = view.try_as<IFolderView>();
    winrt::com_ptr<IPersistFolder2> persistFolder;
    if (!folderView ||
        FAILED(folderView->GetFolder(IID_PPV_ARGS(persistFolder.put())))) {
        return {};
    }
    PIDLIST_ABSOLUTE pidl = nullptr;
    if (FAILED(persistFolder->GetCurFolder(&pidl)) || !pidl) {
        return {};
    }
    std::vector<wchar_t> path(32768);
    bool ok = SHGetPathFromIDListEx(pidl, path.data(), path.size(),
                                    GPFIDL_DEFAULT);
    CoTaskMemFree(pidl);
    return ok ? NormalizePath(path.data()) : std::wstring{};
}

std::wstring CurrentFolder(HWND explorerWindow) {
    ComScope com;
    auto browser = ActiveShellBrowser(explorerWindow);
    return browser ? ShellBrowserFolder(browser.get()) : std::wstring{};
}

// Filesystem folders selected in the active tab. ZIP archives and other items
// that are both a folder and a file are left out.
std::vector<std::wstring> SelectedFolders(HWND explorerWindow) {
    std::vector<std::wstring> folders;
    ComScope com;
    auto browser = ActiveShellBrowser(explorerWindow);
    winrt::com_ptr<IShellView> view;
    if (!browser || FAILED(browser->QueryActiveShellView(view.put())) ||
        !view) {
        return folders;
    }
    auto folderView = view.try_as<IFolderView>();
    winrt::com_ptr<IShellItemArray> items;
    DWORD count = 0;
    if (!folderView ||
        FAILED(folderView->Items(SVGIO_SELECTION,
                                 IID_PPV_ARGS(items.put()))) ||
        !items || FAILED(items->GetCount(&count))) {
        return folders;
    }
    constexpr SFGAOF kFilesystemFolder = SFGAO_FOLDER | SFGAO_FILESYSTEM;
    for (DWORD i = 0; i < count && folders.size() < kMaxFoldersPerAction;
         ++i) {
        winrt::com_ptr<IShellItem> item;
        SFGAOF attributes = 0;
        if (FAILED(items->GetItemAt(i, item.put())) ||
            FAILED(item->GetAttributes(kFilesystemFolder | SFGAO_STREAM,
                                       &attributes)) ||
            (attributes & kFilesystemFolder) != kFilesystemFolder ||
            (attributes & SFGAO_STREAM)) {
            continue;
        }
        PWSTR path = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
            folders.push_back(NormalizePath(path));
        }
        CoTaskMemFree(path);
    }
    return folders;
}

void NavigateToFolder(HWND explorerWindow, const std::wstring& path) {
    ComScope com;
    auto browser = ActiveShellBrowser(explorerWindow);
    if (!browser) {
        return;
    }
    PIDLIST_ABSOLUTE pidl = nullptr;
    HRESULT hr = SHParseDisplayName(path.c_str(), nullptr, &pidl, 0, nullptr);
    if (SUCCEEDED(hr) && pidl) {
        hr = browser->BrowseObject(pidl, SBSP_ABSOLUTE | SBSP_SAMEBROWSER);
        CoTaskMemFree(pidl);
    }
    if (FAILED(hr)) {
        Wh_Log(L"Bookmark navigation failed: %08X", hr);
    }
}

void OpenFolderInNewTab(HWND explorerWindow, const std::wstring& path) {
    // Windows 11 registers this Folder shell verb for Explorer tabs. Explorer
    // chooses which window receives the tab; HWND is the owner for the request.
    HINSTANCE result = ShellExecuteW(explorerWindow, L"opennewtab",
                                     path.c_str(), nullptr, nullptr,
                                     SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        Wh_Log(L"Bookmark new-tab navigation failed: %d",
               static_cast<int>(reinterpret_cast<INT_PTR>(result)));
    } else {
        Wh_Log(L"Bookmark opened in a new tab: %ls", path.c_str());
    }
}

void OpenFolderInNewWindow(HWND explorerWindow, const std::wstring& path) {
    // The Folder verb behind Explorer's "Open in new window" command.
    HINSTANCE result = ShellExecuteW(explorerWindow, L"opennewwindow",
                                     path.c_str(), nullptr, nullptr,
                                     SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        Wh_Log(L"Bookmark new-window navigation failed: %d",
               static_cast<int>(reinterpret_cast<INT_PTR>(result)));
    } else {
        Wh_Log(L"Bookmark opened in a new window: %ls", path.c_str());
    }
}

// Opens the folder in the Explorer window that hosts anchor: Ctrl+click
// opens a new tab, an ordinary click navigates the active tab.
void OpenFolderFrom(const mux::FrameworkElement& anchor,
                    const std::wstring& path) {
    HWND window = anchor ? ExplorerWindowForElement(anchor) : nullptr;
    if (!window) {
        return;
    }
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        OpenFolderInNewTab(window, path);
    } else {
        NavigateToFolder(window, path);
    }
}

BarState* FindState(const muxc::StackPanel& panel) {
    for (auto& state : g_bars) {
        if (state.buttons.get() == panel) {
            return &state;
        }
    }
    return nullptr;
}

std::vector<muxc::Button> BarButtons(const muxc::StackPanel& panel) {
    std::vector<muxc::Button> buttons;
    for (const auto& rowElement : panel.Children()) {
        if (auto row = rowElement.try_as<muxc::StackPanel>()) {
            for (const auto& child : row.Children()) {
                if (auto button = child.try_as<muxc::Button>()) {
                    buttons.push_back(button);
                }
            }
        }
    }
    return buttons;
}

// Ask Explorer to recalculate its stock header after the mod is unloaded.
void RelayoutThreadFrames() {
    EnumThreadWindows(
        GetCurrentThreadId(),
        [](HWND window, LPARAM) -> BOOL {
            if (IsExplorerFrame(window) && !IsIconic(window)) {
                SetWindowPos(window, nullptr, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                                 SWP_NOACTIVATE | SWP_FRAMECHANGED);
                RECT client{};
                if (GetClientRect(window, &client)) {
                    SendMessageW(window, WM_SIZE,
                                 IsZoomed(window) ? SIZE_MAXIMIZED
                                                  : SIZE_RESTORED,
                                 MAKELPARAM(client.right, client.bottom));
                }
            }
            return TRUE;
        },
        0);
}

// A native WM_SIZE has no callback into the mod and runs after the current
// XAML layout pass, so row changes do not reenter Explorer's layout code.
void PostFrameRelayout() {
    EnumThreadWindows(
        GetCurrentThreadId(),
        [](HWND window, LPARAM) -> BOOL {
            if (IsExplorerFrame(window) && !IsIconic(window)) {
                RECT client{};
                if (GetClientRect(window, &client)) {
                    PostMessageW(window, WM_SIZE,
                                 IsZoomed(window) ? SIZE_MAXIMIZED
                                                  : SIZE_RESTORED,
                                 MAKELPARAM(client.right, client.bottom));
                }
            }
            return TRUE;
        },
        0);
}

void ClearDrag(BarState& state);

// A mouse press can end without PointerReleased when Alt+Tab or a system
// dialog takes the mouse. Once the button is physically up the drag is over,
// so refreshes never leave a drag operation active.
bool DragInProgress(BarState& state) {
    if (state.dragPath.empty()) {
        return false;
    }
    if (state.dragIsMouse) {
        int button = GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
        if (!(GetAsyncKeyState(button) & 0x8000)) {
            ClearDrag(state);
            return false;
        }
    }
    return true;
}

void ClearDropTarget(BarState& state) {
    if (auto button = state.dropTarget.get()) {
        try {
            // These bookmark buttons are ours; Tag holds the original brush
            // on the button itself instead of in thread-local state.
            auto oldBrush = button.Tag().try_as<muxm::Brush>();
            button.BorderBrush(oldBrush);
            button.Tag(nullptr);
            button.BorderThickness(state.dropOldThickness);
        } catch (...) {
            Wh_Log(L"Could not restore bookmark drag border: %08X",
                   winrt::to_hresult().value);
        }
    }
    state.dropTarget = nullptr;
    state.dropAfter = false;
}

void ClearDrag(BarState& state) {
    ClearDropTarget(state);
    if (auto button = state.dragButton.get()) {
        try {
            button.Opacity(state.dragOldOpacity);
        } catch (...) {
            Wh_Log(L"Could not restore bookmark drag opacity: %08X",
                   winrt::to_hresult().value);
        }
    }
    state.dragPath.clear();
    state.dragButton = nullptr;
    state.dragPointerId = 0;
    state.dragIsMouse = false;
    state.dragging = false;
    state.dragFromRecents = false;
}

void UpdateFrameRowCount() {
    unsigned rows = 0;
    for (const auto& bar : g_bars) {
        if (bar.strip.get()) {
            rows = std::max(rows, bar.rowCount);
        }
    }
    rows = std::min(rows, kMaxRows);
    if (rows != g_frameRows) {
        g_frameRows = rows;
        if (!g_unloading) {
            PostFrameRelayout();
        }
    }
}

void SetBarRowCount(BarState& state, unsigned count) {
    count = std::clamp(count, 1u, kMaxRows);
    if (count == state.rowCount) {
        return;
    }
    const double height = RowAllocation(count);
    if (state.addedRow) {
        state.addedRow.Height(
            mux::GridLength{height, mux::GridUnitType::Pixel});
    }
    auto strip = state.strip.get();
    if (strip) {
        strip.Height(height);
    }
    if (auto root = state.barRoot.get()) {
        root.Height(height);
    }
    if (auto grid = state.grid.get()) {
        if (grid.MinHeight() == state.appliedGridMinHeight) {
            state.appliedGridMinHeight =
                std::max(state.oldGridMinHeight,
                         state.originalGridHeight + height);
            grid.MinHeight(state.appliedGridMinHeight);
        }
        grid.InvalidateMeasure();
    }
    if (auto nav = state.navControl.get()) {
        if (nav.MinHeight() == state.appliedNavMinHeight) {
            state.appliedNavMinHeight =
                std::max(state.oldNavMinHeight,
                         state.originalNavHeight + height);
            nav.MinHeight(state.appliedNavMinHeight);
        }
        nav.InvalidateMeasure();
    }
    if (auto host = state.hostGrid.get()) {
        host.InvalidateMeasure();
    }
    state.rowCount = count;
    if (strip) {
        UpdateFrameRowCount();
    }
}

struct ReflowGuard {
    bool& active;
    ~ReflowGuard() { active = false; }
};

double MeasuredWidth(const mux::FrameworkElement& element) {
    element.Measure(winrt::Windows::Foundation::Size{10000, kRowHeight});
    double desired = element.DesiredSize().Width;
    if (!std::isfinite(desired) || desired <= 0) {
        auto margin = element.Margin();
        desired = element.ActualWidth() + margin.Left + margin.Right;
    }
    return std::max(1.0, desired);
}

// Rows needed by the same greedy wrapping that ReflowPanel applies. Rows past
// kMaxRows are counted too, so overflow compares as worse than a full row four.
unsigned WrappedRows(const std::vector<double>& widths, double width) {
    unsigned rows = 1;
    double used = 0;
    for (double desired : widths) {
        if (used > 0 && used + desired > width + 0.5) {
            ++rows;
            used = 0;
        }
        used += desired;
    }
    return rows;
}

// Shows as many recent buttons as fit without adding bookmark rows and
// returns the width the recents column needs. With RC, the buttons fold into
// RC together; without it, the oldest (leftmost) buttons are hidden first.
double FitRecents(BarState& state, double width,
                  const std::vector<double>& bookmarkWidths) {
    state.visibleRecents = 0;
    auto recents = state.recents.get();
    if (!recents || recents.Children().Size() == 0) {
        return 0;
    }
    // Children: separator, recent buttons from oldest to newest, then RC.
    std::vector<mux::FrameworkElement> items;
    for (const auto& child : recents.Children()) {
        if (auto element = child.try_as<mux::FrameworkElement>()) {
            element.Visibility(mux::Visibility::Visible);
            items.push_back(element);
        }
    }
    const size_t count = state.recentButtonCount;
    const bool hasMenu = state.hasRecentMenuButton;
    if (items.size() != 1 + count + (hasMenu ? 1 : 0)) {
        return 0;
    }
    std::vector<double> widths;
    for (const auto& item : items) {
        widths.push_back(MeasuredWidth(item));
    }
    const double fixed = widths[0] + (hasMenu ? widths.back() : 0);
    auto newestWidth = [&](size_t shown) {
        double total = 0;
        for (size_t i = count + 1 - shown; i <= count; ++i) {
            total += widths[i];
        }
        return total;
    };
    size_t shown = count;
    if (hasMenu) {
        if (count && WrappedRows(bookmarkWidths, width - fixed - newestWidth(count)) >
                         WrappedRows(bookmarkWidths, width - fixed)) {
            shown = 0;
        }
    } else {
        unsigned baseline = WrappedRows(bookmarkWidths, width);
        while (shown > 0 &&
               WrappedRows(bookmarkWidths,
                           width - fixed - newestWidth(shown)) > baseline) {
            --shown;
        }
    }
    for (size_t i = 1; i <= count; ++i) {
        items[i].Visibility(i > count - shown ? mux::Visibility::Visible
                                              : mux::Visibility::Collapsed);
    }
    const bool anyShown = hasMenu || shown > 0;
    items[0].Visibility(anyShown ? mux::Visibility::Visible
                                 : mux::Visibility::Collapsed);
    state.visibleRecents = shown;
    return anyShown ? fixed + newestWidth(shown) : 0;
}

void ReflowPanel(const muxc::StackPanel& panel, double width) try {
    auto state = FindState(panel);
    if (!state || state->reflowing || DragInProgress(*state) ||
        !std::isfinite(width) || width < 1 || width > 100000 ||
        (std::fabs(state->lastLayoutWidth - width) < 0.5 &&
         panel.Children().Size() != 0)) {
        return;
    }
    state->reflowing = true;
    ReflowGuard guard{state->reflowing};
    auto buttons = BarButtons(panel);
    if (buttons.empty()) {
        return;
    }
    std::vector<double> widths;
    widths.reserve(buttons.size());
    for (const auto& button : buttons) {
        widths.push_back(MeasuredWidth(button));
    }
    const double available =
        std::max(1.0, width - FitRecents(*state, width, widths));
    // Keep strong button refs before detaching the old rows. Reparent only
    // after all measurements succeed, so a measurement failure leaves the
    // current visual tree intact.
    for (const auto& rowElement : panel.Children()) {
        if (auto row = rowElement.try_as<muxc::StackPanel>()) {
            row.Children().Clear();
        }
    }
    panel.Children().Clear();
    muxc::StackPanel row;
    row.Orientation(muxc::Orientation::Horizontal);
    panel.Children().Append(row);
    unsigned rowCount = 1;
    double used = 0;
    for (size_t i = 0; i < buttons.size(); ++i) {
        const double desired = widths[i];
        if (used > 0 && used + desired > available + 0.5 &&
            rowCount < kMaxRows) {
            muxc::StackPanel nextRow;
            nextRow.Orientation(muxc::Orientation::Horizontal);
            nextRow.Margin(mux::Thickness{0, kInterRowGap, 0, 0});
            panel.Children().Append(nextRow);
            row = nextRow;
            ++rowCount;
            used = 0;
        }
        row.Children().Append(buttons[i]);
        used += desired;
    }
    state->lastLayoutWidth = width;
    SetBarRowCount(*state, rowCount);
} catch (...) {
    Wh_Log(L"Bookmark auto-row layout failed: %08X",
           winrt::to_hresult().value);
}

size_t BookmarkDropIndex(const muxc::StackPanel& panel,
                         winrt::Windows::Foundation::Point position) {
    auto rows = panel.Children();
    if (rows.Size() == 0) {
        return 0;
    }
    const double pitch = kRowHeight + kInterRowGap;
    unsigned targetRow = static_cast<unsigned>(std::clamp(
        static_cast<int>(position.Y / pitch), 0,
        static_cast<int>(rows.Size()) - 1));
    size_t buttonIndex = 0;
    size_t bookmarkIndex = 0;
    for (unsigned rowIndex = 0; rowIndex < rows.Size(); ++rowIndex) {
        auto row = rows.GetAt(rowIndex).try_as<muxc::StackPanel>();
        if (!row) {
            continue;
        }
        double offset = 0;
        for (const auto& child : row.Children()) {
            auto button = child.try_as<muxc::Button>();
            if (!button) {
                continue;
            }
            auto margin = button.Margin();
            double midpoint = offset + margin.Left +
                              button.ActualWidth() / 2;
            if (buttonIndex >= kFixedButtons) {
                if (rowIndex == targetRow && position.X < midpoint) {
                    return bookmarkIndex;
                }
                ++bookmarkIndex;
            }
            offset += margin.Left + button.ActualWidth() + margin.Right;
            ++buttonIndex;
        }
        if (rowIndex == targetRow) {
            return bookmarkIndex;
        }
    }
    return bookmarkIndex;
}

void ShowInsertionMark(BarState& state, const muxc::StackPanel& panel,
                       size_t insertionIndex) {
    // + and FX are fixed controls, so only bookmarks receive a drop marker.
    auto children = BarButtons(panel);
    if (children.size() <= kFixedButtons) {
        ClearDropTarget(state);
        return;
    }
    size_t bookmarkCount = children.size() - kFixedButtons;
    size_t index = std::min(insertionIndex, bookmarkCount);
    bool after = index == bookmarkCount;
    auto button = children[kFixedButtons + (after ? index - 1 : index)];
    if (!button) {
        ClearDropTarget(state);
        return;
    }
    if (state.dropTarget.get() == button && state.dropAfter == after) {
        return;
    }
    ClearDropTarget(state);
    try {
        state.dropTarget = winrt::make_weak(button);
        button.Tag(button.BorderBrush());
        state.dropOldThickness = button.BorderThickness();
        state.dropAfter = after;
        button.BorderBrush(muxm::SolidColorBrush(
            winrt::Windows::UI::Color{255, 255, 210, 60}));
        button.BorderThickness(after ? mux::Thickness{0, 0, 3, 0}
                                     : mux::Thickness{3, 0, 0, 0});
    } catch (...) {
        Wh_Log(L"Could not show bookmark drop position: %08X",
               winrt::to_hresult().value);
        ClearDropTarget(state);
    }
}

void RefreshPanel(const muxc::StackPanel& panel);

muxc::FontIcon MakeFluentIcon(const wchar_t* glyph) {
    muxc::FontIcon icon;
    icon.Glyph(glyph);
    icon.FontFamily(muxm::FontFamily{L"Segoe Fluent Icons"});
    icon.FontSize(18);
    icon.VerticalAlignment(mux::VerticalAlignment::Center);
    return icon;
}

struct ScopedIcon {
    HICON value = nullptr;
    ~ScopedIcon() {
        if (value) {
            DestroyIcon(value);
        }
    }
};

// Ask Shell for this particular folder's icon. It applies desktop.ini icon
// customizations and the user's icon cache; USEFILEATTRIBUTES would skip them.
std::optional<IconPixels> FolderIconPixels(const std::wstring& path) {
    SHFILEINFOW info{};
    if (!SHGetFileInfoW(path.c_str(), 0, &info, sizeof(info),
                        SHGFI_ICON | SHGFI_LARGEICON) || !info.hIcon) {
        return std::nullopt;
    }
    ScopedIcon shellIcon{info.hIcon};
    try {
        ComScope com;
        winrt::com_ptr<IWICImagingFactory> factory;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                    CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(factory.put())))) {
            return std::nullopt;
        }
        winrt::com_ptr<IWICBitmap> source;
        if (FAILED(factory->CreateBitmapFromHICON(shellIcon.value,
                                                  source.put()))) {
            return std::nullopt;
        }
        UINT width = 0;
        UINT height = 0;
        if (FAILED(source->GetSize(&width, &height)) || width == 0 ||
            height == 0 || width > 256 || height > 256) {
            return std::nullopt;
        }
        winrt::com_ptr<IWICFormatConverter> converter;
        if (FAILED(factory->CreateFormatConverter(converter.put())) ||
            FAILED(converter->Initialize(source.get(),
                                         GUID_WICPixelFormat32bppBGRA,
                                         WICBitmapDitherTypeNone, nullptr, 0,
                                         WICBitmapPaletteTypeCustom))) {
            return std::nullopt;
        }
        UINT byteCount = width * height * 4;
        IconPixels result{static_cast<int>(width), static_cast<int>(height),
                          std::vector<BYTE>(byteCount)};
        if (FAILED(converter->CopyPixels(nullptr, width * 4, byteCount,
                                         result.bytes.data()))) {
            return std::nullopt;
        }
        return result;
    } catch (...) {
        Wh_Log(L"Failed to load bookmark folder icon: %08X",
               winrt::to_hresult().value);
        return std::nullopt;
    }
}

muxmi::WriteableBitmap BitmapFromPixels(const IconPixels& pixels) try {
    muxmi::WriteableBitmap bitmap(pixels.width, pixels.height);
    auto buffer = bitmap.PixelBuffer();
    if (buffer.Length() < pixels.bytes.size()) {
        return nullptr;
    }
    auto access = buffer.as<::Windows::Storage::Streams::IBufferByteAccess>();
    BYTE* destination = nullptr;
    if (FAILED(access->Buffer(&destination)) || !destination) {
        return nullptr;
    }
    std::memcpy(destination, pixels.bytes.data(), pixels.bytes.size());
    bitmap.Invalidate();
    return bitmap;
} catch (...) {
    Wh_Log(L"Failed to create bookmark icon bitmap: %08X",
           winrt::to_hresult().value);
    return nullptr;
}

void ForgetCachedIcon(const std::wstring& path) {
    std::erase_if(g_iconCache, [&](const auto& entry) {
        return SamePath(entry.path, path);
    });
}

muxmi::WriteableBitmap CachedFolderBitmap(const std::wstring& path) {
    ULONGLONG now = GetTickCount64();
    auto it = std::find_if(g_iconCache.begin(), g_iconCache.end(),
                           [&](const auto& entry) {
                               return SamePath(entry.path, path);
                           });
    if (it == g_iconCache.end()) {
        if (g_iconCache.size() >= kMaxCachedIcons) {
            auto oldest = std::min_element(
                g_iconCache.begin(), g_iconCache.end(),
                [](const auto& a, const auto& b) {
                    return a.usedAt < b.usedAt;
                });
            g_iconCache.erase(oldest);
        }
        g_iconCache.emplace_back();
        it = std::prev(g_iconCache.end());
        it->path = path;
    }
    it->usedAt = now;
    ULONGLONG lifetime = it->pixels ? kIconCacheLifetimeMs
                                   : kFailedIconCacheLifetimeMs;
    if (!it->attempted || now - it->loadedAt >= lifetime) {
        it->pixels = FolderIconPixels(path);
        it->bitmap = {};
        it->attempted = true;
        it->loadedAt = now;
    }
    if (!it->pixels) {
        return nullptr;
    }
    if (auto bitmap = it->bitmap.get()) {
        return bitmap;
    }
    auto bitmap = BitmapFromPixels(*it->pixels);
    if (bitmap) {
        it->bitmap = winrt::make_weak(bitmap);
    }
    return bitmap;
}

mux::UIElement FolderIcon(const std::wstring& path, FolderStatus status) {
    if (status == FolderStatus::Missing) {
        ForgetCachedIcon(path);
        auto warning = MakeFluentIcon(L"\uE7BA");
        warning.Foreground(muxm::SolidColorBrush(
            winrt::Windows::UI::Color{255, 255, 210, 60}));
        return warning;
    }
    if (status == FolderStatus::Unknown) {
        // Avoid a potentially blocking Shell icon handler for remote or
        // inaccessible paths; keep the bookmark usable with a generic icon.
        return MakeFluentIcon(L"\uE8B7");
    }
    if (auto bitmap = CachedFolderBitmap(path)) {
        muxc::Image image;
        image.Source(bitmap);
        image.Width(18);
        image.Height(18);
        image.VerticalAlignment(mux::VerticalAlignment::Center);
        return image;
    }
    return MakeFluentIcon(L"\uE8B7");
}

void RefreshThreadBars() {
    std::vector<winrt::weak_ref<muxc::StackPanel>> panels;
    for (const auto& bar : g_bars) {
        panels.push_back(bar.buttons);
    }
    for (const auto& weakPanel : panels) {
        if (auto panel = weakPanel.get()) {
            RefreshPanel(panel);
        }
    }
}

muxc::Button MakeLabelButton(const wchar_t* text) {
    muxc::Button button;
    muxc::TextBlock label;
    label.Text(text);
    label.FontSize(13);
    button.Content(label);
    button.Width(kButtonHeight);
    button.Height(kButtonHeight);
    button.MinWidth(0);
    button.MinHeight(0);
    button.Padding(mux::Thickness{0, 0, 0, 0});
    button.HorizontalContentAlignment(mux::HorizontalAlignment::Center);
    button.VerticalContentAlignment(mux::VerticalAlignment::Center);
    return button;
}

muxc::Button MakeFolderButton(const std::wstring& path, FolderStatus status,
                              const std::wstring& tooltip) {
    muxc::Button button;
    muxc::StackPanel content;
    content.Orientation(muxc::Orientation::Horizontal);
    content.VerticalAlignment(mux::VerticalAlignment::Center);
    content.Children().Append(FolderIcon(path, status));
    muxc::TextBlock label;
    label.Text(winrt::hstring(ButtonLabel(path)));
    label.VerticalAlignment(mux::VerticalAlignment::Center);
    label.Margin(mux::Thickness{8, 0, 0, 0});
    label.MaxWidth(155);
    label.FontSize(14);
    label.TextTrimming(mux::TextTrimming::CharacterEllipsis);
    if (status == FolderStatus::Missing) {
        label.Opacity(0.7);
    }
    content.Children().Append(label);
    button.Content(content);
    button.Height(kButtonHeight);
    button.MaxWidth(208);
    button.Padding(mux::Thickness{12, 0, 12, 0});
    button.VerticalContentAlignment(mux::VerticalAlignment::Center);
    button.Margin(mux::Thickness{0, kButtonVerticalInset, 8,
                                 kButtonVerticalInset});
    muxc::ToolTipService::SetToolTip(
        button, winrt::box_value(winrt::hstring(tooltip)));
    return button;
}

// Drops land only on the bookmark strip, not on the recents beside it.
bool PointerInsideStrip(const BarState& state,
                        const muxi::PointerRoutedEventArgs& args) {
    auto strip = state.strip.get();
    if (!strip) {
        return false;
    }
    auto position = args.GetCurrentPoint(strip).Position();
    return position.X >= 0 && position.Y >= 0 &&
           position.X <= strip.ActualWidth() &&
           position.Y <= strip.ActualHeight();
}

// Click opens the folder, right-click opens it in a new window, middle-click
// removes the bookmark or recent entry, and a left drag reorders a bookmark
// or turns a recent folder into one.
void AttachFolderButtonHandlers(BarState& state, const muxc::Button& button,
                                const std::wstring& path,
                                const winrt::weak_ref<muxc::StackPanel>& weakPanel,
                                bool recent, RecentSource source) {
    // Buttons consume left presses. AddHandler receives them after the
    // class handler and keeps ordinary Click behavior for a short press.
    TrackPointer(state.panelHandlers, button,
        mux::UIElement::PointerPressedEvent(),
        muxi::PointerEventHandler{
            [path, weakPanel, recent, source](
                const winrt::Windows::Foundation::IInspectable& sender,
                const muxi::PointerRoutedEventArgs& args) {
                if (g_unloading) {
                    return;
                }
                auto panel = weakPanel.get();
                auto state = panel ? FindState(panel) : nullptr;
                auto element = sender.try_as<muxc::Button>();
                if (!state || !element) {
                    return;
                }
                auto point = args.GetCurrentPoint(panel);
                if (point.Properties().IsMiddleButtonPressed()) {
                    args.Handled(true);
                    ClearDrag(*state);
                    if (recent) {
                        RemoveRecent(source, path);
                        RefreshPanel(panel);
                    } else if (RemoveBookmark(path)) {
                        RefreshPanel(panel);
                    }
                } else if (point.Properties().IsLeftButtonPressed()) {
                    ClearDrag(*state);
                    state->suppressClick.clear();
                    state->dragPath = path;
                    state->dragFromRecents = recent;
                    state->dragButton = winrt::make_weak(element);
                    state->dragOldOpacity = element.Opacity();
                    state->dragPointerId = point.PointerId();
                    state->dragIsMouse =
                        args.Pointer().PointerDeviceType() ==
                        winrt::Microsoft::UI::Input::PointerDeviceType::Mouse;
                    state->dragStart = point.Position();
                    state->dragging = false;
                    element.CapturePointer(args.Pointer());
                }
            }});
    TrackPointer(state.panelHandlers, button,
        mux::UIElement::PointerMovedEvent(),
        muxi::PointerEventHandler{
            [weakPanel](const winrt::Windows::Foundation::IInspectable& sender,
                        const muxi::PointerRoutedEventArgs& args) {
                auto panel = weakPanel.get();
                auto state = panel ? FindState(panel) : nullptr;
                auto element = sender.try_as<muxc::Button>();
                if (!state || !element || state->dragButton.get() != element) {
                    return;
                }
                auto point = args.GetCurrentPoint(panel);
                if (point.PointerId() != state->dragPointerId ||
                    !point.Properties().IsLeftButtonPressed()) {
                    return;
                }
                auto position = point.Position();
                float dx = position.X - state->dragStart.X;
                float dy = position.Y - state->dragStart.Y;
                if (dx * dx + dy * dy >= kDragThreshold * kDragThreshold) {
                    if (!state->dragging) {
                        element.Opacity(0.55);
                    }
                    state->dragging = true;
                    state->suppressClick = state->dragPath;
                    if (PointerInsideStrip(*state, args)) {
                        ShowInsertionMark(*state, panel,
                                          BookmarkDropIndex(panel, position));
                    } else {
                        ClearDropTarget(*state);
                    }
                    args.Handled(true);
                }
            }});
    TrackPointer(state.panelHandlers, button,
        mux::UIElement::PointerReleasedEvent(),
        muxi::PointerEventHandler{
            [weakPanel](const winrt::Windows::Foundation::IInspectable& sender,
                        const muxi::PointerRoutedEventArgs& args) {
                auto panel = weakPanel.get();
                auto state = panel ? FindState(panel) : nullptr;
                auto element = sender.try_as<muxc::Button>();
                if (!state || !element || state->dragButton.get() != element) {
                    return;
                }
                auto point = args.GetCurrentPoint(panel);
                if (point.PointerId() != state->dragPointerId) {
                    return;
                }
                auto position = point.Position();
                bool dragging = state->dragging;
                bool fromRecents = state->dragFromRecents;
                auto source = state->dragPath;
                bool inside = PointerInsideStrip(*state, args);
                ClearDrag(*state);
                if (!dragging) {
                    return;
                }
                args.Handled(true);
                if (!inside) {
                    return;
                }
                size_t index = BookmarkDropIndex(panel, position);
                if (fromRecents ? InsertBookmarkAt(source, index)
                                : MoveBookmarkToIndex(source, index)) {
                    RefreshPanel(panel);
                }
            }});
    // Touch and pen contact can be canceled without a release; mouse
    // presses are covered by DragInProgress.
    TrackPointer(state.panelHandlers, button,
        mux::UIElement::PointerCanceledEvent(),
        muxi::PointerEventHandler{
            [weakPanel](const winrt::Windows::Foundation::IInspectable& sender,
                        const muxi::PointerRoutedEventArgs&) {
                auto panel = weakPanel.get();
                auto state = panel ? FindState(panel) : nullptr;
                auto element = sender.try_as<muxc::Button>();
                if (!state || !element || state->dragButton.get() != element) {
                    return;
                }
                ClearDrag(*state);
            }});
    TrackClick(state.panelHandlers, button, [path, weakPanel](
                     const winrt::Windows::Foundation::IInspectable& sender,
                     const mux::RoutedEventArgs&) {
        if (g_unloading) {
            return;
        }
        if (auto panel = weakPanel.get()) {
            if (auto state = FindState(panel);
                state && SamePath(state->suppressClick, path)) {
                state->suppressClick.clear();
                return;
            }
        }
        OpenFolderFrom(sender.try_as<mux::FrameworkElement>(), path);
    });
    // Also raised by the context menu key and by press-and-hold on touch.
    TrackContextRequested(state.panelHandlers, button, [path](
                              const mux::UIElement& sender,
                              const muxi::ContextRequestedEventArgs& args) {
        args.Handled(true);
        if (g_unloading) {
            return;
        }
        auto element = sender.try_as<mux::FrameworkElement>();
        if (HWND window = element ? ExplorerWindowForElement(element) : nullptr) {
            OpenFolderInNewWindow(window, path);
        }
    });
}

void ClearRecentsFromBar(const winrt::weak_ref<muxc::StackPanel>& weakPanel) {
    if (g_unloading) {
        return;
    }
    auto panel = weakPanel.get();
    auto state = panel ? FindState(panel) : nullptr;
    if (!state) {
        return;
    }
    ClearRecents(state->recentSource);
    RefreshThreadBars();
}

void AppendDisabledMenuItem(const muxc::MenuFlyout& menu, const wchar_t* text,
                            const wchar_t* tooltip = nullptr) {
    muxc::MenuFlyoutItem item;
    item.Text(text);
    item.IsEnabled(false);
    if (tooltip) {
        muxc::ToolTipService::SetToolTip(item, winrt::box_value(tooltip));
    }
    menu.Items().Append(item);
}

// Rebuilt on each opening, so it matches the buttons the last reflow showed.
void FillRecentMenu(BarState& state, const muxc::MenuFlyout& menu,
                    const winrt::weak_ref<muxc::Button>& weakAnchor) {
    RevokeHandlers(state.recentMenuHandlers);
    menu.Items().Clear();
    if (state.windowsRecentsOff) {
        AppendDisabledMenuItem(
            menu, L"Windows is not keeping recent items",
            L"Recent files are turned off in Settings > Personalization > "
            L"Start, or by a policy. Choose File Explorer history below, or "
            L"turn recent files on in Windows.");
    } else if (state.visibleRecents >= state.recentFolders.size()) {
        AppendDisabledMenuItem(menu, state.recentFolders.empty()
                                         ? L"No recent folders"
                                         : L"No other recent folders");
    } else {
        for (size_t i = state.visibleRecents; i < state.recentFolders.size();
             ++i) {
            const auto& path = state.recentFolders[i];
            muxc::MenuFlyoutItem item;
            item.Text(winrt::hstring(ButtonLabel(path)));
            muxc::ToolTipService::SetToolTip(
                item, winrt::box_value(winrt::hstring(path)));
            TrackClick(state.recentMenuHandlers, item, [path, weakAnchor](
                           const winrt::Windows::Foundation::IInspectable&,
                           const mux::RoutedEventArgs&) {
                if (!g_unloading) {
                    OpenFolderFrom(weakAnchor.get(), path);
                }
            });
            menu.Items().Append(item);
        }
    }
    menu.Items().Append(muxc::MenuFlyoutSeparator{});
    auto appendSource = [&](const wchar_t* text, RecentSource source) {
        muxc::RadioMenuFlyoutItem item;
        item.Text(text);
        item.GroupName(L"RecentSource");
        item.IsChecked(state.recentSource == source);
        TrackClick(state.recentMenuHandlers, item, [source](
                       const winrt::Windows::Foundation::IInspectable&,
                       const mux::RoutedEventArgs&) {
            if (g_unloading) {
                return;
            }
            SaveRecentSource(source);
            RefreshThreadBars();
        });
        menu.Items().Append(item);
    };
    appendSource(L"Folders opened in File Explorer", RecentSource::Explorer);
    appendSource(L"Windows recent items", RecentSource::Windows);
}

void AppendRecents(BarState& state, const muxc::StackPanel& recents,
                   const winrt::weak_ref<muxc::StackPanel>& weakPanel) {
    muxc::Border separator;
    separator.Width(1);
    separator.Height(20);
    separator.Margin(mux::Thickness{4, 0, 12, 0});
    separator.VerticalAlignment(mux::VerticalAlignment::Center);
    separator.Background(muxm::SolidColorBrush(
        winrt::Windows::UI::Color{160, 128, 128, 128}));
    recents.Children().Append(separator);

    state.recentButtonCount =
        std::min(state.recentFolders.size(), g_recentSettings.buttons);
    // The newest folder sits next to the right edge.
    for (size_t i = state.recentButtonCount; i-- > 0;) {
        const auto& path = state.recentFolders[i];
        auto button = MakeFolderButton(
            path, CheckFolderStatus(path),
            path + L"\nCtrl+click for new tab; right-click for new window"
                   L"\nDrag onto the bookmarks to keep"
                   L"\nMiddle-click to remove from recent folders");
        AttachFolderButtonHandlers(state, button, path, weakPanel, true,
                                   state.recentSource);
        recents.Children().Append(button);
    }

    // RC stays available while no recent folder has a button, so the source
    // can always be changed.
    state.hasRecentMenuButton =
        g_recentSettings.menuButton || state.recentButtonCount == 0;
    if (!state.hasRecentMenuButton) {
        return;
    }
    auto rcButton = MakeLabelButton(L"RC");
    rcButton.Margin(mux::Thickness{0, kButtonVerticalInset, 8,
                                   kButtonVerticalInset});
    muxc::ToolTipService::SetToolTip(
        rcButton,
        winrt::box_value(g_recentSettings.rightClickClears
                             ? L"RC: more recent folders and their source; "
                               L"right-click clears the list"
                             : L"RC: more recent folders and their source; "
                               L"right-click to clear the list"));
    auto weakRc = winrt::make_weak(rcButton);
    muxc::MenuFlyout recentMenu;
    TrackOpening(state.panelHandlers, recentMenu, [weakPanel, weakRc](
                     const winrt::Windows::Foundation::IInspectable& sender,
                     const winrt::Windows::Foundation::IInspectable&) {
        auto panel = weakPanel.get();
        auto state = panel ? FindState(panel) : nullptr;
        auto menu = sender.try_as<muxc::MenuFlyout>();
        if (!g_unloading && state && menu) {
            FillRecentMenu(*state, menu, weakRc);
        }
    });
    // A flyout without items does not open, so Opening would never run.
    AppendDisabledMenuItem(recentMenu, L"No recent folders");
    rcButton.Flyout(recentMenu);
    if (g_recentSettings.rightClickClears) {
        TrackContextRequested(
            state.panelHandlers, rcButton,
            [weakPanel](const mux::UIElement&,
                        const muxi::ContextRequestedEventArgs& args) {
                args.Handled(true);
                ClearRecentsFromBar(weakPanel);
            });
    } else {
        muxc::MenuFlyout clearMenu;
        muxc::MenuFlyoutItem clearItem;
        clearItem.Text(L"Clear recent folders");
        TrackClick(state.panelHandlers, clearItem, [weakPanel](
                       const winrt::Windows::Foundation::IInspectable&,
                       const mux::RoutedEventArgs&) {
            ClearRecentsFromBar(weakPanel);
        });
        clearMenu.Items().Append(clearItem);
        rcButton.ContextFlyout(clearMenu);
    }
    recents.Children().Append(rcButton);
}

void RefreshPanel(const muxc::StackPanel& panel) {
    if (g_unloading) {
        return;
    }
    auto state = FindState(panel);
    if (!state) {
        return;
    }
    std::wstring storage;
    {
        std::lock_guard lock(g_storageMutex);
        storage = ReadStorageLocked();
    }
    const auto bookmarks = SplitBookmarks(storage);
    RecentView recentView;
    std::wstring renderKey = storage;
    if (g_recentSettings.enabled) {
        recentView = LoadRecentView(bookmarks);
        renderKey += recentView.source == RecentSource::Windows
                         ? L"\x1f" L"W"
                         : L"\x1f" L"E";
        if (recentView.windowsRecentsOff) {
            renderKey += L"\x1f" L"off";
        }
        for (const auto& path : recentView.folders) {
            renderKey += L'\x1f';
            renderKey += path;
        }
    }
    ULONGLONG now = GetTickCount64();
    if (DragInProgress(*state) && panel.Children().Size() != 0) {
        return;
    }
    if (renderKey == state->renderedKey && panel.Children().Size() != 0 &&
        state->lastFolderCheckAt != 0 &&
        now - state->lastFolderCheckAt < kFolderCheckIntervalMs) {
        return;
    }
    ClearDrag(*state);
    RevokeHandlers(state->recentMenuHandlers);
    RevokeHandlers(state->driveHandlers);
    RevokeHandlers(state->panelHandlers);
    panel.Children().Clear();
    muxc::StackPanel firstRow;
    firstRow.Orientation(muxc::Orientation::Horizontal);
    panel.Children().Append(firstRow);
    auto weakPanel = winrt::make_weak(panel);

    muxc::Button addButton;
    auto addIcon = MakeFluentIcon(L"\uE710");
    addIcon.FontSize(16);
    addButton.Content(addIcon);
    addButton.Width(kButtonHeight);
    addButton.Height(kButtonHeight);
    addButton.MinWidth(0);
    addButton.MinHeight(0);
    addButton.Padding(mux::Thickness{0, 0, 0, 0});
    addButton.HorizontalContentAlignment(mux::HorizontalAlignment::Center);
    addButton.VerticalContentAlignment(mux::VerticalAlignment::Center);
    addButton.Margin(mux::Thickness{8, kButtonVerticalInset, 8,
                                    kButtonVerticalInset});
    muxc::ToolTipService::SetToolTip(
        addButton, winrt::box_value(g_bookmarkHotkey
                                        ? L"Bookmark this folder (Ctrl+B)"
                                        : L"Bookmark this folder"));
    // ContextFlyout opens on right-click; ordinary left-click still adds the
    // current folder.
    std::wstring backupPath = SuggestedBackupPath();
    muxc::MenuFlyout backupMenu;
    muxc::MenuFlyoutItem saveItem;
    saveItem.Text(L"Save bookmarks");
    muxc::ToolTipService::SetToolTip(
        saveItem, winrt::box_value(L"Choose a JSON backup file"));
    TrackClick(state->panelHandlers, saveItem, [backupPath, weakPanel](
                       const winrt::Windows::Foundation::IInspectable& sender,
                       const mux::RoutedEventArgs&) {
        OperationScope operation;
        if (!operation.active) {
            return;
        }
        auto panel = weakPanel.get();
        HWND window = panel ? ExplorerWindowForElement(panel) : nullptr;
        auto selectedPath = ChooseBackupPath(window, true, backupPath);
        if (selectedPath.empty() || g_unloading) {
            return;
        }
        bool ok = SaveBackup(selectedPath);
        if (ok) {
            Wh_Log(L"Bookmarks saved to %ls", selectedPath.c_str());
        } else {
            Wh_Log(L"Could not save bookmarks to %ls", selectedPath.c_str());
        }
        if (auto item = sender.try_as<muxc::MenuFlyoutItem>()) {
            muxc::ToolTipService::SetToolTip(
                item, winrt::box_value(winrt::hstring(
                          std::wstring(ok ? L"Saved to " : L"Save failed: ") +
                          selectedPath)));
        }
    });
    backupMenu.Items().Append(saveItem);
    muxc::MenuFlyoutItem loadItem;
    loadItem.Text(L"Load bookmarks");
    muxc::ToolTipService::SetToolTip(
        loadItem, winrt::box_value(L"Choose a JSON backup to load"));
    TrackClick(state->panelHandlers, loadItem, [backupPath, weakPanel](
                       const winrt::Windows::Foundation::IInspectable& sender,
                       const mux::RoutedEventArgs&) {
        OperationScope operation;
        if (!operation.active) {
            return;
        }
        auto panel = weakPanel.get();
        HWND window = panel ? ExplorerWindowForElement(panel) : nullptr;
        auto selectedPath = ChooseBackupPath(window, false, backupPath);
        if (selectedPath.empty() || g_unloading) {
            return;
        }
        bool ok = LoadBackup(selectedPath);
        if (ok) {
            Wh_Log(L"Bookmarks loaded from %ls", selectedPath.c_str());
        } else {
            Wh_Log(L"Could not load bookmarks from %ls",
                   selectedPath.c_str());
        }
        if (auto item = sender.try_as<muxc::MenuFlyoutItem>()) {
            muxc::ToolTipService::SetToolTip(
                item, winrt::box_value(winrt::hstring(
                          std::wstring(ok ? L"Loaded from " : L"Load failed: ") +
                          selectedPath)));
        }
        if (ok) {
            if (auto livePanel = weakPanel.get()) {
                RefreshPanel(livePanel);
            }
        }
    });
    backupMenu.Items().Append(loadItem);
    addButton.ContextFlyout(backupMenu);
    TrackClick(state->panelHandlers, addButton, [weakPanel](
                        const winrt::Windows::Foundation::IInspectable& sender,
                        const mux::RoutedEventArgs&) {
        if (g_unloading) {
            return;
        }
        auto button = sender.try_as<mux::FrameworkElement>();
        HWND window = button ? ExplorerWindowForElement(button) : nullptr;
        if (window && AddBookmark(CurrentFolder(window))) {
            if (auto panel = weakPanel.get()) {
                RefreshPanel(panel);
            }
        }
    });
    firstRow.Children().Append(addButton);

    auto fxButton = MakeLabelButton(L"FX");
    fxButton.Margin(mux::Thickness{0, kButtonVerticalInset, 16,
                                   kButtonVerticalInset});
    muxc::ToolTipService::SetToolTip(
        fxButton, winrt::box_value(
            L"FX: left-click for folders; right-click for drives"));
    auto weakFxButton = winrt::make_weak(fxButton);
    muxc::MenuFlyout foldersMenu;
    muxc::MenuFlyout drivesMenu;
    auto appendLocation = [weakFxButton, state](muxc::MenuFlyout& menu,
                              const std::wstring& label,
                              const std::wstring& path,
                              bool driveItem = false) {
        if (path.empty()) {
            return;
        }
        muxc::MenuFlyoutItem item;
        item.Text(label);
        muxc::ToolTipService::SetToolTip(
            item, winrt::box_value(winrt::hstring(path)));
        TrackClick(driveItem ? state->driveHandlers : state->panelHandlers,
                   item, [path, weakFxButton](
                       const winrt::Windows::Foundation::IInspectable&,
                       const mux::RoutedEventArgs&) {
            if (!g_unloading) {
                OpenFolderFrom(weakFxButton.get(), path);
            }
        });
        menu.Items().Append(item);
    };
    appendLocation(foldersMenu, L"~", KnownFolderPath(FOLDERID_Profile));
    appendLocation(foldersMenu, L"Desktop", KnownFolderPath(FOLDERID_Desktop));
    appendLocation(foldersMenu, L"Documents", KnownFolderPath(FOLDERID_Documents));
    appendLocation(foldersMenu, L"Downloads", KnownFolderPath(FOLDERID_Downloads));
    const auto customFolders = LoadFxCustomFolders();
    if (!customFolders.empty()) {
        foldersMenu.Items().Append(muxc::MenuFlyoutSeparator{});
        for (const auto& folder : customFolders) {
            appendLocation(foldersMenu, folder.label, folder.path);
        }
    }
    // Build the list when FX opens so newly attached drives appear immediately.
    // Only local fixed drives are probed; other drives are listed unchecked.
    TrackOpening(state->panelHandlers, drivesMenu, [appendLocation, state](
                           const winrt::Windows::Foundation::IInspectable& sender,
                           const winrt::Windows::Foundation::IInspectable&) {
        if (g_unloading) {
            return;
        }
        auto menu = sender.try_as<muxc::MenuFlyout>();
        if (!menu) {
            return;
        }
        RevokeHandlers(state->driveHandlers);
        menu.Items().Clear();
        DWORD driveMask = GetLogicalDrives();
        for (unsigned index = 0; index < 26; ++index) {
            if (!(driveMask & (DWORD{1} << index))) {
                continue;
            }
            wchar_t drive[] = {static_cast<wchar_t>(L'A' + index), L':', L'\\', L'\0'};
            if (CheckFolderStatus(drive) == FolderStatus::Missing) {
                continue;
            }
            appendLocation(menu, DriveMenuLabel(drive), drive, true);
        }
        if (menu.Items().Size() == 0) {
            AppendDisabledMenuItem(menu, L"No available drives");
        }
    });
    AppendDisabledMenuItem(drivesMenu, L"No available drives");
    fxButton.Flyout(foldersMenu);
    fxButton.ContextFlyout(drivesMenu);
    firstRow.Children().Append(fxButton);

    for (const auto& path : bookmarks) {
        FolderStatus status = CheckFolderStatus(path);
        std::wstring tooltip = path;
        if (status == FolderStatus::Missing) {
            tooltip += L"\nFolder not found";
        }
        tooltip += L"\nCtrl+click for new tab; right-click for new window"
                   L"\nDrag to reorder; middle-click to remove bookmark";
        auto button = MakeFolderButton(path, status, tooltip);
        AttachFolderButtonHandlers(*state, button, path, weakPanel, false,
                                   recentView.source);
        firstRow.Children().Append(button);
    }

    state->recentFolders = std::move(recentView.folders);
    state->recentSource = recentView.source;
    state->windowsRecentsOff = recentView.windowsRecentsOff;
    state->recentButtonCount = 0;
    state->hasRecentMenuButton = false;
    state->visibleRecents = 0;
    if (auto recents = state->recents.get()) {
        recents.Children().Clear();
        if (g_recentSettings.enabled) {
            AppendRecents(*state, recents, weakPanel);
        }
    }

    state->renderedKey = std::move(renderKey);
    state->lastFolderCheckAt = now;
    state->lastLayoutWidth = 0;
    if (auto root = state->barRoot.get()) {
        ReflowPanel(panel, root.ActualWidth());
    }
}

// File Explorer history: each Explorer UI thread listens to the navigation
// events of its own tabs. ShellWindows events report opened and closed tabs.
void OnShellEvent(DISPID id, DISPPARAMS* params);

class ShellEventSink final : public IDispatch {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,
                                             void** object) override {
        if (!object) {
            return E_POINTER;
        }
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDispatch) ||
            IsEqualIID(iid, kWebBrowserEvents2) ||
            IsEqualIID(iid, kShellWindowsEvents)) {
            *object = static_cast<IDispatch*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refs; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG refs = --m_refs;
        if (refs == 0) {
            delete this;
        }
        return refs;
    }
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* count) override {
        if (!count) {
            return E_POINTER;
        }
        *count = 0;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo**) override {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, LPOLESTR*, UINT, LCID,
                                            DISPID*) override {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE Invoke(DISPID id, REFIID, LCID, WORD,
                                     DISPPARAMS* params, VARIANT*,
                                     EXCEPINFO*, UINT*) override {
        if (!g_unloading) {
            OnShellEvent(id, params);
        }
        return S_OK;
    }

private:
    std::atomic<ULONG> m_refs{1};
};

struct BrowserConnection {
    winrt::com_ptr<IUnknown> identity;
    winrt::com_ptr<IConnectionPoint> point;
    DWORD cookie = 0;
};

struct RecentTracker {
    ShellEventSink* sink = nullptr;
    winrt::com_ptr<IShellWindows> shellWindows;
    winrt::com_ptr<IConnectionPoint> windowsPoint;
    DWORD windowsCookie = 0;
    std::vector<BrowserConnection> browsers;
};

// Deleted only by StopRecentTracking. A thread that ends without cleanup
// leaks its tracker instead of releasing COM objects after COM shut down.
thread_local RecentTracker* g_recentTracker = nullptr;

// Work deferred to a thread timer on this Explorer UI thread.
thread_local UINT_PTR g_workTimer = 0;
thread_local bool g_recentSyncPending = false;
thread_local bool g_refreshPending = false;
thread_local HWND g_bookmarkTogglePending = nullptr;
thread_local bool g_bookmarkToggleFromFileList = false;
thread_local bool g_threadWorking = false;

bool ConnectEvents(IUnknown* source, REFIID events, IDispatch* sink,
                   winrt::com_ptr<IConnectionPoint>& point, DWORD& cookie) {
    winrt::com_ptr<IConnectionPointContainer> container;
    if (FAILED(source->QueryInterface(IID_PPV_ARGS(container.put()))) ||
        FAILED(container->FindConnectionPoint(events, point.put())) ||
        FAILED(point->Advise(sink, &cookie))) {
        point = nullptr;
        return false;
    }
    return true;
}

bool EnsureRecentTracker() {
    if (g_recentTracker) {
        return true;
    }
    auto tracker = std::make_unique<RecentTracker>();
    if (FAILED(CoCreateInstance(kShellWindows, nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(tracker->shellWindows.put())))) {
        return false;
    }
    tracker->sink = new ShellEventSink();
    if (!ConnectEvents(tracker->shellWindows.get(), kShellWindowsEvents,
                       tracker->sink, tracker->windowsPoint,
                       tracker->windowsCookie)) {
        Wh_Log(L"Could not listen for File Explorer windows");
        tracker->sink->Release();
        return false;
    }
    g_recentTracker = tracker.release();
    return true;
}

void StopRecentTracking() {
    RecentTracker* tracker = std::exchange(g_recentTracker, nullptr);
    if (!tracker) {
        return;
    }
    for (auto& browser : tracker->browsers) {
        browser.point->Unadvise(browser.cookie);
    }
    if (tracker->windowsPoint) {
        tracker->windowsPoint->Unadvise(tracker->windowsCookie);
    }
    // Release references that COM stubs hold for other apartments, so no
    // event can reach the sink after the mod is unloaded.
    CoDisconnectObject(tracker->sink, 0);
    tracker->sink->Release();
    delete tracker;
}

// Connects to the tabs whose frame belongs to this thread and forgets the
// tabs that ShellWindows no longer lists.
void SyncBrowserConnections(RecentTracker& tracker) {
    long count = 0;
    if (FAILED(tracker.shellWindows->get_Count(&count))) {
        return;
    }
    const DWORD thread = GetCurrentThreadId();
    std::vector<winrt::com_ptr<IUnknown>> live;
    for (long i = 0; i < count; ++i) {
        VARIANT index{};
        index.vt = VT_I4;
        index.lVal = i;
        winrt::com_ptr<IDispatch> dispatch;
        if (FAILED(tracker.shellWindows->Item(index, dispatch.put())) ||
            !dispatch) {
            continue;
        }
        auto webBrowser = dispatch.try_as<IWebBrowser2>();
        SHANDLE_PTR windowValue = 0;
        if (!webBrowser || FAILED(webBrowser->get_HWND(&windowValue)) ||
            GetWindowThreadProcessId(reinterpret_cast<HWND>(windowValue),
                                     nullptr) != thread) {
            continue;
        }
        auto identity = dispatch.try_as<IUnknown>();
        if (!identity) {
            continue;
        }
        live.push_back(identity);
        if (std::any_of(tracker.browsers.begin(), tracker.browsers.end(),
                        [&](const auto& browser) {
                            return browser.identity == identity;
                        })) {
            continue;
        }
        BrowserConnection connection;
        connection.identity = identity;
        if (ConnectEvents(dispatch.get(), kWebBrowserEvents2, tracker.sink,
                          connection.point, connection.cookie)) {
            tracker.browsers.push_back(std::move(connection));
        }
    }
    std::erase_if(tracker.browsers, [&](auto& browser) {
        if (std::any_of(live.begin(), live.end(), [&](const auto& item) {
                return item == browser.identity;
            })) {
            return false;
        }
        browser.point->Unadvise(browser.cookie);
        return true;
    });
}

void SyncRecentTracking() {
    ComScope com;
    if (!EnsureRecentTracker()) {
        return;
    }
    SyncBrowserConnections(*g_recentTracker);
    Wh_Log(L"Recent tracking: %d tab(s) connected on this thread",
           static_cast<int>(g_recentTracker->browsers.size()));
    if (g_recentTracker->browsers.empty() && g_bars.empty()) {
        StopRecentTracking();
    }
}

// With folders selected in the file list, Ctrl+B toggles those; otherwise it
// toggles the active tab's folder. Virtual locations have no path.
bool ToggleBookmarksForHotkey(HWND window, bool fileListFocused) {
    std::vector<std::wstring> paths;
    if (fileListFocused) {
        paths = SelectedFolders(window);
    }
    if (paths.empty()) {
        if (auto path = CurrentFolder(window); !path.empty()) {
            paths.push_back(std::move(path));
        }
    }
    const bool changed = ToggleBookmarks(paths);
    Wh_Log(L"Ctrl+B: %d folder(s), file list focused=%d, bookmarks changed=%d",
           static_cast<int>(paths.size()), fileListFocused ? 1 : 0,
           changed ? 1 : 0);
    return changed;
}

void RunThreadWork() {
    if (g_threadWorking) {
        return;
    }
    OperationScope operation;
    if (!operation.active) {
        return;
    }
    g_threadWorking = true;
    try {
        while (!g_unloading && (g_recentSyncPending || g_refreshPending ||
                                g_bookmarkTogglePending)) {
            if (HWND window = std::exchange(g_bookmarkTogglePending, nullptr);
                window && ToggleBookmarksForHotkey(
                              window, g_bookmarkToggleFromFileList)) {
                g_refreshPending = true;
            }
            if (std::exchange(g_recentSyncPending, false)) {
                SyncRecentTracking();
            }
            if (std::exchange(g_refreshPending, false)) {
                RefreshThreadBars();
            }
        }
    } catch (...) {
        Wh_Log(L"Bookmarks bar update failed: %08X",
               winrt::to_hresult().value);
    }
    g_threadWorking = false;
}

void CALLBACK ThreadWorkTimerProc(HWND, UINT, UINT_PTR id, DWORD) {
    KillTimer(nullptr, id);
    if (id == g_workTimer) {
        g_workTimer = 0;
    }
    RunThreadWork();
}

// Shell events, hotkeys and drops can arrive inside Explorer's own code or
// during a cross-apartment call, so the work runs later from a thread timer.
void ScheduleThreadWork() {
    if (!g_workTimer && !g_unloading) {
        g_workTimer = SetTimer(nullptr, 0, 0, ThreadWorkTimerProc);
    }
}

void StopThreadWork() {
    if (g_workTimer) {
        KillTimer(nullptr, g_workTimer);
        g_workTimer = 0;
    }
    g_recentSyncPending = false;
    g_refreshPending = false;
    g_bookmarkTogglePending = nullptr;
}

void ScheduleRecentWork(bool sync, bool refresh) {
    if (!g_recentSettings.enabled || g_unloading) {
        return;
    }
    g_recentSyncPending |= sync;
    g_refreshPending |= refresh;
    ScheduleThreadWork();
}

// A thread message hook sees Ctrl+B before Explorer dispatches it, in the
// file list, the navigation pane and the bar; Explorer has no Ctrl+B shortcut.
// It also receives refresh requests that other threads post to the frame.
thread_local HHOOK g_messageHook = nullptr;

UINT RefreshBarsMessage() {
    static const UINT message = RegisterWindowMessageW(
        L"Windhawk_ExplorerFolderBookmarksBar_Refresh");
    return message;
}

bool FileListFocused() {
    HWND focus = GetFocus();
    wchar_t className[64]{};
    return focus && GetClassNameW(focus, className, ARRAYSIZE(className)) &&
           wcscmp(className, L"DirectUIHWND") == 0;
}

bool IsTextInputElement(const winrt::Windows::Foundation::IInspectable& element) {
    return element && (element.try_as<muxc::TextBox>() ||
                       element.try_as<muxc::RichEditBox>() ||
                       element.try_as<muxc::PasswordBox>() ||
                       element.try_as<muxc::AutoSuggestBox>());
}

// Typing keeps Ctrl+B: rename fields are Win32 edit controls, and the address
// and search boxes are XAML text boxes in the frame's island.
bool TextInputFocused(HWND frame) {
    HWND focus = GetFocus();
    wchar_t className[64]{};
    if (focus && GetClassNameW(focus, className, ARRAYSIZE(className))) {
        if (wcscmp(className, L"DirectUIHWND") == 0 ||
            wcscmp(className, L"SysTreeView32") == 0) {
            return false;
        }
        if (_wcsicmp(className, L"Edit") == 0 ||
            _wcsnicmp(className, L"RichEdit", 8) == 0) {
            return true;
        }
    }
    try {
        for (const auto& bar : g_bars) {
            auto root = bar.barRoot.get();
            if (!root || IslandWindow(root) != frame) {
                continue;
            }
            if (auto xamlRoot = root.XamlRoot();
                xamlRoot &&
                IsTextInputElement(muxi::FocusManager::GetFocusedElement(xamlRoot))) {
                return true;
            }
        }
    } catch (...) {
        return true;
    }
    return false;
}

LRESULT CALLBACK ThreadMessageHook(int code, WPARAM wParam, LPARAM lParam) {
    auto message = reinterpret_cast<MSG*>(lParam);
    if (code == HC_ACTION && wParam == PM_REMOVE && !g_unloading &&
        message->message == RefreshBarsMessage()) {
        message->message = WM_NULL;
        g_refreshPending = true;
        ScheduleThreadWork();
    } else if (code == HC_ACTION && wParam == PM_REMOVE && !g_unloading &&
        g_bookmarkHotkey &&
        message->message == WM_KEYDOWN && message->wParam == 'B' &&
        !(message->lParam & (1 << 30)) &&
        (GetKeyState(VK_CONTROL) & 0x8000) &&
        !(GetKeyState(VK_SHIFT) & 0x8000) &&
        !(GetKeyState(VK_MENU) & 0x8000) &&
        !(GetKeyState(VK_LWIN) & 0x8000) &&
        !(GetKeyState(VK_RWIN) & 0x8000)) {
        HWND frame = message->hwnd ? GetAncestor(message->hwnd, GA_ROOT)
                                   : nullptr;
        if (IsExplorerFrame(frame) && !TextInputFocused(frame)) {
            message->message = WM_NULL;
            g_bookmarkTogglePending = frame;
            g_bookmarkToggleFromFileList = FileListFocused();
            ScheduleThreadWork();
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

void InstallMessageHook() {
    if (g_messageHook || g_unloading) {
        return;
    }
    g_messageHook = SetWindowsHookExW(WH_GETMESSAGE, ThreadMessageHook,
                                      nullptr, GetCurrentThreadId());
    if (!g_messageHook) {
        Wh_Log(L"Could not install the message hook: %lu", GetLastError());
    }
}

void RemoveMessageHook() {
    if (g_messageHook) {
        UnhookWindowsHookEx(g_messageHook);
        g_messageHook = nullptr;
    }
}

bool RecordNavigation(IDispatch* dispatch, DISPID id) {
    if (!dispatch) {
        return false;
    }
    winrt::com_ptr<IServiceProvider> provider;
    winrt::com_ptr<IShellBrowser> browser;
    if (FAILED(dispatch->QueryInterface(IID_PPV_ARGS(provider.put()))) ||
        FAILED(provider->QueryService(kTopLevelBrowser,
                                      IID_PPV_ARGS(browser.put())))) {
        return false;
    }
    const auto folder = ShellBrowserFolder(browser.get());
    const bool changed = RecordExplorerRecent(folder);
    Wh_Log(L"Navigation event %d: %ls (%ls)", static_cast<int>(id),
           folder.c_str(), changed ? L"recorded" : L"not recorded");
    return changed;
}

void OnShellEvent(DISPID id, DISPPARAMS* params) try {
    switch (id) {
        case kDispidNavigateComplete2:
        case kDispidDocumentComplete:
            // The first argument is the tab that navigated. NavigateComplete2
            // can still report the previous folder, which is already the
            // newest entry; DocumentComplete then records the new one.
            if (params && params->cArgs >= 2 &&
                params->rgvarg[params->cArgs - 1].vt == VT_DISPATCH &&
                RecordNavigation(params->rgvarg[params->cArgs - 1].pdispVal, id)) {
                ScheduleRecentWork(false, true);
            }
            break;
        case kDispidWindowRegistered:
        case kDispidWindowRevoked:
            ScheduleRecentWork(true, false);
            break;
    }
} catch (...) {
    Wh_Log(L"Recent folder event failed: %08X", winrt::to_hresult().value);
}

namespace wdt = winrt::Windows::ApplicationModel::DataTransfer;

// Drops whose items are still being read. Unloading waits for them only
// briefly, so a read that never finishes cannot keep the mod loaded.
std::atomic<unsigned> g_pendingDrops = 0;

bool HasStorageItems(const mux::DragEventArgs& args) {
    return args.DataView().Contains(wdt::StandardDataFormats::StorageItems());
}

// Over the bookmark strip a drop is inserted at the marked position; over the
// recents it appends.
size_t ExternalDropIndex(const BarState& state, const muxc::StackPanel& panel,
                         const mux::DragEventArgs& args) {
    auto strip = state.strip.get();
    if (!strip) {
        return SIZE_MAX;
    }
    auto position = args.GetPosition(strip);
    if (position.X < 0 || position.Y < 0 || position.X > strip.ActualWidth() ||
        position.Y > strip.ActualHeight()) {
        return SIZE_MAX;
    }
    return BookmarkDropIndex(panel, args.GetPosition(panel));
}

void OnBarDragOver(const muxc::StackPanel& panel,
                   const mux::DragEventArgs& args) {
    auto state = FindState(panel);
    if (!state || g_unloading || !HasStorageItems(args)) {
        args.AcceptedOperation(wdt::DataPackageOperation::None);
        return;
    }
    // Never accept Move: the source would delete folders it believes were
    // moved. Link or Copy leaves them untouched.
    auto allowed = args.AllowedOperations();
    if ((allowed & wdt::DataPackageOperation::Link) ==
        wdt::DataPackageOperation::Link) {
        args.AcceptedOperation(wdt::DataPackageOperation::Link);
    } else if ((allowed & wdt::DataPackageOperation::Copy) ==
               wdt::DataPackageOperation::Copy) {
        args.AcceptedOperation(wdt::DataPackageOperation::Copy);
    } else {
        args.AcceptedOperation(wdt::DataPackageOperation::None);
        return;
    }
    args.DragUIOverride().Caption(L"Add to bookmarks");
    args.DragUIOverride().IsCaptionVisible(true);
    ShowInsertionMark(*state, panel, ExternalDropIndex(*state, panel, args));
}

void OnBarDrop(const muxc::StackPanel& panel, const mux::DragEventArgs& args) {
    auto state = FindState(panel);
    if (!state) {
        return;
    }
    ClearDropTarget(*state);
    if (g_unloading || !HasStorageItems(args)) {
        return;
    }
    if (args.AcceptedOperation() == wdt::DataPackageOperation::None) {
        Wh_Log(L"Drop ignored: no drop operation was accepted");
        return;
    }
    // No drop deferral: File Explorer's file list starts the drag on this
    // thread, and the item read needs this thread while a deferral would
    // keep it waiting inside the drop, freezing the window. The read finishes
    // after the drop returns; the bars then refresh on their own thread
    // through the message hook.
    size_t index = ExternalDropIndex(*state, panel, args);
    HWND frame = IslandWindow(panel);
    Wh_Log(L"Drop accepted: reading items, insertion index %d",
           index == SIZE_MAX ? -1 : static_cast<int>(index));
    auto request = args.DataView().GetStorageItemsAsync();
    ++g_pendingDrops;
    request.Completed([index, frame](
                          const auto& completed,
                          winrt::Windows::Foundation::AsyncStatus status) {
        try {
            if (status != winrt::Windows::Foundation::AsyncStatus::Completed) {
                Wh_Log(L"Dropped items could not be read: status %d",
                       static_cast<int>(status));
            }
            std::vector<std::wstring> paths;
            if (status == winrt::Windows::Foundation::AsyncStatus::Completed) {
                for (const auto& item : completed.GetResults()) {
                    if (paths.size() >= kMaxFoldersPerAction) {
                        break;
                    }
                    if (item.IsOfType(
                            winrt::Windows::Storage::StorageItemTypes::Folder) &&
                        !item.Path().empty()) {
                        paths.emplace_back(item.Path().c_str());
                    }
                }
            }
            const bool inserted = !g_unloading && InsertBookmarks(paths, index);
            Wh_Log(L"Drop read finished: %d folder(s) found, inserted=%d",
                   static_cast<int>(paths.size()), inserted ? 1 : 0);
            if (inserted && frame) {
                PostMessageW(frame, RefreshBarsMessage(), 0, 0);
            }
        } catch (...) {
            Wh_Log(L"Could not bookmark dropped folders: %08X",
                   winrt::to_hresult().value);
        }
        --g_pendingDrops;
    });
}

muxc::Grid FindNavigationGrid(const mux::DependencyObject& root, int depth) {
    if (!root || depth > 64) {
        return nullptr;
    }
    if (auto grid = root.try_as<muxc::Grid>();
        grid && grid.Name() == L"NavigationBarControlGrid") {
        return grid;
    }
    int count = muxm::VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < count; ++i) {
        if (auto grid = FindNavigationGrid(
                muxm::VisualTreeHelper::GetChild(root, i), depth + 1)) {
            return grid;
        }
    }
    return nullptr;
}

void RemoveBarVisuals(BarState& state) {
    RevokeHandlers(state.recentMenuHandlers);
    RevokeHandlers(state.driveHandlers);
    RevokeHandlers(state.panelHandlers);
    auto root = state.barRoot.get();
    if (root) {
        root.PointerEntered(state.rootPointerToken);
        root.SizeChanged(state.rootSizeToken);
        root.Loaded(state.rootLoadedToken);
        root.DragOver(state.rootDragOverToken);
        root.DragLeave(state.rootDragLeaveToken);
        root.Drop(state.rootDropToken);
    }
    if (auto grid = state.grid.get()) {
        if (root) {
            auto children = grid.Children();
            for (unsigned i = 0; i < children.Size(); ++i) {
                if (children.GetAt(i) == root) {
                    children.RemoveAt(i);
                    break;
                }
            }
        }
        auto rows = grid.RowDefinitions();
        if (state.addedRow && rows.Size() &&
            rows.GetAt(rows.Size() - 1) == state.addedRow) {
            rows.RemoveAt(rows.Size() - 1);
        }
        if (state.createdFirstRow && rows.Size() == 1) {
            rows.RemoveAt(0);
        }
        if (grid.MinHeight() == state.appliedGridMinHeight) {
            grid.MinHeight(state.oldGridMinHeight);
        }
        grid.InvalidateMeasure();
    }
    if (auto nav = state.navControl.get()) {
        if (nav.MinHeight() == state.appliedNavMinHeight) {
            nav.MinHeight(state.oldNavMinHeight);
        }
        nav.InvalidateMeasure();
    }
    if (auto host = state.hostGrid.get()) {
        auto rows = host.RowDefinitions();
        if (state.commandRow && rows.Size() == 3 &&
            rows.GetAt(2) == state.commandRow &&
            state.commandRow.Height().GridUnitType ==
                mux::GridUnitType::Auto) {
            state.commandRow.Height(state.oldCommandRowHeight);
        }
        host.InvalidateMeasure();
    }
    state.grid = nullptr;
    state.hostGrid = nullptr;
    state.navControl = nullptr;
    state.barRoot = nullptr;
    state.strip = nullptr;
    state.buttons = nullptr;
    state.recents = nullptr;
    state.rootPointerToken = {};
    state.rootSizeToken = {};
    state.rootLoadedToken = {};
    state.rootDragOverToken = {};
    state.rootDragLeaveToken = {};
    state.rootDropToken = {};
    state.recentFolders.clear();
    state.recentButtonCount = 0;
    state.hasRecentMenuButton = false;
    state.visibleRecents = 0;
    state.addedRow = nullptr;
    state.commandRow = nullptr;
    state.createdFirstRow = false;
    state.originalGridHeight = 0;
    state.originalNavHeight = 0;
    state.rowCount = 1;
    state.lastLayoutWidth = 0;
    state.reflowing = false;
    state.renderedKey.clear();
    state.lastFolderCheckAt = 0;
    ClearDrag(state);
    state.suppressClick.clear();
    if (!g_unloading) {
        UpdateFrameRowCount();
    }
}

void TryInstallBar(const muxc::CommandBar& commandBar);

void TryInstallBar(const muxc::CommandBar& commandBar) {
    BarState* state = nullptr;
    try {
    if (g_unloading || !g_frameHooked) {
        return;
    }
    auto root = commandBar.XamlRoot();
    if (!root || !root.Content()) {
        return;
    }
    auto grid = FindNavigationGrid(root.Content(), 0);
    if (!grid) {
        return;
    }

    for (auto& item : g_bars) {
        if (item.commandBar.get() == commandBar) {
            state = &item;
            break;
        }
    }
    if (!state || (state->strip.get() && state->grid.get() == grid)) {
        return;
    }
    // Another tab's command bar may already own the row in this grid.
    for (const auto& child : grid.Children()) {
        auto element = child.try_as<mux::FrameworkElement>();
        if (element && element.Name() == kBarName) {
            return;
        }
    }
    if (state->addedRow || state->strip.get()) {
        // Explorer rebuilt its navigation bar; undo the stale installation.
        RemoveBarVisuals(*state);
    }

    auto nav = muxm::VisualTreeHelper::GetParent(grid)
                   .try_as<mux::FrameworkElement>();
    auto host = nav ? muxm::VisualTreeHelper::GetParent(nav)
                          .try_as<muxc::Grid>()
                    : nullptr;
    if (!host) {
        return;
    }
    auto hostRows = host.RowDefinitions();
    // The measured Explorer header has [Auto, *, *] with navigation in row 1.
    // Leave unknown Windows builds untouched instead of altering another Grid.
    if (hostRows.Size() != 3 || muxc::Grid::GetRow(nav) != 1 ||
        hostRows.GetAt(0).Height().GridUnitType != mux::GridUnitType::Auto ||
        hostRows.GetAt(1).Height().GridUnitType != mux::GridUnitType::Star ||
        hostRows.GetAt(2).Height().GridUnitType != mux::GridUnitType::Star) {
        return;
    }
    const double originalGridHeight =
        std::max<double>(grid.ActualHeight(), grid.DesiredSize().Height);
    const double originalNavHeight =
        std::max<double>(nav.ActualHeight(), nav.DesiredSize().Height);
    muxc::RowDefinition row;
    row.Height(mux::GridLength{kRowHeight, mux::GridUnitType::Pixel});
    auto rows = grid.RowDefinitions();
    state->grid = winrt::make_weak(grid);
    if (rows.Size() == 0) {
        muxc::RowDefinition originalRow;
        rows.Append(originalRow);
        state->createdFirstRow = true;
    }
    unsigned rowIndex = rows.Size();
    state->hostGrid = winrt::make_weak(host);
    state->addedRow = row;
    rows.Append(row);

    // Otherwise the two equal star rows each consume half of the extra host
    // height. Auto keeps Explorer's command row at its natural 48 units.
    state->commandRow = hostRows.GetAt(2);
    state->oldCommandRowHeight = state->commandRow.Height();
    state->commandRow.Height(
        mux::GridLength{1.0, mux::GridUnitType::Auto});

    muxc::StackPanel buttons;
    buttons.Orientation(muxc::Orientation::Vertical);
    muxc::ScrollViewer strip;
    strip.Height(kRowHeight);
    // A visible scrollbar would consume the fixed row height and clip the
    // buttons. Horizontal panning remains available when row four overflows.
    strip.HorizontalScrollBarVisibility(muxc::ScrollBarVisibility::Hidden);
    strip.VerticalScrollBarVisibility(muxc::ScrollBarVisibility::Disabled);
    strip.HorizontalScrollMode(muxc::ScrollMode::Enabled);
    strip.VerticalScrollMode(muxc::ScrollMode::Disabled);
    strip.Content(buttons);
    // Recents stay on the first row, outside the scrolling bookmark strip.
    muxc::StackPanel recents;
    recents.Orientation(muxc::Orientation::Horizontal);
    recents.Height(kRowHeight);
    recents.VerticalAlignment(mux::VerticalAlignment::Top);

    muxc::Grid barRoot;
    barRoot.Name(kBarName);
    barRoot.Height(kRowHeight);
    // A transparent background makes empty parts of the bar a drop target.
    barRoot.Background(muxm::SolidColorBrush(
        winrt::Windows::UI::Color{0, 0, 0, 0}));
    barRoot.AllowDrop(true);
    muxc::ColumnDefinition stripColumn;
    muxc::ColumnDefinition recentsColumn;
    recentsColumn.Width(mux::GridLength{1.0, mux::GridUnitType::Auto});
    barRoot.ColumnDefinitions().Append(stripColumn);
    barRoot.ColumnDefinitions().Append(recentsColumn);
    muxc::Grid::SetColumn(recents, 1);
    barRoot.Children().Append(strip);
    barRoot.Children().Append(recents);
    // Move the scroll and hit-test surface without changing row allocation.
    muxm::TranslateTransform barOffset;
    barOffset.Y(-kRowOpticalLift);
    barRoot.RenderTransform(barOffset);
    muxc::Grid::SetRow(barRoot, static_cast<int>(rowIndex));
    muxc::Grid::SetColumnSpan(
        barRoot,
        static_cast<int>(std::max(1u, grid.ColumnDefinitions().Size())));

    state->navControl = winrt::make_weak(nav);
    state->barRoot = winrt::make_weak(barRoot);
    state->strip = winrt::make_weak(strip);
    state->buttons = winrt::make_weak(buttons);
    state->recents = winrt::make_weak(recents);
    state->oldGridMinHeight = grid.MinHeight();
    state->originalGridHeight = originalGridHeight;
    state->originalNavHeight = originalNavHeight;
    state->appliedGridMinHeight =
        std::max(state->oldGridMinHeight, originalGridHeight + kRowHeight);
    grid.MinHeight(state->appliedGridMinHeight);
    if (nav) {
        state->oldNavMinHeight = nav.MinHeight();
        state->appliedNavMinHeight =
            std::max(state->oldNavMinHeight, originalNavHeight + kRowHeight);
        nav.MinHeight(state->appliedNavMinHeight);
    }

    grid.Children().Append(barRoot);
    grid.InvalidateMeasure();
    host.InvalidateMeasure();
    auto weakButtons = winrt::make_weak(buttons);
    state->rootPointerToken =
        barRoot.PointerEntered([weakButtons](auto const&, auto const&) {
            auto panel = weakButtons.get();
            if (!panel) {
                return;
            }
            RefreshPanel(panel);
        });
    // The full bar width decides both the recents and the bookmark wrapping.
    state->rootSizeToken = barRoot.SizeChanged(
        [weakButtons](auto const&, const mux::SizeChangedEventArgs& args) {
            if (auto panel = weakButtons.get()) {
                ReflowPanel(panel, args.NewSize().Width);
            }
        });
    state->rootLoadedToken = barRoot.Loaded(
        [weakButtons](auto const&, auto const&) {
            if (auto panel = weakButtons.get()) {
                if (auto bar = FindState(panel)) {
                    if (auto root = bar->barRoot.get()) {
                        ReflowPanel(panel, root.ActualWidth());
                    }
                }
            }
        });
    state->rootDragOverToken = barRoot.DragOver(
        [weakButtons](auto const&, const mux::DragEventArgs& args) {
            if (auto panel = weakButtons.get()) {
                OnBarDragOver(panel, args);
            }
        });
    state->rootDragLeaveToken = barRoot.DragLeave(
        [weakButtons](auto const&, auto const&) {
            if (auto panel = weakButtons.get()) {
                if (auto bar = FindState(panel)) {
                    ClearDropTarget(*bar);
                }
            }
        });
    state->rootDropToken = barRoot.Drop(
        [weakButtons](auto const&, const mux::DragEventArgs& args) {
            if (auto panel = weakButtons.get()) {
                OnBarDrop(panel, args);
            }
        });
    RefreshPanel(buttons);
    UpdateFrameRowCount();
    } catch (...) {
        Wh_Log(L"Bookmarks bar insertion failed: %08X",
               winrt::to_hresult().value);
        if (state) {
            try {
                RemoveBarVisuals(*state);
            } catch (...) {
                Wh_Log(L"Bookmarks bar rollback failed: %08X",
                       winrt::to_hresult().value);
            }
        }
    }
}

void TrackCommandBar(const muxc::CommandBar& commandBar) try {
    if (g_unloading || !commandBar ||
        commandBar.Name() != L"FileExplorerCommandBar") {
        return;
    }
    for (const auto& state : g_bars) {
        if (state.commandBar.get() == commandBar) {
            TryInstallBar(commandBar);
            return;
        }
    }
    // Tabs come and go; forget bars whose command bar and strip are gone.
    g_bars.remove_if([](BarState& state) {
        if (state.commandBar.get() || state.strip.get()) {
            return false;
        }
        RevokeHandlers(state.recentMenuHandlers);
        RevokeHandlers(state.driveHandlers);
        RevokeHandlers(state.panelHandlers);
        return true;
    });
    UpdateFrameRowCount();
    g_bars.emplace_back();
    BarState& state = g_bars.back();
    state.commandBar = winrt::make_weak(commandBar);
    auto weakBar = state.commandBar;
    state.loadedToken = commandBar.Loaded([weakBar](auto const&, auto const&) {
        if (auto bar = weakBar.get()) {
            TryInstallBar(bar);
        }
    });
    state.unloadedToken = commandBar.Unloaded(
        [weakBar](auto const&, auto const&) {
            auto bar = weakBar.get();
            // WinUI can raise Unloaded after a quick re-add has already raised
            // Loaded; keep the bar while its command bar is in the tree.
            if (!bar || bar.IsLoaded()) {
                return;
            }
            for (auto& state : g_bars) {
                if (state.commandBar.get() == bar) {
                    // Another tab's command bar can share a header that stays
                    // on screen; keep the bar there instead of removing it.
                    if (auto strip = state.strip.get();
                        strip && strip.IsLoaded()) {
                        break;
                    }
                    try {
                        RemoveBarVisuals(state);
                    } catch (...) {
                        Wh_Log(L"Bookmarks bar unload cleanup failed: %08X",
                               winrt::to_hresult().value);
                    }
                    break;
                }
            }
            if (std::none_of(g_bars.begin(), g_bars.end(),
                             [](const BarState& state) {
                                 return !!state.strip.get();
                             })) {
                g_iconCache.clear();
            }
        });
    TryInstallBar(commandBar);
    InstallMessageHook();
    // A new command bar usually means a new tab to follow for recents.
    ScheduleRecentWork(true, false);
} catch (...) {
    Wh_Log(L"Bookmarks bar tracking failed: %08X",
           winrt::to_hresult().value);
}

void CollectCommandBars(const mux::DependencyObject& element, int depth,
                        std::vector<muxc::CommandBar>& bars) {
    if (!element || depth > 64) {
        return;
    }
    if (auto bar = element.try_as<muxc::CommandBar>();
        bar && bar.Name() == L"FileExplorerCommandBar") {
        bars.push_back(bar);
        return;
    }
    int count = muxm::VisualTreeHelper::GetChildrenCount(element);
    for (int i = 0; i < count; ++i) {
        CollectCommandBars(muxm::VisualTreeHelper::GetChild(element, i),
                           depth + 1, bars);
    }
}

void ScanXamlRootForCommandBars(const mux::UIElement& element) {
    if (!element) {
        return;
    }
    auto root = element.XamlRoot();
    if (!root || !root.Content()) {
        return;
    }
    std::vector<muxc::CommandBar> bars;
    CollectCommandBars(root.Content(), 0, bars);
    for (const auto& bar : bars) {
        TrackCommandBar(bar);
    }
}

void ScanCurrentThreadForCommandBars() try {
    if (g_unloading || !g_frameHooked) {
        return;
    }
    std::vector<winrt::weak_ref<muxc::CommandBar>> knownBars;
    for (const auto& state : g_bars) {
        knownBars.push_back(state.commandBar);
    }
    for (const auto& weakBar : knownBars) {
        if (auto bar = weakBar.get()) {
            ScanXamlRootForCommandBars(bar);
        }
    }
    auto focused = muxi::FocusManager::GetFocusedElement();
    if (auto element = focused ? focused.try_as<mux::UIElement>() : nullptr) {
        ScanXamlRootForCommandBars(element);
    }
} catch (...) {
    Wh_Log(L"Bookmarks bar scan failed: %08X",
           winrt::to_hresult().value);
}

// CommandBarManager receives an actual typed WinUI CommandBar parameter. Its
// implementation object is intentionally never cast to a XAML object.
using CommandBarSetter = void(WINAPI*)(void*, void*);
CommandBarSetter g_commandBarSetterOriginal = nullptr;
void WINAPI CommandBarSetterHook(void* self, void* value) {
    g_commandBarSetterOriginal(self, value);
    if (!g_unloading && value) {
        TrackCommandBar(*reinterpret_cast<muxc::CommandBar*>(value));
    }
}

// The stock header rows are 38/48/48 at 96 DPI. The navigation
// Grid overhangs its row by 3 units at both ends. For N wrapped rows, reserve
// 38*N + 2*(N-1) + 6 = 40*N + 4 units. The original getter returns a fresh
// physical-pixel size on each call; the ratio scales with DPI.
using DesiredSizeGetter = HRESULT(WINAPI*)(void*, SIZE*);
DesiredSizeGetter g_desiredSizeOriginal = nullptr;
HRESULT WINAPI DesiredSizeHook(void* self, SIZE* size) {
    HRESULT result = g_desiredSizeOriginal(self, size);
    if (FAILED(result) || !size || g_unloading || g_frameRows == 0) {
        return result;
    }
    const LONG originalHeight = size->cy;
    const bool eligible = originalHeight >= 80 && originalHeight <= 600;
    const unsigned frameRows = std::min(g_frameRows, kMaxRows);
    const LONG extra = eligible
                           ? MulDiv(originalHeight, HostExtraAt96Dpi(frameRows),
                                    kMeasuredHostHeightAt96Dpi)
                           : 0;
    size->cy += extra;
    return result;
}

bool HookExplorerFrame(bool apply) {
    if (g_frameHooked) {
        return true;
    }
    HMODULE module = GetModuleHandleW(L"Windows.UI.FileExplorer.dll");
    if (!module) {
        return true;
    }
    // Bound symbol resolution to one attempt for this Explorer process;
    // repeated failures would invalidate Windhawk's symbol cache.
    bool expected = false;
    if (!g_frameHookAttempted.compare_exchange_strong(expected, true)) {
        return g_frameHooked;
    }
    // Windows.UI.FileExplorer.dll
    WindhawkUtils::SYMBOL_HOOK hook[] = {{
        {LR"(public: virtual long __cdecl XamlIslandViewAdapter::get_DesiredSizeInPhysicalPixels(struct tagSIZE *))"},
        &g_desiredSizeOriginal, DesiredSizeHook}};
    if (!WindhawkUtils::HookSymbols(module, hook, ARRAYSIZE(hook)) ||
        !g_desiredSizeOriginal) {
        Wh_Log(L"File Explorer frame size symbol unavailable");
        return false;
    }
    g_frameHooked = true;
    if (apply) {
        Wh_ApplyHookOperations();
    }
    return true;
}

bool HookExplorerExtension(bool apply) {
    if (g_extensionHooked) {
        return true;
    }
    HMODULE module = GetModuleHandleW(L"FileExplorerExtensions.dll");
    if (!module) {
        return true;
    }
    // Bound symbol resolution to one attempt for this Explorer process.
    bool expected = false;
    if (!g_extensionHookAttempted.compare_exchange_strong(expected, true)) {
        return g_extensionHooked;
    }
    // FileExplorerExtensions.dll
    WindhawkUtils::SYMBOL_HOOK hook[] = {{
        {LR"(public: void __cdecl winrt::FileExplorerExtensions::implementation::CommandBarManager::CommandBar(struct winrt::Microsoft::UI::Xaml::Controls::CommandBar const &))",
         LR"(public: void __cdecl winrt::FileExplorerExtensions::implementation::CommandBarManager::CommandBar(struct winrt::Microsoft::UI::Xaml::Controls::CommandBar const & __ptr64) __ptr64)"},
        &g_commandBarSetterOriginal, CommandBarSetterHook}};
    if (!WindhawkUtils::HookSymbols(module, hook, ARRAYSIZE(hook)) ||
        !g_commandBarSetterOriginal) {
        Wh_Log(L"File Explorer command bar symbol unavailable");
        return false;
    }
    g_extensionHooked = true;
    if (apply) {
        Wh_ApplyHookOperations();
    }
    return true;
}

using LoadLibraryExW_t = decltype(&LoadLibraryExW);
LoadLibraryExW_t g_loadLibraryOriginal = nullptr;
HMODULE WINAPI LoadLibraryExWHook(LPCWSTR file, HANDLE handle, DWORD flags) {
    HMODULE module = g_loadLibraryOriginal(file, handle, flags);
    if (module && file && !g_unloading &&
        (!g_extensionHookAttempted || !g_frameHookAttempted)) {
        const wchar_t* base = file;
        for (const wchar_t* p = file; *p; ++p) {
            if (*p == L'\\' || *p == L'/') {
                base = p + 1;
            }
        }
        if (_wcsicmp(base, L"Windows.UI.FileExplorer.dll") == 0) {
            HookExplorerFrame(true);
        } else if (_wcsicmp(base, L"FileExplorerExtensions.dll") == 0) {
            HookExplorerExtension(true);
        }
    }
    return module;
}

void ForExplorerWindows(void (*callback)());

void CleanupCurrentThread() {
    RemoveMessageHook();
    StopThreadWork();
    try {
        StopRecentTracking();
    } catch (...) {
        Wh_Log(L"Recent folder tracking cleanup failed: %08X",
               winrt::to_hresult().value);
    }
    bool hadBars = !g_bars.empty();
    for (auto& state : g_bars) {
        try {
            if (auto commandBar = state.commandBar.get()) {
                commandBar.Loaded(state.loadedToken);
                commandBar.Unloaded(state.unloadedToken);
            }
            RemoveBarVisuals(state);
        } catch (...) {
            Wh_Log(L"Bookmarks bar cleanup failed: %08X",
                   winrt::to_hresult().value);
        }
    }
    g_bars.clear();
    g_iconCache.clear();
    if (hadBars) {
        // The size hook is inactive now, so Explorer returns to its stock
        // header height instead of leaving an empty band until a resize.
        RelayoutThreadFrames();
    }
}

struct ThreadCall {
    void (*callback)();
};

UINT ThreadCallMessage() {
    static const UINT message =
        RegisterWindowMessageW(L"Windhawk_ExplorerFolderBookmarksBar_ThreadCall");
    return message;
}

LRESULT CALLBACK ThreadCallHook(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        auto cwp = reinterpret_cast<const CWPSTRUCT*>(lParam);
        if (cwp->message == ThreadCallMessage()) {
            auto call = reinterpret_cast<ThreadCall*>(cwp->lParam);
            call->callback();
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

void RunOnWindowThread(HWND window, void (*callback)()) {
    DWORD threadId = GetWindowThreadProcessId(window, nullptr);
    if (!threadId) {
        return;
    }
    if (threadId == GetCurrentThreadId()) {
        callback();
        return;
    }
    HHOOK hook = SetWindowsHookExW(WH_CALLWNDPROC, ThreadCallHook, nullptr,
                                   threadId);
    if (!hook) {
        return;
    }
    ThreadCall call{callback};
    SendMessageW(window, ThreadCallMessage(), 0,
                 reinterpret_cast<LPARAM>(&call));
    UnhookWindowsHookEx(hook);
}

void ForExplorerWindows(void (*callback)()) {
    EnumWindows(
        [](HWND window, LPARAM param) -> BOOL {
            DWORD processId = 0;
            GetWindowThreadProcessId(window, &processId);
            if (processId != GetCurrentProcessId()) {
                return TRUE;
            }
            wchar_t className[64]{};
            if (GetClassNameW(window, className, ARRAYSIZE(className)) &&
                wcscmp(className, L"CabinetWClass") == 0) {
                RunOnWindowThread(
                    window, reinterpret_cast<void (*)()>(param));
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(callback));
}

void BeginUnloading() {
    std::lock_guard lock(g_operationMutex);
    g_unloading = true;
}

void CloseActiveDialogCurrentThread() {
    if (g_threadFileDialog) {
        g_threadFileDialog->Close(HRESULT_FROM_WIN32(ERROR_CANCELLED));
    }
}

// File dialogs in any process. Each new Open or Save dialog receives the
// recent folders as navigation-pane places before the program shows it.
bool g_isExplorer = false;

bool IsExplorerProcess() {
    wchar_t path[MAX_PATH]{};
    DWORD length = GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
    if (length == 0 || length == ARRAYSIZE(path)) {
        return false;
    }
    const wchar_t* base = wcsrchr(path, L'\\');
    return _wcsicmp(base ? base + 1 : path, L"explorer.exe") == 0;
}

winrt::com_ptr<IShellItem> FolderShellItem(const std::wstring& path) {
    winrt::com_ptr<IShellItem> item;
    if (CheckFolderStatus(path) == FolderStatus::Available &&
        SUCCEEDED(SHCreateItemFromParsingName(path.c_str(), nullptr,
                                              IID_PPV_ARGS(item.put())))) {
        return item;
    }
    // A simple ID list needs no I/O, so network and removable folders never
    // delay the dialog.
    item = nullptr;
    PIDLIST_ABSOLUTE pidl = SHSimpleIDListFromPath(path.c_str());
    if (pidl) {
        SHCreateItemFromIDList(pidl, IID_PPV_ARGS(item.put()));
        CoTaskMemFree(pidl);
    }
    return item;
}

void AddRecentPlaces(IUnknown* object) try {
    winrt::com_ptr<IFileDialog> dialog;
    if (FAILED(object->QueryInterface(IID_PPV_ARGS(dialog.put())))) {
        return;
    }
    std::vector<std::wstring> bookmarks;
    {
        std::lock_guard lock(g_storageMutex);
        bookmarks = SplitBookmarks(ReadStorageLocked());
    }
    int added = 0;
    for (const auto& path : LoadRecentView(bookmarks).folders) {
        if (auto item = FolderShellItem(path)) {
            if (SUCCEEDED(dialog->AddPlace(item.get(), FDAP_TOP))) {
                ++added;
            }
        }
    }
    Wh_Log(L"File dialog: added %d recent place(s)", added);
} catch (...) {
    Wh_Log(L"Could not add recent folders to a file dialog: %08X",
           winrt::to_hresult().value);
}

bool IsFileDialogClass(REFCLSID clsid) {
    return IsEqualCLSID(clsid, CLSID_FileOpenDialog) ||
           IsEqualCLSID(clsid, CLSID_FileSaveDialog);
}

// CoCreateInstance may be implemented through CoCreateInstanceEx; the guard
// keeps one creation from being handled twice.
thread_local bool g_creatingFileDialog = false;

using CoCreateInstance_t = decltype(&CoCreateInstance);
CoCreateInstance_t g_coCreateInstanceOriginal = nullptr;
HRESULT WINAPI CoCreateInstanceHook(REFCLSID clsid, LPUNKNOWN outer,
                                    DWORD context, REFIID iid,
                                    LPVOID* object) {
    if (g_creatingFileDialog || !IsFileDialogClass(clsid)) {
        return g_coCreateInstanceOriginal(clsid, outer, context, iid, object);
    }
    OperationScope operation;
    g_creatingFileDialog = true;
    HRESULT result =
        g_coCreateInstanceOriginal(clsid, outer, context, iid, object);
    g_creatingFileDialog = false;
    if (operation.active && SUCCEEDED(result) && !outer && object &&
        *object) {
        AddRecentPlaces(static_cast<IUnknown*>(*object));
    }
    return result;
}

using CoCreateInstanceEx_t = decltype(&CoCreateInstanceEx);
CoCreateInstanceEx_t g_coCreateInstanceExOriginal = nullptr;
HRESULT WINAPI CoCreateInstanceExHook(REFCLSID clsid, IUnknown* outer,
                                      DWORD context, COSERVERINFO* server,
                                      DWORD count, MULTI_QI* results) {
    if (g_creatingFileDialog || !IsFileDialogClass(clsid)) {
        return g_coCreateInstanceExOriginal(clsid, outer, context, server,
                                            count, results);
    }
    OperationScope operation;
    g_creatingFileDialog = true;
    HRESULT result = g_coCreateInstanceExOriginal(clsid, outer, context,
                                                  server, count, results);
    g_creatingFileDialog = false;
    if (!operation.active || FAILED(result) || outer || !results) {
        return result;
    }
    for (DWORD i = 0; i < count; ++i) {
        if (SUCCEEDED(results[i].hr) && results[i].pItf) {
            AddRecentPlaces(results[i].pItf);
            break;
        }
    }
    return result;
}

bool HookFileDialogCreation() {
    HMODULE combase = GetModuleHandleW(L"combase.dll");
    if (!combase) {
        combase = LoadLibraryExW(L"combase.dll", nullptr,
                                 LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
    auto coCreateInstance = combase
                                ? reinterpret_cast<CoCreateInstance_t>(
                                      GetProcAddress(combase, "CoCreateInstance"))
                                : nullptr;
    auto coCreateInstanceEx =
        combase ? reinterpret_cast<CoCreateInstanceEx_t>(
                      GetProcAddress(combase, "CoCreateInstanceEx"))
                : nullptr;
    return coCreateInstance && coCreateInstanceEx &&
           WindhawkUtils::SetFunctionHook(coCreateInstance,
                                          CoCreateInstanceHook,
                                          &g_coCreateInstanceOriginal) &&
           WindhawkUtils::SetFunctionHook(coCreateInstanceEx,
                                          CoCreateInstanceExHook,
                                          &g_coCreateInstanceExOriginal);
}

BOOL Wh_ModInit() {
    LoadRecentSettings();
    g_bookmarkHotkey = Wh_GetIntSetting(L"bookmarkHotkey") != 0;
    g_isExplorer = IsExplorerProcess();
    if (g_recentSettings.enabled &&
        Wh_GetIntSetting(L"recentInFileDialogs") != 0 &&
        !HookFileDialogCreation()) {
        Wh_Log(L"Could not hook file dialog creation");
    }
    if (!g_isExplorer) {
        return TRUE;
    }
    HMODULE kernelBase = GetModuleHandleW(L"kernelbase.dll");
    auto loadLibraryExW = kernelBase
                              ? reinterpret_cast<LoadLibraryExW_t>(
                                    GetProcAddress(kernelBase, "LoadLibraryExW"))
                              : nullptr;
    if (!loadLibraryExW ||
        !WindhawkUtils::SetFunctionHook(loadLibraryExW, LoadLibraryExWHook,
                                       &g_loadLibraryOriginal)) {
        Wh_Log(L"Could not hook kernelbase LoadLibraryExW");
        return FALSE;
    }
    // Ensure the frame symbol can be hooked before Explorer creates a window.
    if (!GetModuleHandleW(L"Windows.UI.FileExplorer.dll")) {
        LoadLibraryExW(L"Windows.UI.FileExplorer.dll", nullptr,
                       LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
    HookExplorerFrame(false);
    HookExplorerExtension(false);
    return TRUE;
}

void Wh_ModAfterInit() {
    if (!g_isExplorer) {
        return;
    }
    HookExplorerFrame(true);
    HookExplorerExtension(true);
    if (g_frameHooked && g_extensionHooked) {
        ForExplorerWindows(ScanCurrentThreadForCommandBars);
    }
}

void Wh_ModBeforeUninit() {
    BeginUnloading();
}

void Wh_ModUninit() {
    BeginUnloading();
    for (;;) {
        ForExplorerWindows(CloseActiveDialogCurrentThread);
        std::unique_lock lock(g_operationMutex);
        if (g_activeOperations == 0) {
            break;
        }
        g_operationFinished.wait_for(lock, std::chrono::milliseconds(100));
    }
    for (int attempt = 0; attempt < 50 && g_pendingDrops != 0; ++attempt) {
        Sleep(100);
    }
    ForExplorerWindows(CleanupCurrentThread);
}
