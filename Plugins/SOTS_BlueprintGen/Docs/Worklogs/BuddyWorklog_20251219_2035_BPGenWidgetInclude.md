# Buddy Worklog — BPGen Widget include unblock
- goal: fix remaining compile blocker C1083 on Blueprint/WidgetBlueprint.h from 13:26 log.
- what changed: removed direct WidgetBlueprint include in ensure helpers and forward-declared UWidgetBlueprint (header not on include path in UE5.7 engine layout).
- files changed: Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenEnsure.cpp.
- notes/risks/unknowns: assumes WidgetTree/Widget usage is sufficient; relies on forward declaration only. No build yet.
- verification status: not built or run.
- follow-ups/next steps: rerun UBT; if new missing symbols arise for UWidgetBlueprint, consider swapping to UWidgetBlueprintFwd.h or adjust module paths.
