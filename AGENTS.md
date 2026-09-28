# Explorer Folder Bookmarks Bar: agent instructions

## Sources

- Read [DECISIONS.md](DECISIONS.md) before changing layout. The production layout derives from the user-confirmed 0.4.7 geometry; measurements are in `diagnostics/layout.md`.
- Find the repository root with `git rev-parse --show-toplevel`. Only two mod sources belong in this repository: `mods/explorer-folder-bookmarks-bar.wh.cpp` (production, untested 0.7.3 candidate; based on the user-confirmed 0.7.1 build) and the untested Double Decker candidate `explorer-folder-bookmarks-bar-double.wh.cpp` (0.7.2). Repository FX changes after the installed 0.7.1 build still require installation and Explorer verification.
- All variants use the same mod ID and must be compiled one at a time. Windhawk's editor copy under `C:\ProgramData\Windhawk\EditorWorkspace\` is separate from the repository source.

## Working rules

- Before editing, run `git status --short --branch` and preserve existing local changes.
- Put diagnostic and build scratch files under `%TEMP%`, never directly in the root of `%USERPROFILE%`. In the mod, use `GetTempPathW` and fail safely if no temporary directory exists. Bookmark backup JSON is persistent user data selected with a Save/Open dialog; the historical profile filename is only the suggested default.
- For GitHub imports, identify the exact repository, ref, and path; verify downloaded bytes against the GitHub blob SHA with `git hash-object` before replacing a file.
- Before committing, run `git diff --check`. Commit requested changes, then run `git status --short`. Report each commit's SHA and message and any remaining changes.

## Local compile checks

Run the checked-in script for each changed source, including variants affected by shared changes:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\check-windhawk.ps1 -Source mods\explorer-folder-bookmarks-bar.wh.cpp
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\check-windhawk.ps1 -Source explorer-folder-bookmarks-bar-double.wh.cpp
```

The script checks Windhawk macro syntax and builds a standalone test-only DLL for x86-64 by default. Pass `-Target aarch64-w64-windows-gnu` to check ARM64. The bundled compiler otherwise defaults to i686. Recheck `@architecture` and `@compilerOptions` if source metadata changes. The test DLL stubs Windhawk APIs through `WH_EDITING`; never install or distribute it. A passing local check does not compile or enable the installed mod.

## Installed mod

- The user installs and tests variants. Do not operate Windhawk or switch the installed mod without a new explicit request.
- When an installed update is requested, use Windhawk's **Compile Mod**, enable the mod, and verify it in a new Explorer window. A repository edit or local DLL build is not activation.
- If the elevated editor cannot be controlled from this session, report that clearly. Do not bypass elevation or write directly to its protected workspace.
