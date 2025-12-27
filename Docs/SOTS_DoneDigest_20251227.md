# SOTS New Docs — “What’s Done” Digest (Dec 26–27, 2025)

This digest was generated from the two uploaded zip bundles:

- `SOTS_new_docs_20251226_153200.zip`
- `SOTS_new_docs_20251227_073505.zip`

**Totals:** 84 markdown files (35 worklogs, 26 context anchors, 23 reference docs).

## Big-ticket completed items captured by docs/worklogs

- **SOTS_Core BRIDGE series is documented end-to-end (BRIDGE1–20)**, including the “golden switch flip” enablement order and a BridgeHealth diagnostics command.

- **Build-log driven fixes** were applied for: missing/incorrect includes, macro guard, `.uplugin` dependency declarations, and **LNK1120 unresolved externals** caused by missing `DeveloperSettings` module dependencies.

- **DevTools plugin dependency health** dry-run confirmed config coverage across the SOTS plugin set (run-only, no repo mutations).

- **BlueprintAssist UE 5.7 compatibility**: multiple incremental worklogs + context anchors (metadata fix, version macros, SetPurity link fix).

- **SOTS_Backup (restic) + GUI docs**: restic setup/notes + GUI behaviors and recent fixes.

- **UE Engine Snapshot RAG**: documented how to build and use deterministic engine-source snapshot indexes.

## Index by area / plugin

### SuiteTools

**Reference docs:**
- `Docs/SOTS_Backup_GUI.md`
- `Docs/SOTS_Backup_Restic.md`
- `Docs/SOTS_RAG_UE_ENGINE_SNAPSHOTS.md`

**Worklogs:**
- `Docs/Worklogs/BuddyWorklog_20251225_223520_SnapshotAutoTags.md`
- `Docs/Worklogs/BuddyWorklog_20251226_081700_SOTSBackupGUI_ResticResolution.md`
- `Docs/Worklogs/BuddyWorklog_20251226_082200_SOTSBackupGUI_AutoDetectRestic.md`

### SOTS_Core

**Worklog highlights:**

- Buddy Worklog — SOTS_Core — BRIDGE4 Enablement Doc — Lists the participating settings sections/keys for Core + bridged plugins.; Lists authoritative SOTS_Core diagnostics console command names.
- Buddy Worklog — 2025-12-26 08:44:33 — LifecycleHandleExport — Exported `FSOTS_CoreLifecycleListenerHandle` from the `SOTS_Core` module by adding the `SOTS_CORE_API` macro to the class declaration.
- Buddy Worklog — 2025-12-26 10:35 — SOTS_Core BRIDGE5 (BridgeHealth diagnostics) — Added a new diagnostics entrypoint `FSOTS_CoreDiagnostics::DumpBridgeHealth(UWorld*)`.; Added a new console command `SOTS.Core.BridgeHealth` registered alongside existing SPINE7 commands.
- Buddy Worklog — 20251226_153050 — SOTS_Core BRIDGE1–20 Summary Completion — Added this summary worklog in `SOTS_Core` to consolidate completion status for BRIDGE1–20.; Verified `Binaries/` + `Intermediate/` folder presence for the BRIDGE1–20 plugin set.
- Buddy Worklog — 20251226_153422 — Log-driven build fixes (BRIDGE series follow-up) — Replaced `#if PLATFORM_COMPILER_HAS_RTTI` with `#if defined(PLATFORM_COMPILER_HAS_RTTI) && PLATFORM_COMPILER_HAS_RTTI` to avoid `C4668` when the macro is not defined.; Updated include to the correct public header path: `Lifecycle/SOTS_CoreLifecycleListener.h`.
- Buddy Worklog — 20251226_200319 — Log: LNK1120 UDeveloperSettings unresolved externals — Add-only fix: added `"DeveloperSettings"` to `PublicDependencyModuleNames` in the affected plugin `*.Build.cs` files.
- Buddy Worklog — 2025-12-27 01:07:00 — DevTools PluginDepHealth All SOTS — No repo files changed in this step.; Ran the health check in `--dry-run` mode after expanding the config previously, to validate coverage.

