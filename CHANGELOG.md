# Changelog

## Unreleased: changes since 0.7.8

Development source in `mods/`, tested in File Explorer.

### New

- **Ctrl+B** (off by default) bookmarks the current folder or removes its
  bookmark. With folders selected in the file list, it bookmarks them all,
  or removes them if all are bookmarked. Ignored while typing.
- Drag folders onto the bar to bookmark them where you drop them.
  One Ctrl+B or drop takes at most 20 folders.
- Right-click a bookmark to open it in a new window.
- A short notice when the bar is full or a folder is already bookmarked.
- **Recent folders** (off by default): buttons at the right end of the bar
  (3 by default, up to 25) and an **RC** menu with the other folders you
  opened in File Explorer, 10 in total by default and up to 25.
  Middle-click removes one; drag it onto the bookmarks to keep it. Turning
  the feature off erases the list.
- Buttons with the same folder name can also show their parent folder
  (off by default).

### Changed

- Disabling or recompiling the mod while dropped folders are still being
  read cancels that read and waits for it to finish.
- Saved settings apply to open windows; no new window needed.
- Bookmark and recent-folder changes show in all open windows right away.
- Hovering a bookmark updates its icon and missing-folder mark without
  redrawing the bar.
- Desktop, Documents and Downloads in FX follow the custom-folder rule:
  hidden only when missing from a local disk or when their drive is
  unplugged.

### Fixed

- A new window no longer stalls when Desktop, Documents or Downloads point
  to an unreachable network share.
- Backups saved with a UTF-8 byte order mark now load.
- Paths that differ only in the case of non-Latin letters (e.g. Cyrillic)
  count as the same folder and can't be bookmarked twice.
- A backup whose path contains a hidden NUL character is rejected instead
  of loading a shortened path.
- Errors in the bar's click, menu and drag handlers are caught and logged.

## 0.7.8 (2026-09-29)

First release in the Windhawk catalog.
