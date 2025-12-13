# SOTS Safe Patch (Diff-First)

Use this workflow when you are about to make edits.

## Steps
1. Run `git status`
2. If dirty:
   - `git add -A`
   - `git commit -m "pre-agent snapshot"`
3. Identify files via ripgrep/search
4. Propose a unified diff for review
5. Apply the unified diff
6. Run `git diff --stat`
7. Summarize:
   - files changed
   - why each change was made
   - next recommended checks

## Hard Stops
Stop and ask before:
- `.uplugin` changes
- `*.Build.cs` changes
- dependency changes
- renames/moves/deletes
- >25 files changed