**Reference docs:**
- `Plugins/SOTS_Core/Docs/SOTS_Core_Bridge_Enablement_Order.md`
- `Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md`

**Worklogs:**
- `Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_000000_BRIDGE4_EnablementDoc.md`
- `Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_084433_LifecycleHandleExport.md`
- `Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_103500_SOTS_Core_BRIDGE5_BridgeHealth.md`
- `Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_153050_BRIDGE1-20_SummaryCompletion.md`
- `Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_153422_LogDrivenBuildFixes_BRIDGE_Series.md`
- `Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_200319_Log_LNK1120_DeveloperSettingsDeps.md`
- `Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251227_010700_DevTools_PluginDepHealth_AllSOTS.md`

**Context anchors:**
- `Plugins/SOTS_Core/Docs/Anchor/Buddy_ContextAnchor_20251226_0000_BRIDGE4_EnablementDoc.md`
- `Plugins/SOTS_Core/Docs/Anchor/Buddy_ContextAnchor_20251226_0844_LifecycleHandleExport.md`
- `Plugins/SOTS_Core/Docs/Anchor/Buddy_ContextAnchor_20251226_1035_BRIDGE5_BridgeHealth.md`

### BlueprintAssist

**Worklog highlights:**

- Buddy Worklog — BlueprintAssist UE 5.7 compat patch — Added a forward declaration for `UMetaData` in the public utils header (it is only used as a pointer type there).; Updated `SGraphNode::MoveTo` call sites to pass `FVector2f` (UE 5.7 signature) instead of `FVector2D`.
- Buddy Worklog — 2025-12-26 08:52:28 — BlueprintAssist UE5.7 MetaData Fix — Updated `BlueprintAssistCache.cpp` package-metadata read/write helpers to use `FMetaData` on UE 5.7+ (and keep the `UMetaData*` path for older engines) via `BA_UE_VERSION_OR_LATER(5, 7)` guards.; Updated `FBAUtils::GetNodeMetaData` and its single call site (`IsExtraRootNode`) to use `FMetaData` on UE 5.7+.
- Buddy Worklog — 2025-12-26 08:55:10 — BlueprintAssist UE5.7 MetaData Fix (Follow-up) — Fixed `FMetaData` forward declaration to match engine (`class FMetaData;`), eliminating MSVC C4099.; Removed reliance on `BA_UE_VERSION_OR_LATER` in the public header (it was undefined during compilation). Replaced with a robust engine-version check using `ENGINE_MAJOR_VERSION/ENGINE_MINOR_VERSION`, emitting `BA_UE_57_OR_LATER`.
- Buddy Worklog — 2025-12-26 09:02:29 — BlueprintAssist UE Version Macros Fix — Included `Runtime/Launch/Resources/Version.h` in `BlueprintAssistUtils.h` so the engine version macros used by the UE5.7 compatibility guard are defined.; Plugins/BlueprintAssist/Source/BlueprintAssist/Public/BlueprintAssistUtils.h
- Buddy Worklog — 2025-12-26 09:16 — BlueprintAssist SetPurity link fix — Updated BlueprintAssist node-purity toggle to avoid calling `UK2Node_VariableGet::SetPurity(bool)` on UE 5.7+.; Implemented a UE 5.7+ path that sets `UK2Node_VariableGet`'s internal `CurrentVariation` via reflection and then calls `ReconstructNode()` (mirrors engine `TogglePurity()` behavior without relying on a non-exported symbol).

**Worklogs:**
- `Plugins/BlueprintAssist/Docs/Worklogs/BuddyWorklog_20251226_075327_BlueprintAssist_UE57Compat.md`
- `Plugins/BlueprintAssist/Docs/Worklogs/BuddyWorklog_20251226_085228_BlueprintAssist_UE57MetaDataFix.md`
- `Plugins/BlueprintAssist/Docs/Worklogs/BuddyWorklog_20251226_085510_BlueprintAssist_UE57MetaDataFixFollowup.md`
- `Plugins/BlueprintAssist/Docs/Worklogs/BuddyWorklog_20251226_090229_BlueprintAssist_UEVersionMacrosFix.md`
- `Plugins/BlueprintAssist/Docs/Worklogs/BuddyWorklog_20251226_091600_BlueprintAssist_SetPurityLinkFix.md`

