function Invoke-Process {
  param(
    [Parameter(Mandatory)][string]$FilePath,
    [string[]]$Arguments = @(),
    [string]$WorkingDirectory = $null,
    [int]$TimeoutSeconds = 0
  )

  if (-not (Test-Path -LiteralPath $FilePath)) { throw "Exe not found: $FilePath" }

  $psi = [System.Diagnostics.ProcessStartInfo]::new()
  $psi.FileName = $FilePath
  foreach ($a in $Arguments) { [void]$psi.ArgumentList.Add($a) }
  if ($WorkingDirectory) { $psi.WorkingDirectory = $WorkingDirectory }
  $psi.RedirectStandardOutput = $true
  $psi.RedirectStandardError = $true
  $psi.UseShellExecute = $false
  $psi.CreateNoWindow = $true

  $p = [System.Diagnostics.Process]::new()
  $p.StartInfo = $psi
  [void]$p.Start()

  if ($TimeoutSeconds -gt 0) {
    if (-not $p.WaitForExit($TimeoutSeconds * 1000)) {
      try { $p.Kill($true) } catch {}
      throw "Process timed out: $FilePath"
    }
  } else {
    $p.WaitForExit()
  }

  $stdout = $p.StandardOutput.ReadToEnd()
  $stderr = $p.StandardError.ReadToEnd()
  [pscustomobject]@{
    ExitCode = $p.ExitCode
    Stdout   = $stdout
    Stderr   = $stderr
    Cmd      = $FilePath
    Args     = ($Arguments -join ' ')
  }
}
