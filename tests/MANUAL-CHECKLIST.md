# Manual testing checklist — Explorer Folder Bookmarks Bar

Test in real File Explorer using the already-installed mod. Record its version
and variant with your results. This is a checklist of expected behavior, not a
record of completed tests.

Before starting, save a backup of your current bookmarks using **right-click + →
Save bookmarks…**. Use disposable folders under a unique `%TEMP%` subfolder:
`A`, `B`, `C`, a folder with spaces and non-Latin characters, and a folder with a
long name. Keep your original backup separate from test backups. Restore your
bookmarks and any changed settings afterward.

Items marked **Development feature** target the development 0.8.18 source.
The catalog 0.7.8 does not have these features; skip them when unavailable.
Do not switch variants just to complete this list. The frozen personal and
Double Decker variants have different feature sets or layouts.

For each applicable item, record **Pass / Fail**, what happened, and the version.
Use **Skipped** for an unavailable feature or test environment. Keep completed
results, screenshots, backups and generated output outside the repository.

1. **Startup and active tabs**
   - **Steps:** Open three new Explorer windows. Create and switch tabs, navigate
     between folders, then close a tab and one window.
   - **Expected:** Each new window has one bar below the address bar. Controls
     remain usable; actions target the active tab. There is no duplicate bar,
     accumulating blank space, hang or crash. Existing windows from before mod
     activation are not required to acquire a bar.

2. **Add a folder, reject duplicates and virtual locations**
   - **Steps:** Open A and click **+** twice. Add B. Visit Home or This PC and
     click **+** again.
   - **Expected:** A and B each appear once. Virtual locations are not added.
     Existing bookmarks and their order remain intact.

3. **Open a bookmark in the intended tab or window**
   - **Steps:** From another folder, click A. Switch tabs and repeat. If the
     installed version supports it, right-click B.
   - **Expected:** A opens in the active tab. The supported right-click action
     opens B in a new Explorer window without redirecting the original tab.

4. **Ctrl+click opens a new tab**
   - **Steps:** Note the current folder and tab count, then Ctrl+click B once.
   - **Expected:** Explorer opens B in one new tab. The original tab retains its
     folder, and the bookmark list does not change.

5. **Remove a bookmark without deleting its folder**
   - **Steps:** Put a small disposable file in C, bookmark C, then middle-click
     its bookmark. Open C through the address bar.
   - **Expected:** Only the bookmark disappears. C and its file still exist;
     no unintended navigation or deletion occurs.

6. **Reorder within and across rows**
   - **Steps:** Drag A after B, then to the beginning and end. With enough
     bookmarks to wrap, move one to another row.
   - **Expected:** Each drop changes the order once at the indicated position.
     No bookmark is lost or duplicated. The insertion marker and dimming clear;
     the moved bookmark still opens its folder.

7. **Cancel a drag and activate again**
   - **Steps:** In separate attempts, start dragging A and release in its
     original position, outside the bar inside the window, and outside the
     entire window. After each attempt, click A once. Separately, establish that
     Enter or Space activates a focused bookmark, then repeat that supported
     keyboard activation after a cancelled drag, with focus on the bookmark.
   - **Expected:** The order remains unchanged, drag visuals clear, and the
     first subsequent click works. A keyboard activation that works before the
     drag still works afterward; lack of keyboard focus alone is not a failure.

8. **Persist additions, removals and order**
   - **Steps:** Add C, remove B and reorder A. Close all Explorer windows, then
     open a new one.
   - **Expected:** The saved list has exactly the same folders and order, with
     no removed bookmark returning and no added bookmark missing.

9. **Save and restore a backup**
   - **Steps:** Save the test list, including its Unicode folder, through
     **right-click + → Save bookmarks…**. Change the list, then load that file
     through **Load bookmarks…**. Open a new Explorer window.
   - **Expected:** The restored list replaces the changed list and preserves
     paths, Unicode characters and order. The restored bookmarks persist.

10. **Cancel dialogs and reject an invalid backup**
    - **Steps:** Cancel Save and Load separately. Make a disposable copy of a
      valid backup, break its JSON syntax and try loading it.
    - **Expected:** Cancellation creates or overwrites no backup. Invalid input
      leaves the complete current bookmark list unchanged. Explorer stays
      usable, and loading the untouched valid backup still works afterward.

11. **Enforce the 32-bookmark limit**
    - **Steps:** With your original backup secured, fill the list with 32
      distinct disposable folders. Try adding a 33rd. Remove one, then retry.
    - **Expected:** The list never exceeds 32 and existing entries are preserved.
      Adding succeeds after a slot is freed. Development 0.8.18 shows a short
      notice when the full list prevents an addition.

