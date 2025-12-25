# Windows Path Rules

- Always assume paths contain spaces.
- Prefer absolute paths; avoid relative unless explicitly anchored.
- Use canonicalization:
  - PowerShell: Resolve-Path -LiteralPath
  - Python: Path(...).resolve()
- Avoid wildcard operations on paths unless explicitly required.
- Watch for MAX_PATH pitfalls; keep temp paths short if possible.
