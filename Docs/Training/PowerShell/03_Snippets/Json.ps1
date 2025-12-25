function Read-JsonFile {
  param([Parameter(Mandatory)][string]$Path)
  if (-not (Test-Path -LiteralPath $Path)) { return $null }
  $raw = Get-Content -LiteralPath $Path -Raw -Encoding UTF8
  if ([string]::IsNullOrWhiteSpace($raw)) { return $null }
  $raw | ConvertFrom-Json -Depth 100
}

function Write-JsonFileAtomic {
  param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)]$Object)
  $dir = Split-Path -Parent $Path
  if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
  $tmp = Join-Path $dir ("tmp_" + [guid]::NewGuid().ToString('N') + ".json")
  $json = $Object | ConvertTo-Json -Depth 100
  Set-Content -LiteralPath $tmp -Value $json -Encoding UTF8 -NoNewline
  Move-Item -LiteralPath $tmp -Destination $Path -Force
}
