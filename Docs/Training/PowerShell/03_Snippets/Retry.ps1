function Invoke-Retry {
  param(
    [Parameter(Mandatory)][scriptblock]$Action,
    [int]$MaxAttempts = 5,
    [int]$DelayMs = 200
  )
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