**Context anchors:**
- `Plugins/BlueprintAssist/Docs/Anchor/Buddy_ContextAnchor_20251226_0753_BlueprintAssistUE57.md`
- `Plugins/BlueprintAssist/Docs/Anchor/Buddy_ContextAnchor_20251226_0852_BlueprintAssist_UE57MetaDataFix.md`
- `Plugins/BlueprintAssist/Docs/Anchor/Buddy_ContextAnchor_20251226_0855_BlueprintAssist_UE57MetaDataFixFollowup.md`
- `Plugins/BlueprintAssist/Docs/Anchor/Buddy_ContextAnchor_20251226_0902_BlueprintAssist_UEVersionMacrosFix.md`
- `Plugins/BlueprintAssist/Docs/Anchor/Buddy_ContextAnchor_20251226_0916_BlueprintAssist_SetPurityLinkFix.md`

### LightProbePlugin

**Worklog highlights:**

- Buddy Worklog — 2025-12-26 15:10 — BRIDGE16 LightProbePlugin Core Lifecycle Bridge — Added `ULightProbeCoreBridgeSettings` with `bEnableSOTSCoreLifecycleBridge=false` and `bEnableSOTSCoreBridgeVerboseLogs=false`.; `OnCoreWorldReady_Native`

**Reference docs:**
- `Plugins/LightProbePlugin/Docs/LightProbe_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/LightProbePlugin/Docs/Worklogs/BuddyWorklog_20251226_151000_BRIDGE16_LightProbe_CoreLifecycleBridge.md`

**Context anchors:**
- `Plugins/LightProbePlugin/Docs/Anchor/Buddy_ContextAnchor_20251226_1510_BRIDGE16_LightProbe_CoreLifecycleBridge.md`

### OmniTrace

**Worklog highlights:**

- BRIDGE19 — OmniTrace ↔ SOTS_Core Lifecycle Bridge

**Reference docs:**
- `Plugins/OmniTrace/Docs/OmniTrace_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/OmniTrace/Docs/Worklogs/BuddyWorklog_20251226_151149_BRIDGE19_OmniTrace_CoreBridge.md`

**Context anchors:**
- `Plugins/OmniTrace/Docs/Anchor/Buddy_ContextAnchor_20251226_1511_BRIDGE19_OmniTrace_CoreBridge.md`

### SOTS_AIPerception

**Worklog highlights:**

- Buddy Worklog — 2025-12-26 13:48 — SOTS_AIPerception BRIDGE11 (Core lifecycle bridge, state-only) — Added `SOTS_Core` as a dependency for SOTS_AIPerception (Build.cs + .uplugin plugin dependency).; `USOTS_AIPerceptionCoreBridgeSettings::bEnableSOTSCoreLifecycleBridge`

**Reference docs:**
- `Plugins/SOTS_AIPerception/Docs/SOTS_AIPerception_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/SOTS_AIPerception/Docs/Worklogs/BuddyWorklog_20251226_134800_BRIDGE11_SOTS_AIPerception_CoreLifecycleBridge.md`

**Context anchors:**
- `Plugins/SOTS_AIPerception/Docs/Anchor/Buddy_ContextAnchor_20251226_1348_BRIDGE11_SOTS_AIPerception_CoreBridge.md`

### SOTS_BPGen_Bridge

**Worklog highlights:**

- Buddy Worklog — BPGen Bridge Unity helper de-dup — Centralized previously duplicated helper implementations into a single private header with `inline` helpers in a dedicated namespace.; Updated bridge ops translation units to include the shared helpers header and call the helpers via the namespace.

**Worklogs:**
- `Plugins/SOTS_BPGen_Bridge/Docs/Worklogs/BuddyWorklog_20251226_075327_BPGenBridge_UnityHelperDedup.md`

**Context anchors:**
- `Plugins/SOTS_BPGen_Bridge/Docs/Anchor/Buddy_ContextAnchor_20251226_0753_BPGenBridgeUnityHelpers.md`

### SOTS_Debug

**Worklog highlights:**

- BRIDGE20 — SOTS_Debug ↔ SOTS_Core Lifecycle Bridge

