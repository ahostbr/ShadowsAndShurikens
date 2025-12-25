function Compress-ZipSafe {
  param([Parameter(Mandatory)][string]$SourceDir, [Parameter(Mandatory)][string]$ZipPath)
  $src = (Resolve-Path -LiteralPath $SourceDir).Path
  if (-not (Test-Path -LiteralPath $src)) { throw "SourceDir not found: $src" }
  $zipDir = Split-Path -Parent $ZipPath
  if (-not (Test-Path -LiteralPath $zipDir)) { New-Item -ItemType Directory -Path $zipDir -Force | Out-Null }
  if (Test-Path -LiteralPath $ZipPath) { Remove-Item -LiteralPath $ZipPath -Force }
  Compress-Archive -LiteralPath (Join-Path $src '*') -DestinationPath $ZipPath -Force
}

function Expand-ZipSafe {
  param([Parameter(Mandatory)][string]$ZipPath, [Parameter(Mandatory)][string]$DestDir)
  if (-not (Test-Path -LiteralPath $ZipPath)) { throw "Zip not found: $ZipPath" }
  if (-not (Test-Path -LiteralPath $DestDir)) { New-Item -ItemType Directory -Path $DestDir -Force | Out-Null }
  Expand-Archive -LiteralPath $ZipPath -DestinationPath $DestDir -Force
}
