# SOTS Preflight (Read-Only)

Run this workflow at the start of a session to confirm environment + scope.

## Steps
1. Run `git status`
2. Print current working directory
3. List repo root contents (top-level folders/files)
4. Confirm `.clinerules/` is present
5. Confirm you will only write to `Plugins/`, `DevTools/`, or `.clinerules/`

## Constraints
- Do not modify any files.
- Do not run builds.
- Do not create artifacts.