**Reference docs:**
- `Plugins/SOTS_Debug/Docs/SOTS_Debug_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/SOTS_Debug/Docs/Worklogs/BuddyWorklog_20251226_151712_BRIDGE20_SOTS_Debug_CoreBridge.md`

**Context anchors:**
- `Plugins/SOTS_Debug/Docs/Anchor/Buddy_ContextAnchor_20251226_1517_BRIDGE20_SOTS_Debug_CoreBridge.md`

### SOTS_FX_Plugin

**Worklog highlights:**

- Buddy Worklog — 20251226_123900 — BRIDGE13_FX_CoreLifecycleBridge — Added `SOTS_Core` module dependency and plugin dependency for `SOTS_FX_Plugin`.; Added `USOTS_FXCoreBridgeSettings` developer settings (default OFF) to gate registration and verbose logging.

**Reference docs:**
- `Plugins/SOTS_FX_Plugin/Docs/SOTS_FX_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/SOTS_FX_Plugin/Docs/Worklogs/BuddyWorklog_20251226_123900_BRIDGE13_FX_CoreLifecycleBridge.md`

**Context anchors:**
- `Plugins/SOTS_FX_Plugin/Docs/Anchor/Buddy_ContextAnchor_20251226_1239_BRIDGE13_FX_CoreBridge.md`

### SOTS_GAS_Plugin

**Worklog highlights:**

- Buddy Worklog — 2025-12-26 — BRIDGE18 — SOTS_GAS_Plugin ↔ SOTS_Core — lifecycle listener registration; verbose bridge logs

**Reference docs:**
- `Plugins/SOTS_GAS_Plugin/Docs/SOTS_GAS_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/SOTS_GAS_Plugin/Docs/Worklogs/BuddyWorklog_20251226_150100_BRIDGE18_GAS_CoreBridge.md`

**Context anchors:**
- `Plugins/SOTS_GAS_Plugin/Docs/Anchor/Buddy_ContextAnchor_20251226_1501_BRIDGE18_GAS_CoreBridge.md`

### SOTS_GlobalStealthManager

**Worklog highlights:**

- Buddy Worklog — 20251226_122600 — BRIDGE12_GSM_CoreLifecycleBridge — Added `SOTS_Core` module dependency and plugin dependency for `SOTS_GlobalStealthManager`.; Added `USOTS_GlobalStealthManagerCoreBridgeSettings` developer settings (default OFF) to gate registration and verbose logging.

**Reference docs:**
- `Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/SOTS_GlobalStealthManager/Docs/Worklogs/BuddyWorklog_20251226_122600_BRIDGE12_GSM_CoreLifecycleBridge.md`

**Context anchors:**
- `Plugins/SOTS_GlobalStealthManager/Docs/Anchor/Buddy_ContextAnchor_20251226_1226_BRIDGE12_GSM_CoreBridge.md`

### SOTS_INV

**Worklog highlights:**

- Buddy Worklog — 20251226_144500 — BRIDGE14 INV Save Contract Bridge — Added SOTS_Core dependency wiring to SOTS_INV.; Added `USOTS_INVCoreBridgeSettings` (default OFF) to gate bridge enablement and verbose logs.

**Reference docs:**
- `Plugins/SOTS_INV/Docs/SOTS_INV_SOTSCoreSaveContractBridge.md`

**Worklogs:**
- `Plugins/SOTS_INV/Docs/Worklogs/BuddyWorklog_20251226_144500_BRIDGE14_INV_SaveContractBridge.md`

**Context anchors:**
- `Plugins/SOTS_INV/Docs/Anchor/Buddy_ContextAnchor_20251226_1445_BRIDGE14_INV_SaveContractBridge.md`

### SOTS_Input

**Worklog highlights:**

- Buddy Worklog - SOTS_Input BRIDGE1 - 2025-12-25 22:19:15 — Added SOTS_Input core bridge settings (default OFF) for lifecycle bridge + verbose logs.; Ensures the router on PC BeginPlay.

**Reference docs:**
- `Plugins/SOTS_Input/Docs/SOTS_Input_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/SOTS_Input/Docs/Worklogs/BuddyWorklog_20251225_221915_SOTS_Input_BRIDGE1.md`

