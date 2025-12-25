function Assert-True {
  param([Parameter(Mandatory)][bool]$Condition, [Parameter(Mandatory)][string]$Message)
  if (-not $Condition) { throw $Message }
}

function Assert-PathExists {
  param([Parameter(Mandatory)][string]$Path, [string]$Label = 'Path')
  if (-not (Test-Path -LiteralPath $Path)) { throw "$Label does not exist: $Path" }
}

function Resolve-FullPath {
  param([Parameter(Mandatory)][string]$Path)
  (Resolve-Path -LiteralPath $Path).Path
}
