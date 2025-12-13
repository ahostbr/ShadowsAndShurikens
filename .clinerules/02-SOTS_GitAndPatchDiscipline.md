# SOTS Git + Patch Discipline

## Default Execution Pattern
1. `git status`
2. Identify relevant files (ripgrep/search)
3. Propose unified diff
4. Apply patch
5. `git diff --stat`
6. Summarize changes + any follow-up commands

## Patch Rules
- Do not partially apply patches. If a patch fails, regenerate it.
- Avoid whitespace churn unless required (keep diffs readable).
- Do not reformat entire files “just because.”

## Validation (Lightweight)
Use low-risk validation unless explicitly requested:
- `git diff --stat`
- targeted search/grep checks
- optional: run DevTools scripts (only if asked and they exist in DevTools/)

## Escalation
If the task touches:
- build scripts (.uplugin / Build.cs),
- dependencies,
- or large-scale refactors,
STOP and ask for explicit approval.