### SOTS_KillExecutionManager

**Worklog highlights:**

- Buddy Worklog — SOTS_KillExecutionManager — BRIDGE6 Save Participant Bridge — Added a new KEM DeveloperSettings surface to control the bridge.; Implemented `FKEM_SaveParticipant : ISOTS_CoreSaveParticipant` that reports save blocking via `USOTS_KEMManagerSubsystem::IsSaveBlocked()`.

**Reference docs:**
- `Plugins/SOTS_KillExecutionManager/Docs/SOTS_KEM_SOTSCoreSaveContractBridge.md`

**Worklogs:**
- `Plugins/SOTS_KillExecutionManager/Docs/Worklogs/BuddyWorklog_20251226_113100_SOTS_KEM_BRIDGE6_SaveParticipantBridge.md`

**Context anchors:**
- `Plugins/SOTS_KillExecutionManager/Docs/Anchor/Buddy_ContextAnchor_20251226_1131_BRIDGE6_SaveParticipantBridge.md`

### SOTS_MMSS

**Worklog highlights:**

- Buddy Worklog — 2025-12-26 — BRIDGE17 — SOTS_MMSS ↔ SOTS_Core lifecycle/travel bridge — Added MMSS DeveloperSettings toggles (default OFF).; Added state-only native delegate surface for WorldReady / PrimaryPlayerReady / PreLoadMap / PostLoadMap.

**Reference docs:**
- `Plugins/SOTS_MMSS/Docs/SOTS_MMSS_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/SOTS_MMSS/Docs/Worklogs/BuddyWorklog_20251226_154900_BRIDGE17_MMSS_CoreLifecycleBridge.md`

**Context anchors:**
- `Plugins/SOTS_MMSS/Docs/Anchor/Buddy_ContextAnchor_20251226_1549_BRIDGE17_MMSS_CoreLifecycleBridge.md`

### SOTS_MissionDirector

**Worklog highlights:**

- Buddy Worklog — 2025-12-26 — BRIDGE9 MissionDirector Core lifecycle bridge — Added `SOTS_Core` dependency + optional listener registration to `SOTS_MissionDirector`.; Added `USOTS_MissionDirectorCoreBridgeSettings` (defaults OFF).

**Reference docs:**
- `Plugins/SOTS_MissionDirector/Docs/SOTS_MD_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/SOTS_MissionDirector/Docs/Worklogs/BuddyWorklog_20251226_120115_BRIDGE9_MissionDirectorCoreLifecycleBridge.md`

**Context anchors:**
- `Plugins/SOTS_MissionDirector/Docs/Anchor/Buddy_ContextAnchor_20251226_1201_BRIDGE9_MissionDirectorCoreBridge.md`

### SOTS_ProfileShared

**Worklog highlights:**

- Buddy Worklog - SOTS_ProfileShared BRIDGE2 - 2025-12-25 22:32:50 — Added ProfileShared Core bridge settings (default OFF) for lifecycle bridge + verbose logs.; WorldStartPlay -> caches world and fires `OnCoreWorldStartPlay` (state-only).

**Reference docs:**
- `Plugins/SOTS_ProfileShared/Docs/SOTS_ProfileShared_SOTSCoreBridge.md`

**Worklogs:**
- `Plugins/SOTS_ProfileShared/Docs/Worklogs/BuddyWorklog_20251225_223250_SOTS_ProfileShared_BRIDGE2.md`

### SOTS_SkillTree

**Worklog highlights:**

- Buddy Worklog — BRIDGE8 SkillTree Save Contract Bridge — Added a new DeveloperSettings surface for enabling the bridge (defaults OFF).; emits `FragmentId="SkillTree.State"` by serializing `FSOTS_SkillTreeProfileData` to bytes
- Buddy Worklog — 20251226_114600 — BRIDGE8 SkillTree Core Save Bridge — Added `SOTS_Core` dependency wiring for `SOTS_SkillTree` (Build.cs + .uplugin).; Added new DeveloperSettings surface `USOTS_SkillTreeCoreBridgeSettings` (defaults OFF).

