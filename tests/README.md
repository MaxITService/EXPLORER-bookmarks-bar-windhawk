# Progressive runtime coverage

For checks in the installed application, use the English
[20-item manual testing checklist](MANUAL-CHECKLIST.md).
For a read-only CLI accessibility snapshot and the limits of Explorer-like
XAML fixtures, see [Explorer inspection](../diagnostics/explorer-inspection.md).

Run `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\test-stability.ps1`
from the repository root. The executable uses a verified temporary copy of the
unchanged development source with isolated Windhawk storage/settings doubles. It creates hidden native test windows and temporary
build output; it does not install or activate the mod.

Add `-Xaml -Coverage -CoverageDirectory "$env:TEMP\bookmarks-coverage-current"`
to run real XAML checks in a second process and merge **only that run's** profiles.
The report measures the unchanged development mod, excluding test-source counts.
Open `index.html` in the selected temporary output directory to inspect missed
lines. Coverage output inside the repository is rejected. Executables, profile
data, reports and test JSON files are never tracked or committed.

`-PreviewXaml` opens a separate interactive visual fixture. Resize its window to
demonstrate wrapping with in-memory sample data; navigation and bookmark actions
are disabled. Close the preview window to release the temporary executable.
XAML modes require the already-installed Windows App Runtime CBS 1.6 x64 framework;
the fixture adds it only to its own process's package dependency graph.
The executable disables interactive Windows crash reporting for its own process;
a crash still produces a failing process exit code. Temporary cleanup preserves
the original test error rather than replacing it with a directory-removal error.

Coverage proceeds across high-risk boundaries rather than source-file order:

| Area | Automated checks |
| --- | --- |
| Import validation | Empty backup, local/UNC paths, relative paths, trailing separators, control characters and embedded NUL, Unicode case duplicates, count and storage limits |
| Bookmark insertion | Mixed valid/invalid/duplicate/full results, normalization, ordering, append beyond end; every initial count 0–32 and every insertion position through one beyond end, including retries |
| Notice transport | Partial/full/duplicate outcomes, text, encode/decode, invalid kind, saturated counts |
| Encoding/storage | Multilingual UTF-8 round trip, overlong/truncated/surrogate UTF-8 rejection, unpaired UTF-16 rejection, line round trip, empty lines and limits |
| Layout arithmetic | Exact fit, half-unit tolerance, overflow, oversized button, confirmed 1–4 row host heights |
| Settings/frame lifetime | Independent UI snapshots, cross-thread notifications, null/foreign/duplicate tracking, nested close, survivor preservation, COM teardown on owner thread, reopening |
| Storage locking | Independent named mutex handles serialize concurrent writers |
| Persisted operations | Add/remove/toggle/move, write failures without persisted mutation, history trimming, oversized reads |
| Backup files | Real JSON save/load, replacement, Unicode/BOM, malformed schemas/types/paths, embedded NUL, file size and count limits, failed storage commit |
| Settings/history | Both clamp boundaries, RC modes, FX paths/labels/deduplication, environment expansion, promotion and cap, removal and clear |
| Native Shell | Real local folder, file, missing and relative-path status |
| Callbacks | Guarded exceptions, reverse revocation despite failure, nested/unload operation scopes, timer coalescing, COM sink contracts |
| Icons | Real BGRA decoding, failed cache interval, cap/eviction, explicit invalidation, tooltip variants |
| Real XAML | Four-row reflow and collapse preserving identity, row clamps, external sizing, insertion markers/drop indices, recent folding and visibility, full/incremental refresh, menu variants, teardown |
| Loaded XAML tree | Measured sibling tab/navigation/command structure and stock geometry; install only with recognized header geometry, 1–4 row containment/no overlap and collapse, repeated installation/discovery, actual Loaded/Unloaded cleanup, restored host rows/children/minima, idempotent thread cleanup; live resize wraps/collapses and pins RC to the right edge |
| Shell COM fixtures | Active tab lookup failures, current-folder PIDL, selection file/ZIP/virtual filtering, 1000-item and 20-folder caps, Ctrl+B fallback, navigation flags/verbs, subscription startup/resync/revocation |
| File dialog fixtures | Save/Open classes, JSON filters, default folder/name, required flags, active dialog tracking, cancellation/errors and unload guard |
| Real XAML actions | Automation Invoke drives actual Click handlers for add/bookmark and backup commands; real FX/drive Opening events; notice creation/update/timer hide |
| Native coordinators | Same/cross-thread callbacks, own-process enumeration, desired-size boundaries, hook lookup/apply coordinators and abandoned/failed mutex waits |
| Platform faults | Failed/short backup transfers and atomic replacement, encoding/environment races, known-folder failures, WIC stages and ownership |
| Allocation failures | One-shot failures in backup storage/input buffers and icon pixel vectors; destination/storage preservation, native/COM cleanup and subsequent successful recovery |
| Branch boundaries | Exact JSON/storage sizes, failed recent writes, FX label/final slot limits and settings generations |
| Drag & drop | Link/Copy precedence and Move rejection, strip boundaries, asynchronous cancellation/errors, immediate/delayed/out-of-order completion, action limits, failed storage and real temporary StorageFolder/DataPackage integration |
| Pointer delegates | Original guarded callbacks, drag threshold/identity, release inside/outside, reorder, cancellation, recent promotion, middle removal and physical mouse-button recovery |
| Runtime boundary checks | Shell event argument contracts, initialization failure recovery and draining admitted operations/item reads |
| XAML boundary checks | Weak icon-cache ownership, exact bitmap pixels, incremental visual identity, refresh failure recovery, teardown ownership and installation discovery |
| Registered actions | Original folder/RC/FX/backup delegates, cancellation/unload guards, missing drives and installed root drag callbacks |

