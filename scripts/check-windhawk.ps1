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

    $syntaxArgs = @(
        "--target=$Target", '-std=c++20', '-fsyntax-only',
        '-DUNICODE', '-D_UNICODE', '-D_WIN32_WINNT=0x0A00',
        '-I', $include, '-include', $prelude, $sourcePath
    )
    & $compiler @syntaxArgs
    if ($LASTEXITCODE -ne 0) { throw 'Windhawk macro syntax check failed' }

    $linkArgs = @(
        "--target=$Target", '-std=c++20', '-shared', '-O2',
        '-DWH_MOD', '-DWH_EDITING', '-DUNICODE', '-D_UNICODE',
        '-D_WIN32_WINNT=0x0A00', '-I', $include,
        $sourcePath, '-o', $testDll, '-Wl,--export-all-symbols',
        '-lole32', '-loleaut32', '-lshell32', '-luuid',
        '-lruntimeobject', '-lwindowscodecs'
    )
    & $compiler @linkArgs
    if ($LASTEXITCODE -ne 0) { throw 'Standalone test DLL build failed' }

    Write-Output "Local $Target syntax and link checks passed: $sourcePath"
} finally {
    foreach ($path in @($prelude, $testDll)) {
        Remove-Item -LiteralPath $path -Force -ErrorAction SilentlyContinue
    }
}
