# Exercise 01 — Filesystem: Plan → DryRun → Execute

Write a script that:
- Takes -Root and -Dest
- Plans “copy .log files” (preserve relative paths)
- If -DryRun: prints plan and exits 0
- Else: executes copy

Requirements:
- Join-Path + -LiteralPath everywhere
- No aliases
- Idempotent (safe to rerun)