## Remaining integration coverage

This suite does **not** claim full program or line coverage. Windhawk persistence
is represented by in-memory doubles; actual Windhawk persistence and settings UI,
actual file picker UI, hook installation, navigation, physical routed input and live
Explorer enable/unload behavior still require more integration fixtures.
The real XAML checks exercise mod functions against a standalone tree; they do
not prove compatibility with Explorer's actual tree. The installation fixture
uses the recorded sibling header structure and independent 96-DPI measurements,
then checks stock geometry, strip containment and restoration. Public controls
represent Explorer's private controls. The native host sizing hook is not
installed; the fixture supplies its expected allocation explicitly. UI Automation
inspection exposes accessibility elements, not Grid definitions or private XAML
templates. Native broadcasts are
restricted to the test process so they cannot refresh the user's Explorer.
The user controls installation;
the frozen catalog, personal and Double Decker sources are not altered or run.

Dialog and navigation requests are intercepted by test doubles; automated tests
never show a real file picker or navigate the user's Explorer. Unused COM methods
are generated mechanically from local SDK declarations into the temporary build
directory; the handwritten fixtures supply behavior for the methods under test.

The native window tests exercise multiple UI threads in one process; the mutex
test uses independent handles in one process, not two Explorer processes.

The temporary source copy observes the guarded pointer delegates immediately
after their real `AddHandler` registration, as well as the registered Click,
ContextRequested, Opening and root pointer/drag delegates. Tests invoke those
same delegates with WinRT argument doubles; they do not synthesize physical input.
The one native `CapturePointer` call is routed through a test adapter only in
the temporary copy. A uniquely named synthetic pointer returns `false` without
entering native WinUI capture; real pointers retain the original call. Synthetic
Pointer and PointerPoint device types agree. The suite checks the mod's handlers,
not OS pointer capture. This removes an unsafe fixture boundary; it does not
establish the cause of the earlier standalone access violation.
Removing the observer fragments reconstructs the original source exactly, and
the preparation script verifies the repository source hash remains unchanged.
The observer uses an existing blank line and the original registration line,
preserving source line numbers. Clang's coverage prefix mapping resolves the copy
to the original source path; observer functions live in the excluded test header.
The copy is removed
with the other temporary build files.

Up to 0.8.15 the RC menu held recent-source radio items, and opening it in the
standalone CBS 1.6 host fail-fasted inside WinUI with missing
`TabViewScrollButtonBackground` and `TabViewButtonBackground` theme resources.
0.8.16 removed those items; opening the RC popup in the standalone host has not
been retried. RC population and clear callbacks are tested directly; RC popup
activation remains an integration gap.

Opt-in platform faults verify preservation of existing backup files and removal
of partial temporary files after failed or short writes, flushes and replacement.
Shell/WIC doubles also check resource ownership and release after failures.
Allocation faults select one allocation on the calling test thread and disarm
before throwing, allowing error handling and cleanup to allocate normally.
Bitmap factory faults run first in the XAML process and use non-agile factories
which the SDK does not cache; the prior activation handler is restored afterward.

Full coverage is still pending. Some remaining conditions are redundant guards
that cannot be reached after earlier validation, including CR/LF checks after all
control characters were rejected. Other gaps include allocation failures, input
strings larger than `INT_MAX`, and actual Explorer/Windhawk integration. These
conditions remain in the coverage denominator.

The latest progressive breadth pass strengthens existing groups across backup
picker failure/recovery, deleted-target navigation, native cleanup before bar
installation and measured XAML layout. It adds no source instrumentation or coverage exclusions. The original
source denominator remains unchanged; all previously covered branch outcomes
remain covered when aggregated across template instantiations.

Last completed ordinary aggregate run on 2026-10-09 (development 0.8.21):
**4,335/4,461 lines (97.18%)**, **1,751/1,911 branch outcomes (91.63%)**, and
**247/248 functions (99.60%)**. All native and XAML check groups passed, exit
code 0, without a debugger. This is not a claim of 100% coverage or
compatibility with live Explorer.

## Checking a failed scenario in the real application

When a check fails, report its name, the observed failure, and whether its input
represents a real user action or an injected platform fault. Give the matching
Explorer steps and expected behavior; a fixture failure alone is not evidence
that the installed mod fails. The suite targets development 0.8.33, so a manual
check of a different installed version validates only that version. Installation
and variant changes remain the user's choice.

For the earlier standalone pointer/drag crash, use the already-installed mod:

1. Add two ordinary temporary folders, A and B, as bookmarks. Drag A to the other
   side of B and release over the bar. The order changes once, highlighting clears
   and the button returns to normal opacity. A click still opens its folder.
2. Start another drag and release outside the bar. The saved order stays unchanged;
   the marker clears and the button returns to normal opacity.
3. If recent buttons are enabled, visit a third unbookmarked folder. Drag its recent
   button into the bookmarks bar. It becomes a bookmark, and the recent area no
   longer duplicates that bookmark. Reopen an Explorer window to verify persistence.

On 2026-10-01 the user confirmed bookmark release both inside the Explorer window
and outside the entire window works in the installed version. This verifies those
manual actions; it does not identify the cause of the earlier standalone crash.

For the known standalone RC popup-template gap, open RC in Explorer. Its menu
should display without closing/crashing Explorer; a folder command should open
that folder. The standalone preview has editing/navigation disabled
and cannot validate these input actions.

Injected out-of-memory, COM and transfer errors generally have no equivalent UI
gesture. Ordinary backup save/load and cancellation can check visible behavior
and file preservation, but cannot prove recovery from the injected failure itself.
