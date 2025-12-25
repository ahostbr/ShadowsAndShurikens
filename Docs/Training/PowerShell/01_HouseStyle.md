# SOTS PowerShell House Style (MANDATORY)

## Non‑negotiable defaults
- Target PowerShell 7+ unless explicitly told “5.1 only”.
- No aliases in shipped scripts (NO: gci, ls, %, ?, ni, rm, cp, mv).
- Always set:
  - Set-StrictMode -Version Latest
  - $ErrorActionPreference = 'Stop'
- Always use Join-Path; always use -LiteralPath where possible.
- Never depend on current working directory (resolve paths explicitly).
- Validate inputs and throw actionable errors.
- Prefer CmdletBinding + SupportsShouldProcess for scripts that modify state.

## Output rules
- Tool-facing output: objects or JSON (not log-parsed text).
- Human logs: Write-Log with INFO/WARN/ERROR.

## Safety rules
- Avoid wildcard deletes unless explicitly required.
- For destructive ops: support -WhatIf via SupportsShouldProcess and honor it.
