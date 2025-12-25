#Requires -Version 7.0
<#
.SYNOPSIS
  SOTS PowerShell script template.

.DESCRIPTION
  Strict mode, plan/dry-run, safe file ops, stable exit codes.
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

[CmdletBinding(SupportsShouldProcess = $true)]
param(
  [Parameter(Mandatory = $true)]
  [string]$Root,

  [switch]$DryRun
)

function Write-Log {
  param(
    [Parameter(Mandatory)][string]$Message,
    [ValidateSet('INFO','WARN','ERROR')][string]$Level = 'INFO'
  )
  $ts = (Get-Date).ToString('yyyy-MM-dd HH:mm:ss')
  Write-Host "[$ts][$Level] $Message"
}

function Assert-PathExists {
  param([Parameter(Mandatory)][string]$Path, [string]$Label = 'Path')
  if (-not (Test-Path -LiteralPath $Path)) { throw "$Label does not exist: $Path" }
}

function Invoke-Retry {
  param([Parameter(Mandatory)][scriptblock]$Action, [int]$MaxAttempts = 5, [int]$DelayMs = 200)
  $attempt = 0
  while ($true) {
    $attempt++
    try { return & $Action }
    catch {
      if ($attempt -ge $MaxAttempts) { throw }
      Start-Sleep -Milliseconds ($DelayMs * $attempt)
    }
  }
}

try {
  Assert-PathExists -Path $Root -Label 'Root'
  $rootFull = (Resolve-Path -LiteralPath $Root).Path

  # BUILD PLAN
  $plan = @()
  $plan += [pscustomobject]@{ Op='Example'; Path=$rootFull; Note='Replace with real ops' }

  if ($DryRun) {
    Write-Log "DRY RUN — Plan:" 'INFO'
    $plan | Format-Table -AutoSize | Out-String | Write-Host
    exit 0
  }

  # EXECUTE
  foreach ($step in $plan) {
    if ($PSCmdlet.ShouldProcess($step.Path, $step.Op)) {
      Write-Log "$($step.Op): $($step.Path)" 'INFO'
      # TODO: do work
    }
  }

  Write-Log "Done." 'INFO'
  exit 0
}
catch {
  Write-Log $_.Exception.Message 'ERROR'
  exit 1
}
