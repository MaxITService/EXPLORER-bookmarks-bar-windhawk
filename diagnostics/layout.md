# Windows 11 Explorer header: measured geometry

Measured on 2026-09-27 with UWPSpy in **WinUI 3** mode and the mod's **Save
layout diagnostics** command. All `x`/`y` coordinates below are relative to
the header XAML island's root. Heights are WinUI layout units; the saved
snapshot reported `RasterizationScale=1.0` and window DPI 96, so they also
equal physical pixels in this observation. These are observations of one
Windows build, not constants guaranteed by Explorer.

## Provenance and limitations

- **Mod on:** installed personal source reported `mod=0.4.4`, Explorer PID
  11060, UI thread 13400. The three saved snapshots were taken at 01:13:38,
  01:14:11, and 01:15:36. The source adds one 38-unit row. The user reported
  clipping and extra space in the command row.
- **Mod off:** after Explorer restarted, UWPSpy inspected the new `explorer.exe`
  process PID 46704, thread 12680. The user confirmed the mod was disabled.
  This is a different process; PIDs are recorded only to identify the sessions.
- UWPSpy's Explorer island was off-screen/minimized while inspected. It reported
  **zero width** for several elements and shifted some horizontal coordinates.
  Therefore use its **vertical** rectangles below, not its horizontal sizes.
  The mod-on file supplied valid horizontal sizes from visible wide and narrow
  Explorer windows.
- The mod-on saved report has `root.DesiredSize.Height=155` but
  `root.ActualHeight=172`. Desired size is not the allocated size. The table
  below uses actual rectangles unless explicitly marked `desired`.
- The source and tag mentioned in the original measurement session are not
  part of the repository's current one-commit history.

## Header tree and row definitions

UWPSpy showed the second `DesktopWindowXamlSource` island containing:

```text
RootScrollViewer / ScrollContentPresenter / Border / Grid
  Row 0: FileExplorerTabControl
  Row 1: NavigationBarControl
    NavigationBarControlGrid
      NavigationCommands
      FileExplorerAddressBarGrid
      FileExplorerSearchBox
      [mod on] WindhawkExplorerFolderBookmarksBar
  Row 2: CommandBarControl
    CommandBarControlRootGrid
      FileExplorerCommandBar
      FileExplorerSecondaryCommandBar
```

The common Grid had `RowDefinitions=[Auto, *, *]` in **both** sessions. Its
two star rows have equal weight. The navigation grid had one `Auto` row with
the mod off; the mod appended a second, fixed 38-unit row. The navigation
grid's `Padding` was `3,3,3,3`, and its vertical alignment was centered.

| Element | Mod off: top..bottom (height) | Mod on: top..bottom (height) | Change |
| --- | ---: | ---: | ---: |
| Header root/common Grid | 0..134 (134) | 0..172 (172) | +38 |
| Tab control, Grid row 0 | 0..38 (38) | 0..38 (38) | 0 |
| Navigation control, row 1 | 38..86 (48) | 38..105 (67) | +19 |
| Command control, row 2 | 86..134 (48) | 105..172 (67) | +19 |
| NavigationBarControlGrid | 35..89 (54) | 26..118 (92) | +38 |
| Primary FileExplorerCommandBar | 86..134 (48) | 106..154 (48) | 0 height |
| Bookmark strip | absent | 77..115 (38) | +38 |

The last two star rows divide the added 38 units as `38 / 2 = 19` each.
The command control consequently has **19 units of unused height** while the
navigation control is **25 units shorter** than its 92-unit child Grid.
The child's centered layout places it about 12 units outside the navigation
control on each side. The saved report measured the strip bottom at 115 and
the primary command bar top at 106: `106 - 115 = -9`, an overlap of 9 units.

### Additional mod-off coordinates

| Element | Top..bottom | Height | Other observed properties |
| --- | ---: | ---: | --- |
| NavigationCommands | 38..86 | 48 | First child of navigation Grid |
| FileExplorerAddressBarGrid | 38..86 | 48 | Second child |
| FileExplorerSearchBox | 46..78 | 32 | Top/bottom margins 8; `MinHeight=32` |
| CommandBarControlRootGrid | 86..134 | 48 | Parent of both command bars |
| FileExplorerSecondaryCommandBar | 87..135 | 48 | Extends 1 unit below command control |

