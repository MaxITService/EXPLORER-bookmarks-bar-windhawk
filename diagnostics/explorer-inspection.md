# Read-only Explorer inspection and useful XAML fixtures

Research checked on 2026-10-02. The existing observations in
[layout.md](layout.md) remain the source of the private header structure and
96-DPI geometry. No current Explorer XAML capture was performed for this work.

## CLI snapshot

Run this in a terminal against the explicitly selected existing Explorer HWND:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\inspect-explorer-uia.ps1 -WindowHandle 123456
```

Replace `123456` with the actual HWND, not a process ID. To list candidate handles
without opening, focusing or resizing a window:

```powershell
Get-Process explorer | Select-Object Id, MainWindowHandle, MainWindowTitle
```

`MainWindowHandle` may be zero and does not enumerate every Explorer window.
The helper accepts any explicitly identified window, allowing validation with an
owned test window. It saves JSON only under TEMP and returns the path. Names may
contain personal folder information; the snapshot is local diagnostic data and
must not be committed. No target control patterns/actions are invoked.

The helper creates an MTA PowerShell inspector process, bounds tree depth and
element count, and stops only that child after a timeout (default 20 seconds).
Provider calls are cross-process and can block; a traversal bound alone would
not impose a wall-clock limit. Errors and truncation are recorded in the report;
a snapshot is not atomic when the target changes while being read.

## What the snapshot can establish

Microsoft's [UI Automation tree overview](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-treeoverview)
defines Raw, Control and Content views of the provider's automation elements.
Even Raw is an accessibility tree, not a private XAML object dump. It can expose
control identities, visible bounding rectangles and process/framework information;
layout panels can be absent. It cannot establish Grid row definitions, padding,
DesiredSize, RenderTransform, event delegates or theme-resource dictionaries.
UIA rectangles use screen coordinates; do not compare them directly to the
root-relative XAML layout units in layout.md, especially at non-100% DPI.
Microsoft's [threading guidance](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-threading)
recommends a non-UI MTA client thread. [Inspect](https://learn.microsoft.com/en-us/windows/win32/winauto/inspect-objects)
is an SDK accessibility inspection GUI, not a private XAML CLI exporter.

For an actual XAML visual tree, [VisualTreeHelper](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.media.visualtreehelper?view=windows-app-sdk-1.8)
traverses DependencyObjects in an application's runtime tree, including generated
template parts. It is not a cross-process HWND-to-XAML API.
[InitializeXamlDiagnosticsEx](https://learn.microsoft.com/en-us/windows/win32/api/xamlom/nf-xamlom-initializexamldiagnosticsex)
documents the TAP DLL injected into the target for a diagnostics session.
[UWPSpy's own repository](https://github.com/m417z/UWPSpy) identifies its use of
these diagnostics APIs and supports WinUI 3. Such inspection is instrumentation,
even when the operator only reads properties; it is outside this helper's scope.
No injection, debugger attachment or installation is performed here.

## Fixture improvements and their limits

The installation fixture uses the observed sibling layout: tabs in row 0,
navigation/grid in row 1, command control/root/primary command bar in row 2.
The previous fixture placed the command bar inside the navigation grid, which
could mask wrong ancestor assumptions. Public WinUI controls represent private
Explorer controls; no claim is made that their templates are the same.

Checks span the risky boundaries before pursuing detail: recognized header
admission, real Loaded installation, 1–4 row allocation and collapse, tab and
command heights, strip containment/no overlap, duplicate discovery, and real
Unloaded restoration of the original Auto navigation row, children and minima.
A second attachment installs through the actual Loaded callback without an
explicit installation call from the test, and unloads cleanly again.
The native host sizing hook is not installed: the fixture supplies the independent
confirmed `40*N + 4` reservation and tests WinUI's resulting geometry. This does
not test the hook's physical-pixel/DPI calculation or Explorer's host behavior.
The 38-unit tabs, 48-unit command row, 134-unit stock header and 3-unit grid
padding come from the recorded 96-DPI observations, not general Explorer guarantees.

No theme brushes are invented to make a popup pass. The CBS 1.6 standalone RC
radio-menu template gap remains unresolved; only a representative host/resource
environment or an explicitly authorized installed-version check can close it.
Real Explorer enable/unload, high DPI, physical routed input, private templates,
Windhawk persistence and live native hook installation remain integration gaps.
