param([Parameter(Mandatory)][string]$IncludeDirectory,
      [Parameter(Mandatory)][string]$OutputFile)
$ErrorActionPreference = 'Stop'
# Only mechanical unused-method defaults are generated from the local SDK.
# Handwritten test doubles override methods whose behavior matters to assertions.
$sdkText = (@('shobjidl.h', 'exdisp.h', 'oleidl.h', 'oaidl.h', 'objidl.h', 'ocidl.h') |
    ForEach-Object { Get-Content -LiteralPath (Join-Path $IncludeDirectory $_) -Raw }) -join "`n"
function Get-Methods([string]$Interface) {
    if ($Interface -eq 'IUnknown') { return @() }
    $match = [regex]::Match($sdkText, '(?s)\b' + [regex]::Escape($Interface) + '\s*:\s*public\s+(\w+)\s*\{(.*?)\n\};')
    if (-not $match.Success) { throw "SDK interface not found: $Interface" }
    $inherited = @(Get-Methods $match.Groups[1].Value)
    $own = @([regex]::Matches($match.Groups[2].Value, '(?s)virtual\s+(\w+)\s+STDMETHODCALLTYPE\s+(\w+)\s*\((.*?)\)\s*=\s*0;') |
        ForEach-Object {
            $return = $_.Groups[1].Value
            $name = $_.Groups[2].Value
            $parameters = ($_.Groups[3].Value -replace '\s+', ' ').Trim()
            $default = if ($return -eq 'HRESULT') { 'E_NOTIMPL' } else { '0' }
            "    $return STDMETHODCALLTYPE $name($parameters) override { return $default; }"
        })
    return $inherited + $own
}
$code = [Collections.Generic.List[string]]::new()
$code.Add('#pragma once')
$code.Add('// Generated into TEMP. Do not commit this file.')
foreach ($interface in @('IShellBrowser', 'IShellView', 'IFolderView', 'IPersistFolder2',
                         'IShellItem', 'IShellItemArray', 'IWebBrowser2', 'IShellWindows', 'IFileDialog')) {
    $code.Add("struct Stub$interface : TestComObject<$interface> {")
    foreach ($method in (Get-Methods $interface)) { $code.Add($method) }
    $code.Add('};')
}
[IO.File]::WriteAllLines($OutputFile, $code, [Text.UTF8Encoding]::new($false))