**Reference docs:**
- `Plugins/SOTS_SkillTree/Docs/SOTS_SkillTree_SOTSCoreSaveContractBridge.md`

**Worklogs:**
- `Plugins/SOTS_SkillTree/Docs/Worklogs/BuddyWorklog_20251226_114500_BRIDGE8_SkillTreeSaveContractBridge.md`
- `Plugins/SOTS_SkillTree/Docs/Worklogs/BuddyWorklog_20251226_114600_BRIDGE8_SkillTreeCoreSaveBridge.md`

**Context anchors:**
- `Plugins/SOTS_SkillTree/Docs/Anchor/Buddy_ContextAnchor_20251226_1145_BRIDGE8_SkillTreeSaveContractBridge.md`
- `Plugins/SOTS_SkillTree/Docs/Anchor/Buddy_ContextAnchor_20251226_1146_BRIDGE8_SkillTreeCoreSaveBridge.md`

### SOTS_Stats

**Worklog highlights:**

- Buddy Worklog — SOTS_Stats — BRIDGE7 Save Participant Bridge — Added a Stats DeveloperSettings surface to enable/disable the bridge (default OFF).; Implemented `FStats_SaveParticipant : ISOTS_CoreSaveParticipant` (id `Stats`).

**Reference docs:**
- `Plugins/SOTS_Stats/Docs/SOTS_Stats_SOTSCoreSaveContractBridge.md`

**Worklogs:**
- `Plugins/SOTS_Stats/Docs/Worklogs/BuddyWorklog_20251226_114800_SOTS_Stats_BRIDGE7_SaveParticipantBridge.md`

**Context anchors:**
- `Plugins/SOTS_Stats/Docs/Anchor/Buddy_ContextAnchor_20251226_1148_BRIDGE7_SaveParticipantBridge.md`

### SOTS_Steam

**Worklog highlights:**

- Buddy Worklog — 2025-12-26 14:58 — BRIDGE15 Steam Core Lifecycle Bridge — `bEnableSOTSCoreLifecycleBridge` (master); `bAllowCoreTriggeredSteamInit` (explicit allow)

**Reference docs:**
- `Plugins/SOTS_Steam/Docs/SOTS_Steam_SOTSCoreLifecycleBridge.md`

**Worklogs:**
- `Plugins/SOTS_Steam/Docs/Worklogs/BuddyWorklog_20251226_145800_BRIDGE15_Steam_CoreLifecycleBridge.md`

**Context anchors:**
- `Plugins/SOTS_Steam/Docs/Anchor/Buddy_ContextAnchor_20251226_1458_BRIDGE15_Steam_CoreLifecycleBridge.md`

### SOTS_UI

**Worklog highlights:**

- Buddy Worklog — 2025-12-26 09:30 — SOTS_UI BRIDGE3 (Core HUDReady host seam) — Added `SOTS_Core` dependency to SOTS_UI module (Build.cs).; `bEnableSOTSCoreHUDHostBridge`
- Buddy Worklog — 2025-12-26 13:35 — SOTS_UI BRIDGE10 (Core Save Contract query seam) — `bEnableSOTSCoreSaveContractBridge`; `bEnableSOTSCoreSaveContractBridgeVerboseLogs`

**Reference docs:**
- `Plugins/SOTS_UI/Docs/SOTS_UI_SOTSCoreBridge.md`
- `Plugins/SOTS_UI/Docs/SOTS_UI_SOTSCoreSaveContractBridge.md`

**Worklogs:**
- `Plugins/SOTS_UI/Docs/Worklogs/BuddyWorklog_20251226_0930_SOTS_UI_BRIDGE3.md`
- `Plugins/SOTS_UI/Docs/Worklogs/BuddyWorklog_20251226_133500_BRIDGE10_SOTS_UI_SaveContractQuery.md`

**Context anchors:**
- `Plugins/SOTS_UI/Docs/Anchor/Buddy_ContextAnchor_20251226_0930_SOTS_UI_BRIDGE3.md`
- `Plugins/SOTS_UI/Docs/Anchor/Buddy_ContextAnchor_20251226_1335_BRIDGE10_SOTS_UI_SaveContractBridge.md`