The unmodified navigation Grid already extends **3 units above and below**
its 48-unit navigation control (`35..89` versus `38..86`). The mod's 38-unit
row makes the Grid 92 units tall. A candidate that keeps the command row at
its natural 48 units therefore needs a navigation allocation of **92 units**:
`48 + 38 + 2*3 = 92`. The resulting candidate header height is
`38 + 92 + 48 = 178`, or **44 units above** the observed stock height of 134.
This 44-unit reservation is a measured candidate, not yet a validated layout.
The existing Windhawk hook logged an original **physical host height of 136**
at this scale; the XAML root was 134. The candidate retains 136 as the DPI
scaling denominator and adds 44 at 96 DPI. These are different layers of the
header and should not be conflated.

## Details from the mod-on snapshots

The two wide snapshots agreed; the narrow snapshot had identical vertical
coordinates. All three reported `grid rows=2`, `grid children=4`, appended
row `ActualHeight=38` and requested height 38. The grid and navigation
minimum heights were both changed from 0 to 38. Before insertion, the mod log
reported the navigation Grid and control as actual height 0 and desired
height 54; the installed code calculated its minima from `ActualHeight`,
which explains why those minima were only 38.

| Field | Wide window | Narrow window |
| --- | ---: | ---: |
| Explorer client size | 1918 x 2094 | 474 x 2094 |
| Window screen rectangle | 1088,0..3022,2102 | 1088,0..1578,2102 |
| Root/navigation/Grid width | 1918 | 474 |
| Root actual / desired height | 172 / 155 | 172 / 155 |
| Navigation actual / desired / min height | 67 / 67 / 38 | 67 / 67 / 38 |
| Navigation Grid actual / desired / min height | 92 / 67 / 38 | 92 / 67 / 38 |
| Bookmark strip actual height | 38 | 38 |
| Primary command bar actual / desired height | 48 / 48 | 48 / 48 |
| Primary command bar width | 716 | 339 |
| Strip extent / viewport width | 1912 / 1912 | 707 / 468 |
| Strip horizontal offset | 0 | 0 |
| Gap: command top minus strip bottom | -9 | -9 |

At the wide width, the strip's horizontal rectangle was `x=3..1915`;
at the narrow width, `x=3..471`. Its `y=77..115` remained unchanged. The
buttons panel's extent was 707 units wide in both snapshots. The narrow
viewport of 468 units requires horizontal scrolling; it did not cause the
vertical defect.

| Mod-on button | Actual size | Desired size | Margin L,T,R,B | Root rectangle x / y |
| --- | ---: | ---: | ---: | ---: |
| `+` | 24 x 24 | 40 x 38 | 8,3,8,11 | 11..35 / 80..104 |
| `FX` | 32 x 24 | 48 x 38 | 0,3,16,11 | 43..75 / 80..104 |
| Bookmark 1 | 117 x 32 | 125 x 38 | 0,3,8,3 | 91..208 / 80..112 |
| Bookmark 2 | 142 x 32 | 150 x 38 | 0,3,8,3 | 216..358 / 80..112 |
| Bookmark 3 | 122 x 32 | 130 x 38 | 0,3,8,3 | 366..488 / 80..112 |
| Bookmark 4 | 206 x 32 | 214 x 38 | 0,3,8,3 | 496..702 / 80..112 |

## What to verify in future variants

1. Record the same root, tab, navigation, navigation Grid, strip, command
   control, and primary command bar rectangles, plus their `DesiredSize`,
   `MinHeight`, margins, and row definitions. A command bar height of 48 is
   different from a 67-unit **command row**.
2. Capture both wide and narrow windows and note DPI/rasterization scale.
   Recheck at a non-100% scale before generalizing the 44-unit calculation.
3. Require the strip to stay inside the navigation allocation and measure
   `command top - strip bottom` again. The command row should retain its
   natural height and the address/search controls should not move toward tabs.
4. Test a newly opened Explorer window after compiling and enabling the new
   source in Windhawk. A local syntax/link check does not measure the running
   Explorer layout. Check unloading as well: the mod must restore Explorer's
   original row definition.

Microsoft's [Grid.RowDefinitions documentation](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.grid.rowdefinitions?view=windows-app-sdk-1.8)
describes how star rows share remaining space. The
[RowDefinition.Height documentation](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.rowdefinition.height?view=windows-app-sdk-1.8)
documents `Auto`, star sizing, and row constraints.
