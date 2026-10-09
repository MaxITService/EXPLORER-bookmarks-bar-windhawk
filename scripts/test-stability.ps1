param([switch]$Coverage, [string]$CoverageDirectory, [switch]$Xaml, [switch]$PreviewXaml,
      [string]$TestSource = 'tests\stability-runtime.cpp')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$compiler = 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe'
$include = 'C:\Program Files\Windhawk\Compiler\include'
$testSource = if ([IO.Path]::IsPathRooted($TestSource)) {
    [IO.Path]::GetFullPath($TestSource)
} else {
    Join-Path $projectRoot $TestSource
}
$buildDirectory = Join-Path ([IO.Path]::GetTempPath()) ('bookmarks-tests-' + [guid]::NewGuid().ToString('N'))
$testExe = Join-Path $buildDirectory 'bookmarks-stability.exe'
$stubFile = Join-Path $buildDirectory 'com-stubs.generated.h'
$modCopy = Join-Path $buildDirectory 'explorer-folder-bookmarks-bar.wh.cpp'
$testOriginalPath = $env:PATH
$testOriginalProfile = $env:LLVM_PROFILE_FILE
$coverageFlags = @()
if ($Coverage) {
    if (-not $CoverageDirectory) {
        $CoverageDirectory = Join-Path ([IO.Path]::GetTempPath()) ('bookmarks-coverage-' + [guid]::NewGuid().ToString('N'))
    }
    $coverageAbsolute = [IO.Path]::GetFullPath($CoverageDirectory)
    $projectAbsolute = [IO.Path]::GetFullPath($projectRoot).TrimEnd('\', '/')
    if ($coverageAbsolute.Equals($projectAbsolute, [StringComparison]::OrdinalIgnoreCase) -or
        $coverageAbsolute.StartsWith($projectAbsolute + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Coverage output must stay outside the repository; use a directory under TEMP.'
    }
    New-Item -ItemType Directory -Force -Path $CoverageDirectory | Out-Null
    $CoverageDirectory = (Resolve-Path -LiteralPath $CoverageDirectory).Path
    $runProfilePattern = 'runtime-' + [guid]::NewGuid().ToString('N') + '-*.profraw'
    $env:LLVM_PROFILE_FILE = Join-Path $CoverageDirectory ($runProfilePattern.Replace('*', '%p'))
    $coverageFlags = @('-fprofile-instr-generate', '-fcoverage-mapping',
                      ('-fcoverage-prefix-map=' + $buildDirectory + '=' + (Join-Path $projectRoot 'mods')))
}
try {
    New-Item -ItemType Directory -Path $buildDirectory | Out-Null
    & (Join-Path $PSScriptRoot 'generate-com-stubs.ps1') -IncludeDirectory $include -OutputFile $stubFile
    & (Join-Path $PSScriptRoot 'prepare-test-source.ps1') -OutputFile $modCopy
    $env:PATH = (Split-Path -Parent $compiler) + ';' + $env:PATH
    & $compiler '--target=x86_64-w64-windows-gnu' '-std=c++23' '-O1' '-static' `
        '-DUNICODE' '-D_UNICODE' '-DWINVER=0x0A00' '-D_WIN32_WINNT=0x0A00' `
        '-D_WIN32_IE=0x0A00' '-DNTDDI_VERSION=0x0A000008' '-DWH_MOD' '-DWH_EDITING' `
        @coverageFlags '-I' $buildDirectory '-I' (Join-Path $projectRoot 'tests') '-I' $include $testSource '-o' $testExe '-ladvapi32' '-lole32' '-loleaut32' '-lshell32' `
        '-luuid' '-lruntimeobject' '-lwindowscodecs' '-lcomctl32' '-lwer'
    if ($LASTEXITCODE -ne 0) { throw 'Stability test build failed' }
    if ($PreviewXaml) {
        & $testExe --preview
    } else {
        & $testExe
    }
    if ($LASTEXITCODE -ne 0) { throw "Stability tests failed, exit code $LASTEXITCODE" }
    if ($Xaml) {
        & $testExe --xaml
        if ($LASTEXITCODE -ne 0) { throw "XAML tests failed, exit code $LASTEXITCODE" }
    }
    if ($Coverage) {
        $profile = Join-Path $CoverageDirectory 'runtime.profdata'
        $rawProfiles = @(Get-ChildItem -LiteralPath $CoverageDirectory -Filter $runProfilePattern | ForEach-Object FullName)
        & 'C:\Program Files\LLVM\bin\llvm-profdata.exe' merge -sparse @rawProfiles -o $profile
        if ($LASTEXITCODE -ne 0) { throw 'Coverage profile merge failed' }
        $modSource = Join-Path $projectRoot 'mods\explorer-folder-bookmarks-bar.wh.cpp'
        $ignoreTests = '-ignore-filename-regex=tests[/\\]|com-stubs\.generated\.h'
        & 'C:\Program Files\LLVM\bin\llvm-cov.exe' report $testExe "-instr-profile=$profile" $ignoreTests $modSource
        if ($LASTEXITCODE -ne 0) { throw 'Coverage report failed' }
        & 'C:\Program Files\LLVM\bin\llvm-cov.exe' export $testExe "-instr-profile=$profile" $ignoreTests $modSource |
            Set-Content -LiteralPath (Join-Path $CoverageDirectory 'coverage.json') -Encoding UTF8
        if ($LASTEXITCODE -ne 0) { throw 'Coverage export failed' }
        & 'C:\Program Files\LLVM\bin\llvm-cov.exe' show $testExe "-instr-profile=$profile" $ignoreTests $modSource '-format=html' "-output-dir=$CoverageDirectory"
        if ($LASTEXITCODE -ne 0) { throw 'Coverage HTML generation failed' }
        Write-Output "Coverage artifacts: $CoverageDirectory"
    }
} finally {
    $env:PATH = $testOriginalPath
    $env:LLVM_PROFILE_FILE = $testOriginalProfile
    Remove-Item -LiteralPath $testExe -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $stubFile -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $modCopy -Force -ErrorAction SilentlyContinue
    # Do not let a PowerShell directory-removal error hide the test failure.
    # Remove only this run's known files and its empty TEMP build directory.
    try {
        if ([IO.Directory]::Exists($buildDirectory) -and
            [IO.Directory]::GetFileSystemEntries($buildDirectory).Length -eq 0) {
            [IO.Directory]::Delete($buildDirectory)
        }
    } catch { Write-Warning "Could not remove TEMP build directory: $buildDirectory" }
}
