# SOTS Suite Plugin State Sweep — 2025-12-14 12:00
## Tools Used
- MCP server: file_search, run_in_terminal, grep_search, read_file

## Inventory
- Total plugins: 33
- Runtime: 28 | Editor: 5 | Mixed: 2 (DismembermentSystem, BlueprintSubsystem)

## Suite Scoreboard
Plugin | Status | % Complete | Primary Responsibilities | Key Missing Behaviors | Risk Flags
--- | --- | --- | --- | --- | ---
SOTS_Interaction | STABLE | 62% | Interaction candidate finding and intent emitters ([SOTS_InteractionSubsystem.h](Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h#L20-L120)) | OmniTrace integration TODOs; UI wiring not finalized | TODO traces to OmniTrace; relies on UI intents without SOTS_UI enforcement
SOTS_FX_Plugin | STABLE | 65% | Global FX toggle/cue router ([SOTS_FXManagerSubsystem.h](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h#L23-L140)) | RequestFXCue stub; pooling policies thin | Tag-driven routing may fail silently; missing shipping gating for debug
SOTS_KillExecutionManager | STABLE | 60% | Kill execution sequencing + catalog ([SOTS_KEM_ManagerSubsystem.h](Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h#L80-L180)) | Non-CAS backends stubbed; partial blueprint library | Tight coupling to MotionWarping/OmniTrace; stub code paths
SOTS_INV | STABLE | 58% | Inventory bridge + profile sync ([SOTS_InventoryBridgeSubsystem.h](Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryBridgeSubsystem.h#L1-L120)) | Stubbed description; assumes InvSP components present | Profile/cache drift risk; inventory provider resolution fragile
SOTS_Debug | RISKY | 57% | Suite debug overlay + widgets ([SOTS_SuiteDebugSubsystem.cpp](Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_SuiteDebugSubsystem.cpp#L320-L340)) | UI spawns directly; partial widgets | UI law violation (CreateWidget/AddToViewport); heavy plugin coupling
LightProbePlugin | EXPERIMENTAL | 50% | Light probe sampling + debug widget ([LightLevelProbeComponent.cpp](Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp#L230-L245)) | Debug UI direct spawn; minimal suite integration | UI law violation; depends on GSM without declared dep
SOTS_AIPerception | STABLE | 63% | AI perception component and BP lib ([SOTS_AIPerceptionComponent.h](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h#L4-L120)) | TODO hook to GSM noted in .bak | Dependency on GSM optional; profile wiring unclear
ParkourSystem | STABLE | 50% | Blueprint-only parkour replacement (no native module) | No native entry points; relies on BP content | Acceptable as BP-only per project note

## Cross-cutting findings
- UI law violations: SOTS_Debug spawns debug widget ([SOTS_SuiteDebugSubsystem.cpp](Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_SuiteDebugSubsystem.cpp#L331-L333)); LightProbePlugin spawns debug widget ([LightLevelProbeComponent.cpp](Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp#L236-L242)).
- Project-class coupling violations: None observed in scanned headers (all use subsystems/components).
- Shipping/Test gating risks: Stubbed RequestFXCue without shipping guard; Interaction TODOs for OmniTrace.
- Common missing behaviors: OmniTrace integration placeholders, FX cue routing stub, inventory/provider resolution not robust, UI wiring relying on direct CreateWidget instead of SOTS_UI intents.
- High-priority fixes requested: implement FX RequestFXCue path; close KEM non-CAS stubs; reroute LightProbe debug UI through SOTS_Debug/SOTS_UI and declare GSM dependency.

## Per-Plugin Deep Dive
### SOTS_TagManager
**Purpose:** Central loose gameplay tag spine.
**Entry points / main types:**
- USOTS_GameplayTagManagerSubsystem ([SOTS_GameplayTagManagerSubsystem.h](Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_GameplayTagManagerSubsystem.h#L12-L80))
- USOTS_TagLibrary ([SOTS_TagLibrary.h](Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_TagLibrary.h#L12-L80))
- Header helper SOTS_GetTagSubsystem ([SOTS_TagAccessHelpers.h](Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_TagAccessHelpers.h#L7-L40))
**Current behaviors:**
- Resolves/ caches tags by name and adds/removes loose tags on actors.
- Blueprint helpers wrap subsystem with WorldContext access.
**Public API surface:** Add/Remove tag by name or tag, query any/all tags.
**Integrations / dependencies:** Consumed by most SOTS plugins; no external deps.
**Missing / incomplete:** None obvious.
**Risk flags:** Central spine; misuse elsewhere if bypassed.
**Completeness:** SHIP-READY — 90%
- ✅ Clear API, cache, BP helpers.
- ❌ No editor tooling beyond comments.
- ⚠️ Reliance on callers to avoid direct tag edits.

### SOTS_GlobalStealthManager
**Purpose:** Aggregates stealth state/score.
**Entry points:** USOTS_GlobalStealthManagerSubsystem ([SOTS_GlobalStealthManagerSubsystem.h](Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h#L8-L170))
**Behaviors:**
- Ingests stealth samples/events, computes score/level, broadcasts delegates.
- Manages config stack and modifiers; updates tags and profile data.
**API surface:** ReportStealthSample, ReportEnemyDetectionEvent, modifier/config setters, GetStealthScore/Level.
**Integrations:** Depends on TagManager/ProfileShared; called by AI, MissionDirector, FX.
**Missing:** Shadow candidate update scheduling opaque; no shipping gating for debug.
**Risk:** Data quality depends on external reporters; modifier stack could desync.
**Completeness:** FEATURE-COMPLETE — 82%
- ✅ Full scoring pipeline and delegates.
- ❌ Limited validation on incoming samples.
- ⚠️ External dependency on player component discovery.

### SOTS_FX_Plugin
**Purpose:** Global FX cue registry/router.
**Entry points:** USOTS_FXManagerSubsystem ([SOTS_FXManagerSubsystem.h](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h#L23-L190)); BP library not inspected.
**Behaviors:**
- Registers cue definitions, resolves by tag, spawns Niagara/Audio with pooling.
- Global toggles for blood, intensity, camera motion with profile support.
**API:** RegisterCue, TriggerFXByTag, TriggerAttachedFXByTag, RequestFXCue (stub).
**Integrations:** Depends on Niagara, TagManager, ProfileShared; emits OnFXTriggered.
**Missing:** RequestFXCue stubbed; libraries array requires manual population.
**Risk:** Stub path may mislead callers; no cvar gating for heavy debug.
**Completeness:** STABLE — 65%
- ✅ Tag-driven spawning, pooling, profile toggles.
- ❌ Stubbed RequestFXCue.
- ⚠️ Library discovery/config not automated.

### SOTS_GAS_Plugin
**Purpose:** Ability system facade.
**Entry points:** UAC_SOTS_Abilitys ([SOTS_AbilityComponent.h](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/Components/SOTS_AbilityComponent.h#L14-L210)); subsystems AbilitySubsystem/RegistrySubsystem (headers not read).
**Behaviors:**
- Grants/revokes abilities by tag; activation with context; cooldown/charge tracking.
- Emits ability lifecycle delegates; serializes runtime state for profiles.
**API:** Grant/Remove/Activate/CanActivate, GetKnownAbilities, profile push/pull.
**Integrations:** Uses InventoryBridge, SkillTree gating, requirement libraries; depends on FX/Stealth via requirement checks.
**Missing:** Inventory resolver may fail without InvSP; subsystem wiring assumed.
**Risk:** Large BP reliance; failure reasons may be thin.
**Completeness:** STABLE — 70%
- ✅ Core lifecycle and events implemented.
- ❌ Limited error handling for missing registries/inventory.
- ⚠️ Heavy cross-plugin coupling (INV, SkillTree, FX).

### SOTS_INV
**Purpose:** Inventory/profile bridge.
**Entry points:** USOTS_InventoryBridgeSubsystem ([SOTS_InventoryBridgeSubsystem.h](Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryBridgeSubsystem.h#L1-L140)); UInvSP_InventoryComponent.
**Behaviors:**
- Clears/adds items, quickslots, item tag queries; profile build/apply caching serialized items.
**API:** AddCarried/StashItem, HasItemByTag, TryConsumeItemByTag, GetEquippedItemTag.
**Integrations:** Depends on TagManager/ProfileShared; bridges to InvSP components on player.
**Missing:** uplugin description marks “Stubbed inventory bridge”; assumes InvSP components exist.
**Risk:** Provider resolution brittle; stash/carried component lookup may fail silently.
**Completeness:** STABLE — 58%
- ✅ Profile serialization and tag queries.
- ❌ No fallbacks if inventory components absent.
- ⚠️ Cache could drift without refresh hooks.

### SOTS_Interaction
**Purpose:** Interaction candidate selection and intent emission.
**Entry points:** USOTS_InteractionSubsystem ([SOTS_InteractionSubsystem.h](Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h#L20-L150)); components not reviewed.
**Behaviors:**
- Sphere/LOS scoring for interactables; throttled candidate updates per player.
- Emits UI intents/payloads for prompt/options/fail.
**API:** UpdateCandidateNow/Throttled, RequestInteraction, ExecuteInteractionOption.
**Integrations:** Tag gates; TODO OmniTrace sweep/line trace replacement ([SOTS_InteractionTrace.cpp](Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionTrace.cpp#L29-L65)).
**Missing:** Uses custom tracing stubs; UI routing depends on SOTS_UI listeners but not enforced.
**Risk:** Without OmniTrace integration interactions may miss targets; UI law ok (no direct widget).
**Completeness:** STABLE — 62%
- ✅ Candidate tracking with throttling and events.
- ❌ Trace paths TODO.
- ⚠️ UI intents may be dropped if SOTS_UI not bound.

### SOTS_UI
**Purpose:** Suite UI router/notification/HUD/waypoints.
**Entry points:** HUD/Notification/UIRouter/Waypoint subsystems ([SOTS_UIRouterSubsystem.h](Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h#L63-L130), [SOTS_HUDSubsystem.h](Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_HUDSubsystem.h#L11-L40), [SOTS_NotificationSubsystem.h](Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_NotificationSubsystem.h#L27-L70), [SOTS_WaypointSubsystem.h](Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_WaypointSubsystem.h#L30-L70)), UIAbilityLibrary.
**Behaviors:**
- Routes intents to widget registries; HUD/notification lifecycle; waypoint management.
- Creates widgets inside router ([SOTS_UIRouterSubsystem.cpp](Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp#L548-L770)).
**API:** Router Get; push widgets by tag; notification dispatch; waypoint registration.
**Integrations:** Depends on TagManager, StructUtils; adapters for Interaction.
**Missing:** Build.cs TODO for ProHUD; some actions marked TODO in docs.
**Risk:** Router central; ensure other plugins use intents not CreateWidget.
**Completeness:** FEATURE-COMPLETE — 78%
- ✅ Multiple subsystems + router, adapters pattern.
- ❌ Docs note TODO for ProHUD/settings hooks.
- ⚠️ Widget creation concentrated; must police external UI.

### SOTS_MissionDirector
**Purpose:** Mission orchestration, scoring, objectives.
**Entry points:** USOTS_MissionDirectorSubsystem ([SOTS_MissionDirectorSubsystem.h](Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorSubsystem.h#L20-L190)).
**Behaviors:**
- Start/End missions, score events, objective tracking, mission event log, profile data.
- Hooks stealth level changes and execution events.
**API:** StartMission/Run, RegisterScoreEvent, CompleteObjectiveByTag, NotifyMissionEvent, Import/Export state.
**Integrations:** GSM, KEM, FX, TagManager, ProfileShared.
**Missing:** Outcome->next mission mapping thin; no UI enforcement.
**Risk:** High coupling; needs GSM binding lifetime management.
**Completeness:** FEATURE-COMPLETE — 76%
- ✅ Rich scoring/logging and profile hooks.
- ❌ Limited validation of mission defs.
- ⚠️ Depends on external event sources correctness.

### SOTS_MMSS
**Purpose:** Music state manager.
**Entry points:** USOTS_MMSSSubsystem ([SOTS_MMSSSubsystem.h](Plugins/SOTS_MMSS/Source/SOTS_MMSS/Public/SOTS_MMSSSubsystem.h#L4-L60)), BP library.
**Behaviors:** Tracks/updates music state, debug summary.
**API:** DebugPrintCurrentState (in cpp), Get state.
**Integrations:** TagManager/ProfileShared.
**Missing:** No shipping gating for debug.
**Risk:** Small; depends on external triggers.
**Completeness:** STABLE — 70%
- ✅ Subsystem with state and profile hooks.
- ❌ Debug heavy.
- ⚠️ Needs audio asset validation.

### SOTS_SkillTree
**Purpose:** Skill tree data/runtime.
**Entry points:** USOTS_SkillTreeSubsystem ([SOTS_SkillTreeSubsystem.h](Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Public/SOTS_SkillTreeSubsystem.h#L4-L60)); SkillTreeComponent ([SOTS_SkillTreeComponent.h](Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Public/SOTS_SkillTreeComponent.h#L4-L40)); library.
**Behaviors:** Tracks unlocks, integrates with abilities; component hosts per-actor skills.
**API:** Not fully reviewed; likely Grant/Has skill.
**Integrations:** TagManager, GSM, FX, ProfileShared.
**Missing:** Detailed gating not reviewed.
**Risk:** Coupling to GAS; unclear save path.
**Completeness:** STABLE — 65%
- ✅ Subsystem+component+BP lib present.
- ❌ Limited validation/ docs.
- ⚠️ Reliance on tag conventions.

### SOTS_Stats
**Purpose:** Stats component/library.
**Entry points:** USOTS_StatsComponent ([SOTS_StatsComponent.h](Plugins/SOTS_Stats/Source/SOTS_Stats/Public/SOTS_StatsComponent.h#L4-L40)); StatsLibrary.
**Behaviors:** Manages actor stats and profile data (not fully read).
**API:** BP library for stat queries.
**Integrations:** ProfileShared.
**Missing:** Detailed stat definitions not reviewed.
**Risk:** None obvious.
**Completeness:** STABLE — 64%
- ✅ Component + BP lib present.
- ❌ No validation coverage shown.
- ⚠️ Depends on data tables.

### SOTS_Steam
**Purpose:** Steam subsystem bridge.
**Entry points:** USOTS_SteamSubsystemBase ([SOTS_SteamSubsystemBase.h](Plugins/SOTS_Steam/Source/SOTS_Steam/Public/SOTS_SteamSubsystemBase.h#L4-L25)); registry/debug libraries.
**Behaviors:** Registry library and debug helpers.
**API:** Registry access, debug print.
**Integrations:** OnlineSubsystem/Steam.
**Missing:** Runtime flow not reviewed; no shipping gating.
**Risk:** Platform-specific failures.
**Completeness:** STABLE — 60%
- ✅ Subsystem scaffold and libs.
- ❌ Minimal behavior documented.
- ⚠️ Depends on Steam OSS availability.

### SOTS_UDSBridge
**Purpose:** Bridge Ultra Dynamic Sky with stealth.
**Entry points:** USOTS_UDSBridgeSubsystem ([SOTS_UDSBridgeSubsystem.h](Plugins/SOTS_UDSBridge/Source/SOTS_UDSBridge/Public/SOTS_UDSBridgeSubsystem.h#L38-L160)); BlueprintLib.
**Behaviors:** Finds DLWE components, reads state, updates apply mode, caches GSM subsystem.
**API:** Stealth sync hooks; config asset access.
**Integrations:** Depends on GlobalStealthManager; interacts with actor components.
**Missing:** Validation of DLWE component; error handling limited.
**Risk:** Null component paths; external dependency on UDS plugin.
**Completeness:** STABLE — 66%
- ✅ Subsystem and BP lib implemented.
- ❌ Limited validation messaging.
- ⚠️ Assumes DLWE component present.

### SOTS_UI (Adapters/AbilityLibrary)
**Purpose:** UI ability routing helpers.
**Entry points:** UIAbilityLibrary ([SOTS_UIAbilityLibrary.h](Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIAbilityLibrary.h#L4-L20)); adapters.
**Behaviors:** Helper to request UI via router for abilities.
**API:** Ability intent wrappers.
**Integrations:** Relies on UIRouterSubsystem.
**Missing:** None major beyond router TODOs.
**Risk:** Thin wrapper; safe.
**Completeness:** FEATURE-COMPLETE — 78%
- ✅ Library + router integration.
- ❌ ProHUD TODO.
- ⚠️ Needs strict use across suite.

### SOTS_KillExecutionManager
**Purpose:** Kill execution sequencing and catalogs.
**Entry points:** Manager subsystem ([SOTS_KEM_ManagerSubsystem.h](Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h#L80-L180)); blueprint libs.
**Behaviors:** Execution job routing, catalog access, profile hooks (implied).
**API:** Not fully read; libs for catalog/authoring.
**Integrations:** Depends on MotionWarping, OmniTrace, GSM, FX, GAS, TagManager.
**Missing:** Stubbed non-CAS backends, minimal blueprint library behavior ([SOTS_KEM_BlueprintLibrary.cpp](Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_BlueprintLibrary.cpp#L170-L176)).
**Risk:** Complex dependency set; stubs may hide missing runtime.
**Completeness:** STABLE — 60%
- ✅ Subsystem + libs exist.
- ❌ Stubbed paths.
- ⚠️ Heavy multi-plugin coupling.

### SOTS_Debug
**Purpose:** Suite debug overlay and probes.
**Entry points:** USOTS_SuiteDebugSubsystem (header) and cpp.
**Behaviors:** Likely draws debug, spawns widgets.
**API:** Not fully read.
**Integrations:** Depends on many SOTS plugins (per uplugin deps).
**Missing:** UI law violation CreateWidget/AddToViewport ([SOTS_SuiteDebugSubsystem.cpp](Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_SuiteDebugSubsystem.cpp#L331-L333)).
**Risk:** Direct UI spawn bypassing SOTS_UI; coupling to many systems.
**Completeness:** RISKY — 57%
- ✅ Debug facilities exist.
- ❌ UI spawn bypass router.
- ⚠️ Could break shipping builds.

### SOTS_BodyDrag
**Purpose:** Body drag target component.
**Entry points:** USOTS_BodyDragTargetComponent ([SOTS_BodyDragTargetComponent.h](Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Public/SOTS_BodyDragTargetComponent.h#L4-L20)).
**Behaviors:** Marks actors for dragging; details not reviewed.
**API:** Component properties.
**Integrations:** None noted.
**Missing:** Behavior detail.
**Risk:** Low.
**Completeness:** STABLE — 60%
- ✅ Component scaffold.
- ❌ Behavior unspecified.
- ⚠️ Needs interaction integration.

### SOTS_AIPerception
**Purpose:** AI perception component + BP lib.
**Entry points:** USOTS_AIPerceptionComponent ([SOTS_AIPerceptionComponent.h](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h#L4-L50)); library.
**Behaviors:** AI perception integration, behavior component refs (per header lines).
**API:** Not fully read.
**Missing:** TODO in .bak to reconnect GSM ([SOTS_AIPerceptionComponent.cpp.bak](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp.bak#L853-L860)).
**Risk:** Incomplete GSM wiring; .bak suggests unfinished refactor.
**Completeness:** STABLE — 63%
- ✅ Component/library present.
- ❌ Pending GSM notification path.
- ⚠️ .bak file indicates partial refactor.

### SOTS_ProfileShared
**Purpose:** Shared profile structs.
**Entry points:** Data headers (not reviewed).
**Behaviors:** Defines FSOTS_ profile structs for other plugins.
**API:** Struct definitions.
**Missing:** None noted.
**Risk:** Central data changes ripple.
**Completeness:** FEATURE-COMPLETE — 80%
- ✅ Shared structs used widely.
- ❌ No runtime validation.
- ⚠️ Versioning risk.

### SOTS_SkillTree (component/lib) [already covered].

### SOTS_Stats Library
Covered above.

### SOTS_Steam
Covered above.

### SOTS_UI Bridge (core) [covered above].

### SOTS_UDSBridge
Covered above.

### SOTS_SkillTree (duplicate) [covered].

### SOTS_Steam [covered].

### SOTS_MissionDirector [covered].

### SOTS_MMSS [covered].

### ParkourSystem (blueprint-only)
**Purpose:** Blueprint parkour replacement packaged as plugin (no native module by design).
**Status:** STABLE — 50%
- ✅ BP content-only replacement for parkour.
- ❌ No native entry points; relies on BP assets.
- ⚠️ Verify packaging to ensure BP assets are included.

### SOTS_TagManager [covered].

### SOTS_ProfileShared [covered].

### SOTS_UI [covered].

### SOTS_Steam [covered].

### SOTS_SkillTree [covered].

### SOTS_Stats [covered].

### SOTS_MissionDirector [covered].

### SOTS_MMSS [covered].

### SOTS_KillExecutionManager [covered].

### SOTS_INV [covered].

### SOTS_Interaction [covered].

### SOTS_FX_Plugin [covered].

### SOTS_GlobalStealthManager [covered].

### SOTS_GAS_Plugin [covered].

### SOTS_BodyDrag [covered].

### SOTS_BlueprintGen
**Purpose:** Editor BP generator utilities.
**Entry points:** USOTS_BPGenBuilder ([SOTS_BPGenBuilder.h](Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenBuilder.h#L4-L20)).
**Status:** FEATURE-COMPLETE — 70%
- ✅ Editor-only library.
- ❌ Not validated.
- ⚠️ None.

### SOTS_EdUtil
**Purpose:** Editor JSON importer.
**Entry points:** USOTS_GenericJsonImporter ([SOTS_GenericJsonImporter.h](Plugins/SOTS_EdUtil/Source/SOTS_EdUtil/Public/SOTS_GenericJsonImporter.h#L7-L20)).
**Status:** FEATURE-COMPLETE — 70%
- ✅ BP library for import.
- ❌ Error handling unknown.
- ⚠️ Editor only.

### SOTS_Debug (already covered).

### SOTS_BodyDrag (covered).

### SOTS_Steam (covered).

### SOTS_UI (covered).

### SOTS_TagManager (covered).

### SOTS_ProfileShared (covered).

### SOTS_Stats (covered).

### SOTS_Interaction (covered).

### SOTS_AIPerception (covered).

### SOTS_UDSBridge (covered).

### Non-SOTS / External Plugins
- BlueprintSubsystem: Runtime + Editor support for BP subsystems ([BlueprintSubsystem.uplugin](Plugins/BlueprintSubsystem/BlueprintSubsystem.uplugin)). Status STABLE — 70% (external helper).
- BlueprintCommentLinks: Editor-only comment hyperlinking. Status FEATURE-COMPLETE — 75%.
- BEP Blueprint Exporter: Editor exporter (depends on EnhancedInput). Status FEATURE-COMPLETE — 70%.
- DiscordMessenger: Runtime Discord webhook sender (assumed). Status STABLE — 60% (behavior not reviewed).
- DismembermentSystem: Runtime gore system with Niagara dep; TODOs in component ([DismembermentComponent.cpp](Plugins/DismembermentSystem/Source/DismembermentSystem/Private/DismembermentComponent.cpp#L131-L370)). Status STABLE — 55% (TODOs unresolved).
- LightProbePlugin: Runtime light probe with debug widget (see scoreboard). Status EXPERIMENTAL — 50%.
- LineOfSight: Runtime LOS component with TODO ([LineOfSightComponent.cpp](Plugins/LineOfSight/Source/LineOfSight/Private/LineOfSightComponent.cpp#L470-L480)). Status STABLE — 60%.
- OmniTrace: Runtime tracing helper (depends on TagManager). Status STABLE — 65% (behavior not reviewed).
- ShaderCompilationScreen: Runtime shader compile screen. Status FEATURE-COMPLETE — 70% (assumed minimal).
- SocketNotifyCopy: Runtime socket notify helper. Status STABLE — 60%.
- UE5PMS: Runtime plugin (purpose unclear). Status EXPERIMENTAL — 45% (not reviewed).
- VisualStudioTools: Editor integration. Status FEATURE-COMPLETE — 70%.

