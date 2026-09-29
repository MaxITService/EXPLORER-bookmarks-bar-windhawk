param(
    [Parameter(Mandatory = $true)]
    [string]$Source,
    [ValidateSet('x86_64-w64-windows-gnu', 'aarch64-w64-windows-gnu')]
    [string]$Target = 'x86_64-w64-windows-gnu'
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$sourcePath = if ([IO.Path]::IsPathRooted($Source)) {
    [IO.Path]::GetFullPath($Source)
} else {
    [IO.Path]::GetFullPath((Join-Path $projectRoot $Source))
}
$compiler = 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe'
$include = 'C:\Program Files\Windhawk\Compiler\include'
$tempDirectory = [IO.Path]::GetTempPath()

foreach ($path in @($sourcePath, $compiler)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing file: $path"
    }
}
if (-not (Test-Path -LiteralPath $include -PathType Container)) {
    throw "Missing include directory: $include"
}
if (-not (Test-Path -LiteralPath $tempDirectory -PathType Container)) {
    throw "Missing temporary directory: $tempDirectory"
}

# Windhawk 1.7 and later compile mods with Clang 20 in C++23 mode. These
# flags mirror the editor's C:\ProgramData\Windhawk\EditorWorkspace\
# compile_flags.txt, minus -x, -target and -include, which are set per pass.
$windhawkDefines = @(
    '-std=c++23', '-DUNICODE', '-D_UNICODE', '-DWINVER=0x0A00',
    '-D_WIN32_WINNT=0x0A00', '-D_WIN32_IE=0x0A00',
    '-DNTDDI_VERSION=0x0A000008', '-D__USE_MINGW_ANSI_STDIO=0'
)
$windhawkWarnings = @(
    '-Wall', '-Wextra', '-Wno-unused-parameter',
    '-Wno-missing-field-initializers', '-Wno-cast-function-type-mismatch'
)

# Warn when the installed editor's flags drift from the copy above.
$editorFlagsFile = 'C:\ProgramData\Windhawk\EditorWorkspace\compile_flags.txt'
if (Test-Path -LiteralPath $editorFlagsFile -PathType Leaf) {
    $editorFlags = @(Get-Content -LiteralPath $editorFlagsFile |
        ForEach-Object { $_.Trim() } |
        Where-Object { $_ -match '^-(std|D|W)' })
    $expectedFlags = $windhawkDefines + $windhawkWarnings + @('-DWH_MOD', '-DWH_EDITING')
    $drift = @(Compare-Object -ReferenceObject $editorFlags -DifferenceObject $expectedFlags)
    if ($drift.Count -gt 0) {
        Write-Warning ('Windhawk editor flags differ from this script: ' +
            (($drift | ForEach-Object { "$($_.SideIndicator) $($_.InputObject)" }) -join ', '))
    }
}

$buildId = [guid]::NewGuid().ToString('N')
$prelude = Join-Path $tempDirectory "windhawk-bookmarks-$buildId-defines.h"
$testDll = Join-Path $tempDirectory "windhawk-bookmarks-$buildId-build-check.dll"
$defines = @(
    '#define WH_MOD 1'
    '#define WH_MOD_ID L"explorer-folder-bookmarks-bar"'
    '#define WH_MOD_VERSION L"compile-check"'
) -join [Environment]::NewLine

try {
    [IO.File]::WriteAllText($prelude, $defines + [Environment]::NewLine)

    # Pass 1: the flags of Windhawk's own editor compile. Warnings are shown
    # but do not fail the check, as in the editor. Output is captured with
    # ErrorAction relaxed because Windows PowerShell 5.1 turns redirected
    # native stderr into terminating errors under 'Stop'.
    $editorArgs = @('-x', 'c++', "--target=$Target") + $windhawkDefines +
        $windhawkWarnings + @('-DWH_MOD', '-DWH_EDITING', '-I', $include,
        '-include', 'windhawk_api.h', '-fsyntax-only', $sourcePath)
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $editorOutput = @(& $compiler @editorArgs 2>&1 | ForEach-Object { "$_" })
    $editorExit = $LASTEXITCODE
    $ErrorActionPreference = $previousPreference
    $editorWarnings = @($editorOutput | Where-Object { $_ -match 'warning:' })
    if ($editorExit -ne 0 -or $editorWarnings.Count -gt 0) {
        $editorOutput | Write-Output
    }
    if ($editorExit -ne 0) { throw 'Windhawk editor-flags syntax check failed' }
    if ($editorWarnings.Count -gt 0) {
        Write-Warning "Windhawk editor flags report $($editorWarnings.Count) warning(s)"
    }

    # Pass 2: the macros as a compiled (non-editing) mod sees them.
    $syntaxArgs = @("--target=$Target") + $windhawkDefines + @(
        '-fsyntax-only', '-I', $include, '-include', $prelude, $sourcePath
    )
    & $compiler @syntaxArgs
    if ($LASTEXITCODE -ne 0) { throw 'Windhawk macro syntax check failed' }

    # Pass 3: a standalone test-only DLL that stubs Windhawk through WH_EDITING.
    $linkArgs = @("--target=$Target") + $windhawkDefines + @(
        '-shared', '-O2', '-DWH_MOD', '-DWH_EDITING', '-I', $include,
        $sourcePath, '-o', $testDll, '-Wl,--export-all-symbols',
        '-lole32', '-loleaut32', '-lshell32', '-luuid',
        '-lruntimeobject', '-lwindowscodecs'
    )
    & $compiler @linkArgs
    if ($LASTEXITCODE -ne 0) { throw 'Standalone test DLL build failed' }

    Write-Output "Local $Target editor-flags, syntax and link checks passed: $sourcePath"
} finally {
    foreach ($path in @($prelude, $testDll)) {
        Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue
    }
}