12. **Wrap, collapse and reach overflow**
    - **Steps:** Use enough long-label bookmarks to fill four rows. Slowly
      narrow and widen the window, maximize and restore it, then reach overflow
      by horizontal scrolling/panning.
    - **Expected:** The bar adapts between one and four rows and collapses again
      without losing order or actions. Every bookmark remains reachable. The
      address bar, command bar and file list are not clipped or overlapped.

13. **Labels, tooltips, themes and display scaling**
    - **Steps:** Hover bookmarks with long, spaced and non-Latin names. Check
      light and dark themes and, if available, another display scaling setting.
    - **Expected:** Tooltips show the full correct paths, labels remain readable
      with ellipsis when necessary, and icons and hit areas remain aligned.
      There is no text overlap or unusable control.

14. **FX standard and custom folders**
    - **Steps:** Left-click FX and open its standard entries. Add a custom label
      and a valid path using `%USERPROFILE%`, plus a blank entry, in settings.
      Save and open a new Explorer window. Test click and Ctrl+click on the
      custom entry.
    - **Expected:** Available standard folders and the custom entry appear;
      the variable expands correctly and the blank entry is omitted. Click
      opens the active tab; Ctrl+click opens a new tab.

15. **Drives and unavailable targets**
    - **Steps:** Right-click FX and open a known available drive. If you have a
      test USB drive, reopen the menu before and after connecting it. Rename a
      disposable bookmarked local folder and open a new Explorer window. Also
      check an unavailable network/removable target if one is available.
    - **Expected:** The drive menu reflects current drives. A missing local
      bookmark remains identifiable with a warning; a missing local FX folder
      is hidden. Opening an unavailable target is handled by Explorer, without
      corrupting bookmarks. Building the bar/menu does not probe unavailable
      network/removable folder targets.

16. **Disable, enable and clean up open windows**
    - **Steps:** With several Explorer windows open, disable the mod in
      Windhawk. Repeat once while a bookmark Save/Load dialog is open. Enable
      the mod again and open a new Explorer window.
    - **Expected:** Explorer stays usable; added controls and reserved space
      are removed, and an active bookmark dialog closes cleanly. The new window
      gets exactly one working bar with the saved bookmarks.

17. **Synchronize open panels and apply settings — Development feature**
    - **Steps:** Keep two windows open. Add, remove and reorder bookmarks in
      one; observe the other. Change an FX entry and the Recent settings. Close
      one window, repeat an edit, then open another.
    - **Expected:** Other open panels update without reopening. Saved settings
      apply to existing panels, and closing a window does not break updates in
      surviving or newly opened windows.

18. **Ctrl+B toggling and typing protection — Development feature**
    - **Steps:** With the file list focused and no selection, press Ctrl+B
      twice. Repeat with two folders selected, then a mixed folder/file
      selection. Press it while typing in the address bar, search box and a
      rename field. Turn the shortcut off and try again.
    - **Expected:** The current folder toggles; selected folders are added, or
      removed when all are already bookmarked. Files/ZIPs are not bookmarked.
      At most 20 folders are handled per press. Typing fields and the disabled
      shortcut leave bookmarks unchanged; holding the key does not repeatedly
      toggle them.

19. **Drop folders from the file list — Development feature**
    - **Steps:** Drag two folders from the file list onto the bar at a chosen
      insertion point. Repeat with a bookmarked folder and with mixed
      folders/files. In another attempt, release outside the bar. With spare
      capacity, try a selection of more than 20 folders.
    - **Expected:** Valid new folders are inserted at the drop position without
      duplicates. Files are ignored; source folders are not moved or deleted.
      An outside drop does not add bookmarks. One drop processes at most 20
      folders and the total remains at most 32.

20. **Recent folders, RC and promotion — Development feature**
    - **Steps:** Enable Recent and visit several unbookmarked local folders.
      Open RC and open a listed folder. Set **Buttons** and **Remembered
      folders** to 25 and visit more than 25 folders. Narrow and widen the
      window. Drag a recent folder into bookmarks; middle-click another
      recent folder. Exercise the configured RC right-click Clear action,
      then turn Recent off and on before reopening a window.
    - **Expected:** RC opens without a crash; newest buttons are nearest the
      right edge and RC stays at the edge when shown. In narrow windows the
      oldest buttons fold into RC first and return when space permits. Promotion creates one
      persistent bookmark and removes its recent duplicate. Middle-click and
      Clear affect recent history without deleting folders. Buttons and RC
      together list at most the configured number of folders. Remembered
      history survives off/on unless cleared.
