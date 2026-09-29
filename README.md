# Explorer Folder Bookmarks Bar

A [Windhawk](https://windhawk.net/) mod for Windows 11 File Explorer on x64 and ARM64. It puts folder bookmarks below the address bar, supports up to 32 folders, adapts from one to four rows, and scrolls horizontally when needed.

[![Hits](https://hits.sh/github.com/MaxITService/EXPLORER-bookmarks-bar-windhawk.svg?style=flat)](https://hits.sh/github.com/MaxITService/EXPLORER-bookmarks-bar-windhawk/)

<p align="center">
  <img src="Promo/How-it-works.gif" alt="How it works">
</p>

## Choose a version

| Version | Source | Status |
| --- | --- | --- |
| **0.7.8** | [Root source](explorer-folder-bookmarks-bar.wh.cpp) · [download source](https://github.com/MaxITService/EXPLORER-bookmarks-bar-windhawk/raw/main/explorer-folder-bookmarks-bar.wh.cpp) | Tested in File Explorer. [Windhawk PR #5755](https://github.com/ramensoftware/windhawk-mods/pull/5755) was merged on September 29, 2026. Catalog search may take time to reflect the merge. |
| **0.8.10** | [Development source](mods/explorer-folder-bookmarks-bar.wh.cpp) · [download source](https://github.com/MaxITService/EXPLORER-bookmarks-bar-windhawk/raw/main/mods/explorer-folder-bookmarks-bar.wh.cpp) | Source only. Adds recent folders and other features described below. Passes local compile checks but has **not** been tested in File Explorer. It is not the version submitted to the Windhawk catalog. |

The root file is the exact 0.7.8 source from the tested release. The newer source is kept separately so it does not silently replace that baseline.

## Install

**From the Windhawk catalog:** Search Windhawk for **Explorer Folder Bookmarks Bar**, select the mod, and click **Install**. This installs the catalog version, **0.7.8**. If search does not find it yet, use the source method below while catalog publication catches up with the merged PR.

**From source, available now:** Download either source linked above from GitHub. In Windhawk, enable developer mode if **Create a new mod** is hidden, then create a mod or edit your existing `explorer-folder-bookmarks-bar` mod. Replace the entire `mod.wh.cpp` editor content with the chosen source, click **Compile Mod**, and enable the mod. The newer 0.8.10 version is available only by this source route; compilation may take a while. Both files use the same mod ID, so only one version can be active at a time.

Open a new File Explorer window after installing, enabling, or updating the mod. Windows already open may remain unchanged.

## Use: 0.7.8 and newer

- Click **+** to bookmark the current filesystem folder. Click a bookmark to navigate there; Ctrl+click opens it in a new tab. Drag bookmarks to reorder them. Middle-click removes a bookmark from the bar without deleting its folder. Hover to see its full path. Icons come from Windows Shell.
- Left-click **FX** for your profile folder, Desktop, Documents, Downloads, and custom folders. Right-click **FX** for all drives. Ctrl+click a menu item opens it in a new tab.
- Set **FX custom folders** in Windhawk settings. Enter an absolute path and optional label; blank entries are ignored. Environment variables are supported. A folder missing from a local fixed drive is hidden; network and removable entries remain visible, and Explorer reports an unavailable target when clicked. Open a new window after changing settings.
- Right-click **+** to save or load a bookmark backup using a file dialog. Bookmarks also persist in Windhawk's local mod storage.

## Additional features in source-only 0.8.10

These features have not yet been tested in File Explorer.

- Press **Ctrl+B** to toggle a bookmark for the current folder. If folders are selected in the file list, it bookmarks the selected folders or removes them when all are bookmarked already. Drag folders onto the bar to add them. One shortcut or drop handles at most 20 folders. Ctrl+B is ignored while typing in the address bar, search box, or a rename field and can be turned off in settings.
- Right-click a bookmark to open it in a new Explorer window. A short notice appears if folders cannot all be added.
- Turn on **Recent folders → Show recent folders** for recent-folder buttons and an **RC** menu at the right end of the bar. The group controls the number of buttons, remembered folders, and RC behavior. RC switches between folders opened in File Explorer and Windows recent items. Windows items are read only when the Recent folder is on a local fixed drive. Middle-click a recent folder to remove it; drag it onto the bookmarks to keep it. Explorer folder history stays in local mod storage until cleared, even if the setting is turned off.

## Other source variants

The [Double Decker source](explorer-folder-bookmarks-bar-double.wh.cpp) has two independent, horizontally scrollable bookmark rows. It is frozen at 0.7.8 and has not been tested in File Explorer. The [personal build](explorer-folder-bookmarks-bar-personal.wh.cpp) adds optional recent folders to Open and Save dialogs in all programs. It is frozen at 0.8.5, untested, loads into every process, and is not intended for the Windhawk catalog. All variants use the same mod ID; compile one at a time.

Licensed under [MIT](LICENSE).

## Other projects by MaxITService

- [AivoRelay](https://github.com/MaxITService/AIVORelay) — voice to text for Windows.
- [OneClickPrompts](https://github.com/MaxITService/OneClickPrompts) — prompt shortcuts for AI chats.
- [Console2Ai](https://github.com/MaxITService/Console2Ai) — send a PowerShell buffer to AI.
- [Ping-Plotter-PS51](https://github.com/MaxITService/Ping-Plotter-PS51) — a PowerShell ping plotter.
