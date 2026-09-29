param([switch]$Coverage, [string]$CoverageDirectory)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$compiler = 'C:\Program Files\Windhawk\Compiler\bin\clang++.exe'
$include = 'C:\Program Files\Windhawk\Compiler\include'
$testSource = Join-Path $projectRoot 'tests\stability-runtime.cpp'
$testExe = Join-Path ([IO.Path]::GetTempPath()) ('bookmarks-stability-' + [guid]::NewGuid().ToString('N') + '.exe')
$testOriginalPath = $env:PATH
$testOriginalProfile = $env:LLVM_PROFILE_FILE
$coverageFlags = @()
if ($Coverage) {
    if (-not $CoverageDirectory) {
        $CoverageDirectory = Join-Path ([IO.Path]::GetTempPath()) ('bookmarks-coverage-' + [guid]::NewGuid().ToString('N'))
    }
    New-Item -ItemType Directory -Force -Path $CoverageDirectory | Out-Null
    $CoverageDirectory = (Resolve-Path -LiteralPath $CoverageDirectory).Path
    $env:LLVM_PROFILE_FILE = Join-Path $CoverageDirectory 'runtime.profraw'
    $coverageFlags = @('-fprofile-instr-generate', '-fcoverage-mapping')
}
try {
    $env:PATH = (Split-Path -Parent $compiler) + ';' + $env:PATH
    & $compiler '--target=x86_64-w64-windows-gnu' '-std=c++23' '-O1' '-static' `
        '-DUNICODE' '-D_UNICODE' '-DWINVER=0x0A00' '-D_WIN32_WINNT=0x0A00' `
        '-D_WIN32_IE=0x0A00' '-DNTDDI_VERSION=0x0A000008' '-DWH_MOD' '-DWH_EDITING' `
        @coverageFlags '-I' $include $testSource '-o' $testExe '-lole32' '-loleaut32' '-lshell32' `
        '-luuid' '-lruntimeobject' '-lwindowscodecs' '-lcomctl32'
    if ($LASTEXITCODE -ne 0) { throw 'Stability test build failed' }
    & $testExe
    if ($LASTEXITCODE -ne 0) { throw "Stability tests failed, exit code $LASTEXITCODE" }
    if ($Coverage) {
        $profile = Join-Path $CoverageDirectory 'runtime.profdata'
        & 'C:\Program Files\LLVM\bin\llvm-profdata.exe' merge -sparse $env:LLVM_PROFILE_FILE -o $profile
        if ($LASTEXITCODE -ne 0) { throw 'Coverage profile merge failed' }
        $modSource = Join-Path $projectRoot 'mods\explorer-folder-bookmarks-bar.wh.cpp'
        & 'C:\Program Files\LLVM\bin\llvm-cov.exe' report $testExe "-instr-profile=$profile" $modSource
        if ($LASTEXITCODE -ne 0) { throw 'Coverage report failed' }
        & 'C:\Program Files\LLVM\bin\llvm-cov.exe' export $testExe "-instr-profile=$profile" $modSource |
            Set-Content -LiteralPath (Join-Path $CoverageDirectory 'coverage.json') -Encoding UTF8
        if ($LASTEXITCODE -ne 0) { throw 'Coverage export failed' }
        & 'C:\Program Files\LLVM\bin\llvm-cov.exe' show $testExe "-instr-profile=$profile" $modSource '-format=html' "-output-dir=$CoverageDirectory"
        if ($LASTEXITCODE -ne 0) { throw 'Coverage HTML generation failed' }
        Write-Output "Coverage artifacts: $CoverageDirectory"
    }
} finally {
    $env:PATH = $testOriginalPath
    $env:LLVM_PROFILE_FILE = $testOriginalProfile
    Remove-Item -LiteralPath $testExe -Force -ErrorAction SilentlyContinue
}
