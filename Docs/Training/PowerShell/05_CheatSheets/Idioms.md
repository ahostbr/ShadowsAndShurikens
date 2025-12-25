# Idioms Buddy must copy (don’t invent)

## Enumerate files
Get-ChildItem -LiteralPath $root -Recurse -File -Filter '*.log'

## Create dir
New-Item -ItemType Directory -Path $dir -Force | Out-Null

## Plan then execute
$plan = @()
$plan += [pscustomobject]@{ Op='Copy'; Src=$src; Dst=$dst }
foreach ($step in $plan) { if ($PSCmdlet.ShouldProcess($step.Dst, $step.Op)) { Copy-Item -LiteralPath $step.Src -Destination $step.Dst -Force } }

## JSON atomic
Write-JsonFileAtomic -Path $statePath -Object $obj

## Process exec
$r = Invoke-Process -FilePath $exe -Arguments @('arg1','arg2')
if ($r.ExitCode -ne 0) { throw "Failed: $($r.Cmd) $($r.Args)`n$($r.Stderr)" }
