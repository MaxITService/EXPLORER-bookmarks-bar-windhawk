param([Parameter(Mandatory)][string]$OutputFile)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $projectRoot 'mods\explorer-folder-bookmarks-bar.wh.cpp'
$sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
$original = [IO.File]::ReadAllText($source)
$outputAbsolute = [IO.Path]::GetFullPath($OutputFile)
$tempAbsolute = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
if (-not $outputAbsolute.StartsWith($tempAbsolute + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The instrumented test copy must be written under TEMP.'
}

# Observe the same guarded delegates after their real registration. Includes
# occupy existing blank lines; captures and observers share original physical
# lines, preserving production line numbers and branch conditions.
$newline = if ($original.Contains("`r`n")) { "`r`n" } else { "`n" }
$edits = @(
    @{ Before = $newline + $newline + 'template <typename Element, typename Handler>' + $newline + 'void TrackClick(';
       After = $newline + '#include "event-observer.h"' + $newline + 'template <typename Element, typename Handler>' + $newline + 'void TrackClick(' },
    @{ Before = '    auto token = element.Click(' + $newline + '        GuardHandler(L"Click handler", std::forward<Handler>(handler)));';
       After = '    mux::RoutedEventHandler observedClick{nullptr}; auto token = element.Click(' + $newline + '        CaptureObserved(observedClick, GuardHandler(L"Click handler", std::forward<Handler>(handler)))); ObserveEvent(element, L"Click", observedClick);' },
    @{ Before = '    auto token = element.ContextRequested(GuardHandler(' + $newline + '        L"ContextRequested handler", std::forward<Handler>(handler)));';
       After = '    event_observer::Context observedContext{nullptr}; auto token = element.ContextRequested(CaptureObserved(observedContext, GuardHandler(' + $newline + '        L"ContextRequested handler", std::forward<Handler>(handler)))); ObserveEvent(element, L"ContextRequested", observedContext);' },
    @{ Before = '    auto token = menu.Opening(' + $newline + '        GuardHandler(L"Opening handler", std::forward<Handler>(handler)));';
       After = '    event_observer::Opening observedOpening{nullptr}; auto token = menu.Opening(' + $newline + '        CaptureObserved(observedOpening, GuardHandler(L"Opening handler", std::forward<Handler>(handler)))); ObserveEvent(menu, L"Opening", observedOpening);' },
    @{ Before = $newline + $newline + 'void TrackPointer(';
       After = $newline + '#include "pointer-observer.h"' + $newline + 'void TrackPointer(' },
    @{ Before = '    button.AddHandler(event, boxed, true);';
       After = '    button.AddHandler(event, boxed, true); ObservePointer(button, event, boxed);' },
    @{ Before = '                    element.CapturePointer(args.Pointer());';
       After = '                    pointer_observer::CapturePointer(element, args.Pointer());' }
)
# These callbacks register directly on the real root. Capture each complete,
# uniquely anchored block without adding physical lines or moving its lambda.
foreach ($event in @(
    @{ Token = 'rootPointerToken'; Name = 'PointerEntered'; Type = 'muxi::PointerEventHandler' },
    @{ Token = 'rootDragOverToken'; Name = 'DragOver'; Type = 'mux::DragEventHandler' },
    @{ Token = 'rootDragLeaveToken'; Name = 'DragLeave'; Type = 'mux::DragEventHandler' },
    @{ Token = 'rootDropToken'; Name = 'Drop'; Type = 'mux::DragEventHandler' }
)) {
    $start = '    state->' + $event.Token + ' = barRoot.' + $event.Name + '(GuardHandler('
    $pattern = [regex]::Escape($start) + '[\s\S]*?' + [regex]::Escape('        }));')
    $blocks = [regex]::Matches($original, $pattern)
    if ($blocks.Count -ne 1) { throw 'Root event test-copy anchor is not unique.' }
    $before = $blocks[0].Value
    $variable = 'observed' + $event.Name
    $after = $before.Replace($start, '    ' + $event.Type + ' ' + $variable + '{nullptr}; ' +
        $start.TrimStart().Replace('(GuardHandler(', '(CaptureObserved(' + $variable + ', GuardHandler('))
    $after = $after.Substring(0, $after.Length - '        }));'.Length) +
        '        }))); ObserveEvent(barRoot, L"' + $event.Name + '", ' + $variable + ');'
    $edits += @{ Before = $before; After = $after }
}
$copy = $original
foreach ($edit in $edits) {
    $matches = [regex]::Matches($original, [regex]::Escape($edit.Before))
    if ($matches.Count -ne 1) { throw 'Test-copy anchor is not unique.' }
    $copy = $copy.Replace($edit.Before, $edit.After)
}
$restored = $copy
foreach ($edit in $edits) { $restored = $restored.Replace($edit.After, $edit.Before) }
if (-not [string]::Equals($restored, $original, [StringComparison]::Ordinal) -or
    [regex]::Matches($copy, "`n").Count -ne [regex]::Matches($original, "`n").Count -or
    (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $sourceHash) {
    throw 'Production source verification failed while preparing the test copy.'
}
[IO.File]::WriteAllText($outputAbsolute, $copy,
                       (New-Object Text.UTF8Encoding $false))
