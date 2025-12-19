# Buddy Worklog — BlueprintCommentLinks Panel-Local Handles

**Date:** 2025-12-14
**Goal:** Restore visible/usable Blueprint Comment Links handles in UE5.7 by fixing coordinate space drift.

**Changes:**
- Switched handle/link coordinate outputs to panel-local space and converted node-local positions via the graph panel to avoid absolute transform drift; keeps handles onscreen at 0.5 zoom [Plugins/BlueprintCommentLinks/Source/BlueprintCommentLinksEditor/Private/SCommentLinkOverlay.cpp#L708-L734](Plugins/BlueprintCommentLinks/Source/BlueprintCommentLinksEditor/Private/SCommentLinkOverlay.cpp#L708-L734) [Plugins/BlueprintCommentLinks/Source/BlueprintCommentLinksEditor/Private/SCommentLinkOverlay.cpp#L821-L847](Plugins/BlueprintCommentLinks/Source/BlueprintCommentLinksEditor/Private/SCommentLinkOverlay.cpp#L821-L847) [Plugins/BlueprintCommentLinks/Source/BlueprintCommentLinksEditor/Private/SCommentLinkOverlay.cpp#L874-L890](Plugins/BlueprintCommentLinks/Source/BlueprintCommentLinksEditor/Private/SCommentLinkOverlay.cpp#L874-L890).

**Files touched:**
- Plugins/BlueprintCommentLinks/Source/BlueprintCommentLinksEditor/Private/SCommentLinkOverlay.cpp

**Verification:**
- Ran in-editor: toggled Comment Link Mode; handles now visible and interactive on the tested blueprint at zoom 0.5. Logs show total=3 drawn=3 zero=0 with expected positions.

**Cleanup:**
- Deleted Binaries/Intermediate for BlueprintCommentLinks after edits (per policy).

**Follow-ups:**
- None requested; reopen if any other zoom/viewport offsets surface.
