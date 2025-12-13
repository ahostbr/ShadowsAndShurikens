# SOTS Cline Laws

These rules are non-negotiable for any automated edits in this repository.

## Scope
- Allowed write locations:
  - `Plugins/**`
  - `DevTools/**`
  - `.clinerules/**`
- Never write to:
  - `Content/**`
  - `Saved/**`
  - `DerivedDataCache/**`
  - `Binaries/**`
  - `Intermediate/**`

## Change Discipline
- Make changes only via **unified diff/patch**.
- Prefer small, reviewable patches.
- If a task requires broad changes, break it into multiple passes.

## Hard Stop Conditions (Ask Before Proceeding)
Stop and ask before you:
- Modify any `.uplugin`
- Modify any `*.Build.cs`
- Change module dependencies / loading phases
- Rename/move/delete files or folders
- Touch more than 25 files in a single run

## Git Safety
- Always start with `git status`.
- If the working tree is not clean, create a safety point:
  - `git add -A`
  - `git commit -m "pre-agent snapshot"`
- After applying patches, show:
  - `git diff --stat`

## Unreal Specific Hygiene
- Do not generate or commit build artifacts.
- Never edit anything under plugin `Binaries/` or `Intermediate/`.
- Prefer forward declarations where appropriate; keep includes minimal and correct.

## Communication Style
- Restate the goal briefly.
- List the exact files you will touch before applying the patch.
- After patch, summarize “what changed” and “why,” plus suggested next checks.