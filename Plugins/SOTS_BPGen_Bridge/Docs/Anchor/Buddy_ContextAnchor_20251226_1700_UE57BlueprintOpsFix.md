Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1700 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: UE57 BlueprintOps Fix | Owner: Buddy
Scope: UE 5.7.1 compile unblock for BlueprintOps (EditorAssetLibrary include, MakeError overloads, reparent fallback)

DONE
- Added `#include "EditorAssetLibrary.h"` in `SOTS_BPGenBridgeBlueprintOps.cpp` for `UEditorAssetLibrary`.
- Added `MakeError(const TCHAR*, ...)` overloads in `SOTS_BPGenBridgeBlueprintOps.cpp` and `SOTS_BPGenBridgeAssetOps.cpp`.
- Replaced `FKismetEditorUtilities::ReparentBlueprint(BP, Resolved)` with direct `BP->ParentClass = Resolved` + `FBlueprintEditorUtils::{MarkBlueprintAsModified,RefreshAllNodes,RefreshVariables}` + `FKismetEditorUtilities::CompileBlueprint`.

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- Assumed `UEditorAssetLibrary` errors were due to missing include, not missing module linkage.
- Assumed VibeUE reparent pattern is acceptable replacement for missing UE 5.7 API.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeBlueprintOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeAssetOps.cpp

NEXT (Ryan)
- Full rebuild `ShadowsAndShurikensEditor`; confirm `SOTS_BPGenBridgeBlueprintOps.cpp` no longer errors on `UEditorAssetLibrary` and `ReparentBlueprint`.
- If any remaining `MakeError` collisions exist elsewhere, apply same overload pattern or rename helper away from `MakeError`.

ROLLBACK
- Revert the touched files above (git revert or restore to previous versions).
