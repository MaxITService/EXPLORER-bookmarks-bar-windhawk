# Decision log

## Current checkout (2026-09-27)

The user reports that the installed 0.7.1 bar now works and requested only two
mod sources. Keep `mods/explorer-folder-bookmarks-bar.wh.cpp` as the production
source based on that build and `explorer-folder-bookmarks-bar-double.wh.cpp` as
the experimental Double Decker. The latter retains two separate bookmark lists
and measured two-row spacing while adopting production's event-driven startup hooks,
editable FX shortcuts, blank shortcut defaults, checks that hide missing FX
folders, and hidden horizontal scrollbars. Its live Explorer behavior remains
**untested**; local x64 and ARM64 syntax and link checks alone do not prove the layout.
The production source at `b639b81` was tested in Windhawk: newly opened
Explorer windows show the bar, but windows already open when the mod is enabled
do not. A later local candidate crashed on opening windows because
`WindhawkUtils::StringSetting` was constructed with a setting name instead of
the allocated setting value. The repository was restored to `b639b81` before
developing the 0.7.2 candidate. Version 0.7.2 has not been installed or tested
in Explorer. Opening a new window is the supported activation path. Older source
variants are not included in this repository's new one-commit history.

## Confirmed baselines

- **0.4.1 (2026-09-26):** The user confirmed the single-row bar in Explorer after the early insertion fix for 0.4.0 clipping.
- **0.4.7 (2026-09-27):** The user called its single-row appearance perfect. Its spacing informs the current layouts.

## Layout decisions

- **0.4.4–0.4.7:** Measurements found Explorer's `[Auto, *, *]` header gave half the added height to the command row and clipped the bookmark strip. Version 0.4.5 changed that row to `Auto` and reserved 44 host units at 96 DPI; 0.4.6 centered 32-unit buttons with 3-unit top/bottom margins; 0.4.7 lifted the ScrollViewer by 3 units to balance visible gaps. Exact measurements are in [the layout record](diagnostics/layout.md).
- **Two-row layout:** Double Decker uses 38/40-unit rows, a 2-unit lower inset, 84 host units at 96 DPI, and an `Auto` command row. Its lower row has separate `folders2` storage and backup JSON, with no FX.
- **Adaptive layout:** The production source wraps one bookmark list into 1–4 rows and reserves `40*N + 4` host units at 96 DPI; overflow after row four pans horizontally. The user verified three-row geometry and accepted an intermittent background shade difference.

## Production

- **0.7.0:** The historical production source brought adaptive rows to `mods/explorer-folder-bookmarks-bar.wh.cpp`, kept fixed C:, D:, E:, F:, G:, Q: drive shortcuts, made extra FX folders editable, and removed layout diagnostics. Default custom entries were `Q:\code` and `C:\PS`. The installed bar sometimes stayed missing after boot, even in a new window; toggling the mod restored it.
- **0.7.1:** The repository source hooks `kernelbase.dll` to observe Explorer DLL loads and does not run the old retry thread. It has a blank FX template instead of personal folder paths; blank rows are ignored. Saved settings remain, but paths that do not currently resolve to directories are hidden from FX and can reappear on a later bar refresh. The user confirmed that disabling this build without recompiling did not crash Explorer; newly opened windows received the bar, while already-open windows did not.
- **0.7.2 candidate:** Filters UNC paths and mapped network drives out of both FX menus, while preserving settings. The production source also closes active Save/Load dialogs before unload and waits for their handlers; uses weak references for cached bitmaps, caches plain icon pixels, removes the bar on command bar `Unloaded` only when neither that command bar nor the strip is still loaded, posts WM_SIZE after row changes, and limits symbol-resolution attempts to one per module. An installed 0.7.2 build that held the added grid rows by weak reference left an empty row in the address-bar grid after the mod was disabled, shifting the address bar up; the rows are strong references again, as in 0.7.1. The user confirmed the 0.7.3 build in Explorer, including disabling the mod with a window open, and the Windhawk AI review of 0.7.3 found no blocking issues.
- **0.7.4:** Text only. README and settings state that FX supports folders on local fixed drives only; network locations (including standard folders redirected to a share) and removable or optical drives are unsupported and may be missing. Double Decker 0.7.3 carries the same text.
- **0.7.5:** Text only. The FX custom folders group description, which Settings shows without expanding an entry, also states the local-fixed-drive limit. Double Decker 0.7.4 carries the same text.
- **0.7.6:** Text only. The settings group description, shown at the top of Settings, explains where FX is, what its left-click menu lists, the order and limit (24) of custom entries, click versus Ctrl+click, right-click drives, and that changes apply to newly opened windows. Label and path field descriptions use plain wording with examples. Double Decker 0.7.5 carries the same text, placing FX on its upper row. Local compilation alone does not verify Explorer behavior. The Double Decker source received only the FX filter and remains untested in Explorer.
- **Drive menu:** Right-click FX enumerates logical drives and includes only roots that currently resolve to directories. Both sources rebuild the list whenever the menu opens.
- **Backup and network paths:** Save/Load uses file dialogs, with the former profile filenames suggested by default. Remote and otherwise uncertain paths are checked on one lazily started background worker per process; they stay hidden until a check confirms a folder. Results are cached for 30 seconds.
- **Architecture:** Both sources declare `x86-64`. Windhawk uses x64 on x64 Windows and ARM64 for `explorer.exe` on Windows ARM64. Local compilation checks cover both targets; live Explorer behavior on ARM64 has not been verified.

## Historical constraints

- New diagnostics use `%TEMP%` through `GetTempPathW`. Bookmark backup JSON is persistent data. Local DLL checks do not install a Windhawk mod.
