# SOTS Python House Style (MANDATORY)

## Non‑negotiable defaults
- Target Python 3.12+.
- Prefer stdlib. If adding deps, justify and pin.
- Use pathlib.Path everywhere (no string path joins).
- Never rely on CWD; resolve paths explicitly.
- No silent except: always rethrow with context.
- Tool outputs must be structured (dict/JSON), not “print soup”.
- Write state files atomically (tmp + replace).
- Plan → DryRun → Execute for multi-step operations.
- Stable exit codes: 0 success, 1+ failure.

## Logging
- Console: concise human logs.
- File: JSONL log for tools (optional but recommended).

## Subprocess
- Always capture stdout/stderr.
- Never assume exit code 0.
- Include command + stderr in errors.
