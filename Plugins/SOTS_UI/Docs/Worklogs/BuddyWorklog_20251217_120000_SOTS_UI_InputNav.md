# Buddy Worklog — SOTS_UI Input Nav Layer

**Goal**: Wire SOTS_UI to push/pop the SOTS_Input UI navigation layer per the SPINE_6 integration contract and add the required dependency.

**Changes**
- Added SOTS_Input as a module and plugin dependency for SOTS_UI so the stable input API is available.
- UIRouter now toggles the `Input.Layer.UI.Nav` gameplay tag when UI layers are active (driven by the top entry input policy) using `USOTS_InputBlueprintLibrary`, while preserving existing input mode logic.
- Cleans up UI nav state on subsystem teardown to avoid leaked layers.

**Files touched**
- Plugins/SOTS_UI/Source/SOTS_UI/SOTS_UI.Build.cs
- Plugins/SOTS_UI/SOTS_UI.uplugin
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp

**Verification**
- Not run (code-only change; no build executed per SOTS law).

**Cleanup**
- Deleted Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate after edits.

**Follow-ups**
- Still need to wire gameplay-side layers: baseline `Input.Layer.Ninja.Default`, dragon control push/pop, and cutscene exclusivity per the integration contract (identify correct gameplay owner and hook points).
