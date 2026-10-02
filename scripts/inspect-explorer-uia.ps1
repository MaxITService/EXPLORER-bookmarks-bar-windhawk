param(
    [Parameter(Mandatory=$true)][ValidateRange(1, [long]::MaxValue)][long]$WindowHandle,
    [ValidateRange(1,16)][int]$MaxDepth = 8,
    [ValidateRange(1,2000)][int]$MaxElements = 500,
    [ValidateRange(1,60)][int]$TimeoutSeconds = 20,
    [switch]$SnapshotWorker,
    [string]$WorkerOutput
)
$ErrorActionPreference = 'Stop'
# Read properties only. No Invoke, SetFocus, Expand, Scroll, navigation,
# diagnostic DLL injection or changes to the target process/window.
if (-not $SnapshotWorker) {
    if ($WorkerOutput) { throw 'WorkerOutput is internal; output always goes under TEMP.' }
    $output = Join-Path ([IO.Path]::GetTempPath()) ('explorer-uia-' + [guid]::NewGuid().ToString('N') + '.json')
    $start = New-Object Diagnostics.ProcessStartInfo
    $start.FileName = Join-Path ([Environment]::GetFolderPath('System')) 'WindowsPowerShell\v1.0\powershell.exe'
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
    $start.Arguments = '-NoProfile -NonInteractive -MTA -ExecutionPolicy Bypass -File "{0}" -WindowHandle {1} -MaxDepth {2} -MaxElements {3} -SnapshotWorker -WorkerOutput "{4}"' -f $PSCommandPath, $WindowHandle, $MaxDepth, $MaxElements, $output
    $child = [Diagnostics.Process]::Start($start)
    try {
        if (-not $child.WaitForExit($TimeoutSeconds * 1000)) {
            # Only our newly created inspection process is terminated.
            $child.Kill()
            $child.WaitForExit()
            throw 'UIA snapshot timed out; the owned inspector was stopped. Target was not terminated.'
        }
        if ($child.ExitCode -ne 0) { throw "UIA snapshot failed (inspector exit $($child.ExitCode))." }
        Write-Output $output
    } catch {
        if (Test-Path -LiteralPath $output) { Remove-Item -LiteralPath $output }
        throw
    } finally { $child.Dispose() }
    exit
}

$tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
$destination = [IO.Path]::GetFullPath($WorkerOutput)
if (-not $destination.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Snapshot output must be under TEMP.'
}
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName WindowsBase
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class SnapshotWindowValidation {
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsWindow(IntPtr hwnd);
}
'@
if (-not [SnapshotWindowValidation]::IsWindow([IntPtr]$WindowHandle)) {
    throw 'WindowHandle is not a live window; desktop fallback is forbidden.'
}
$root = [Windows.Automation.AutomationElement]::FromHandle([IntPtr]$WindowHandle)
if (-not $root) { throw 'WindowHandle does not identify an accessible live window.' }
$walker = [Windows.Automation.TreeWalker]::RawViewWalker
$nodes = New-Object 'Collections.Generic.List[object]'
$errors = New-Object 'Collections.Generic.List[string]'
$script:depthLimited = $false
$script:elementLimited = $false
function Read-Node($element, [int]$depth, [int]$parent) {
    if ($nodes.Count -ge $MaxElements) { $script:elementLimited = $true; return }
    try {
        $current = $element.Current
        $rect = $current.BoundingRectangle
        $index = $nodes.Count
        $nodes.Add([ordered]@{
            index=$index; parent=$parent; depth=$depth; processId=$current.ProcessId
            name=$current.Name; automationId=$current.AutomationId; className=$current.ClassName
            frameworkId=$current.FrameworkId; controlType=$current.ControlType.ProgrammaticName
            isOffscreen=$current.IsOffscreen; nativeWindowHandle=$current.NativeWindowHandle
            boundingRectangle=[ordered]@{ x=$rect.X; y=$rect.Y; width=$rect.Width; height=$rect.Height }
        })
        if ($depth -ge $MaxDepth) { $script:depthLimited = $true; return }
        $child = $walker.GetFirstChild($element)
        while ($child) {
            Read-Node $child ($depth + 1) $index
            if ($nodes.Count -ge $MaxElements) { $script:elementLimited = $true; break }
            $child = $walker.GetNextSibling($child)
        }
    } catch { $errors.Add("depth ${depth}: $($_.Exception.Message)") }
}
Read-Node $root 0 -1
if ($nodes.Count -eq 0) { throw 'UIA snapshot failed to read any elements.' }
[ordered]@{
    schema='explorer-uia-raw-v1'; capturedUtc=[DateTime]::UtcNow.ToString('o')
    windowHandle=$WindowHandle; tree='UI Automation raw accessibility view (not XAML visual tree)'
    coordinates='screen coordinates reported by UIA; not XAML layout units'
    maxDepth=$MaxDepth; maxElements=$MaxElements
    depthLimitReached=$script:depthLimited; elementLimitReached=$script:elementLimited
    errors=$errors.ToArray(); nodes=$nodes.ToArray()
} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $destination -Encoding UTF8
