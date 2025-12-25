# Exercise 01 — Plan → DryRun → Execute

Write a CLI that:
- takes --root and --dry-run
- produces a JSON plan list (stdout)
- on execute, performs a small safe action (e.g., create an output folder)

Requirements:
- pathlib everywhere
- no CWD assumptions
- stable exit codes
