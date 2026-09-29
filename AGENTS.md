# Explorer Folder Bookmarks Bar: agent instructions

## Sources

- Read [DECISIONS.md](DECISIONS.md) before changing layout. The production layout derives from the user-confirmed 0.4.7 geometry; measurements are in `diagnostics/layout.md`.
- Find the repository root with `git rev-parse --show-toplevel`. Four mod sources belong in this repository: the root `explorer-folder-bookmarks-bar.wh.cpp` is the exact, user-tested 0.7.8 catalog submission snapshot from commit `d89e838`; `mods/explorer-folder-bookmarks-bar.wh.cpp` is the newer source-only development version (0.8.10, untested in Explorer); `explorer-folder-bookmarks-bar-double.wh.cpp` is the untested Double Decker candidate (0.7.8); and `explorer-folder-bookmarks-bar-personal.wh.cpp` is the user's personal build (0.8.5, untested). PR ramensoftware/windhawk-mods#5755 was merged on September 29, 2026; catalog publication may lag. The personal build is production plus recent folders in the Open/Save dialogs of all processes (`@include *`); it is never submitted to the Windhawk catalog. **Keep the root 0.7.8 snapshot exact. The personal build and Double Decker candidate remain frozen: no changes or version bumps until the user lifts each freeze.** The personal build's `@include *` loads the mod into every process, and a `CoCreateInstance` hook in each of them may look suspicious to anti-cheat software; Double Decker is limited to `explorer.exe`. Bump `@version` in every source you edit, including text-only edits.
- All variants use the same mod ID and must be compiled one at a time. Windhawk's editor copy under `C:\ProgramData\Windhawk\EditorWorkspace\` is separate from the repository source.

## Working rules

- Before editing, run `git status --short --branch` and preserve existing local changes.
- Put diagnostic and build scratch files under `%TEMP%`, never directly in the root of `%USERPROFILE%`. In the mod, use `GetTempPathW` and fail safely if no temporary directory exists. Bookmark backup JSON is persistent user data selected with a Save/Open dialog; the historical profile filename is only the suggested default.
- For GitHub imports, identify the exact repository, ref, and path; verify downloaded bytes against the GitHub blob SHA with `git hash-object` before replacing a file.
- Untested paths need `Wh_Log` breadcrumbs so the user's test report is diagnosable. `Wh_Log` evaluates its arguments only while logging is enabled, so keep them free of side effects and reuse values the code already computed.
- Before committing, run `git diff --check`. Commit requested changes, then run `git status --short`. Report each commit's SHA and message and any remaining changes.
- Push to GitHub or any other Git remote only when the user explicitly instructs you to push. Requests to edit, commit, or finish work do not authorize a push.

## Local compile checks

Run the checked-in script for each changed source. Check the root 0.7.8 source when adding or otherwise changing it. The frozen personal and Double Decker builds are neither changed nor checked while their freeze lasts.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\check-windhawk.ps1 -Source mods\explorer-folder-bookmarks-bar.wh.cpp
```

When a freeze is lifted, add `-Source explorer-folder-bookmarks-bar-double.wh.cpp` or `-Source explorer-folder-bookmarks-bar-personal.wh.cpp` back to the checks, including for shared changes.

The script first compiles with the flags of Windhawk's editor (Clang 20, C++23, `-Wall -Wextra`, mirrored from `C:\ProgramData\Windhawk\EditorWorkspace\compile_flags.txt`; it warns when that file drifts and prints any compiler warnings), then checks Windhawk macro syntax and builds a standalone test-only DLL for x86-64 by default. Pass `-Target aarch64-w64-windows-gnu` to check ARM64. The bundled compiler otherwise defaults to i686. Recheck `@architecture` and `@compilerOptions` if source metadata changes. The test DLL stubs Windhawk APIs through `WH_EDITING`; never install or distribute it. A passing local check does not compile or enable the installed mod.

## Installed mod

- The user installs and tests variants. Do not operate Windhawk or switch the installed mod without a new explicit request.
- When an installed update is requested, use Windhawk's **Compile Mod**, enable the mod, and verify it in a new Explorer window. A repository edit or local DLL build is not activation.
- If the elevated editor cannot be controlled from this session, report that clearly. Do not bypass elevation or write directly to its protected workspace.
