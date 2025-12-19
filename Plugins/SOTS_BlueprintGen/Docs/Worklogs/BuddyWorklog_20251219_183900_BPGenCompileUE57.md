# Buddy Worklog — BPGen UE5.7 compile fixes
- goal: clear UE5.7 compile errors (EdGraphSchema include, ImportText owner constness, FunctionEntry flag access, widget binding ObjectName type).
- what changed:
  - switched EdGraphSchema include in GraphResolver to the UE5.7 header path.
  - updated ImportText_Direct to pass a non-const owner and set pure/const flags via reflected ExtraFlags property instead of direct member access (warning if property missing).
  - adjusted widget binding creation to write ObjectName as FString to match updated delegate binding API.
- files changed: Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenGraphResolver.cpp; Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenEnsure.cpp.
- notes/risks/unknowns: ExtraFlags reflection assumes the property still exists; if removed, pure/const flags will stay default (warning emitted). No build/run yet; further UE5.7 API drift possible.
- verification status: not built or run.
- follow-ups / next steps: Ryan re-run UBT; if ExtraFlags warning appears or new compile errors surface, share log and adjust; verify ensure-generated functions still honor pure/const and widget bindings create/update correctly.
