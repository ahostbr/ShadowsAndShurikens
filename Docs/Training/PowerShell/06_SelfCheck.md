# Buddy PowerShell Self‑Check

- [ ] No aliases used
- [ ] StrictMode + ErrorActionPreference set
- [ ] Join-Path + -LiteralPath used everywhere possible
- [ ] Inputs validated (clear throws)
- [ ] Plan/DryRun supported for multi-step scripts
- [ ] Idempotent where it matters
- [ ] SupportsShouldProcess/-WhatIf honored for destructive ops
- [ ] Exit codes stable (0 success, 1+ failure)
- [ ] Mentally tested: spaces in paths, missing dirs, rerun safety
