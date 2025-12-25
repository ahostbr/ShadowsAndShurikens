# Exercise 02 — JSON: Atomic state file

Write functions:
- Read-State(Path) returns object or null
- Write-State(Path,obj) writes atomically (tmp + rename)

Then script:
- increments counter in state.json
- prints updated value as JSON
