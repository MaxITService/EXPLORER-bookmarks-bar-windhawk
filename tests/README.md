# Progressive runtime coverage

Run `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\test-stability.ps1`
from the repository root. The executable includes the unchanged development
source with Windhawk stubs. It creates hidden native test windows and temporary
build output; it does not install or activate the mod.

Coverage proceeds across high-risk boundaries rather than source-file order:

| Area | Automated checks |
| --- | --- |
| Import validation | Empty backup, local/UNC paths, relative paths, trailing separators, control characters and embedded NUL, Unicode case duplicates, count and storage limits |
| Bookmark insertion | Mixed valid/invalid/duplicate/full results, normalization, ordering, append beyond end; every initial count 0–32 and every insertion position through one beyond end, including retries |
| Notice transport | Partial/full/duplicate outcomes, text, encode/decode, invalid kind, saturated counts |
| Encoding/storage | Multilingual UTF-8 round trip, overlong/truncated/surrogate UTF-8 rejection, unpaired UTF-16 rejection, line round trip, empty lines and limits |
| Layout arithmetic | Exact fit, half-unit tolerance, overflow, oversized button, confirmed 1–4 row host heights |
| Worker | Readers during blocked scan, last ready snapshot, obsolete generation rejection, request coalescing, handle cleanup/join |
| Settings/frame lifetime | Independent UI snapshots, cross-thread notifications, null/foreign/duplicate tracking, nested close, survivor preservation, COM teardown on owner thread, reopening |
| Storage locking | Independent named mutex handles serialize concurrent writers |

## Remaining integration coverage

This suite does **not** claim full program or line coverage. In particular,
Windhawk storage APIs are stubbed, so persisted read/write, backup file dialogs
and JSON file import/export need a separate injectable storage/file harness.
Real Shell recent-item scanning, settings loading, hook installation, icon
loading, navigation and drag/drop require additional integration fixtures.
Explorer XAML construction, focus, visual reflow, menu interaction and live
enable/unload behavior require testing in Explorer. The user controls installation;
the frozen catalog, personal and Double Decker sources are not altered or run.

The native window tests exercise multiple UI threads in one process; the mutex
test uses independent handles in one process, not two Explorer processes.
