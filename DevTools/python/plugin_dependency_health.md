SOTS Repo Overview — High-level structure
Root: ShadowsAndShurikens
ShadowsAndShurikens.uproject — main Unreal project (EngineAssociation 5.7).
ShadowsAndShurikens.sln — Visual Studio solution created after GenerateProjectFiles.
Source/ — project’s native module ShadowsAndShurikens (Game module, plus the ShadowsAndShurikensEditor target).
Plugins/ — multiple self-contained plugins (SOTS_* modular components + other third party / editor plugins).
DevTools/python/ — automation and analysis scripts for builds, linting, plugin audits, and log analysis.
Config/ (project config), Saved/ (logs, UBT logs, crash logs), Binaries/, Intermediate/
Engine references:
Default DevTools config references UE 5.7 at UE_5.7.
The ShadowsAndShurikens.uproject references engine 5.7.
How to build:

Use sots_tools.py hub or the UBT Build.bat in sots_tools_config.json.
Configurable build command: E:/UE_5.7p/UE_5.7/Engine/Build/BatchFiles/Build.bat ShadowsAndShurikensEditor Win64 Development
Use GenerateProjectFiles.bat to make Visual Studio solution: GenerateProjectFiles.bat -project="E:/SAS/ShadowsAndShurikens/ShadowsAndShurikens.uproject" -game -engine
Plugin Map — Summary of SOTS* and other important plugins
Below are the main plugins, everything discovered under Plugins/ in top-level folder. For each plugin: name, role, modules, and important dependencies.

SOTS_TagManager

Role: Central gameplay tag helpers and cached tag lookup (GameInstance subsystem).
Module(s): SOTS_TagManager (Runtime).
Key files: SOTS_GameplayTagManagerSubsystem.h, SOTS_TagAccessHelpers.h.
Important to many modules: SOTS_TagManager is a core hub for tag queries; used by many other plugins.
SOTS_ProfileShared

Role: Profile snapshot/struct definitions (FSOTS_* types). Shared data for saving/loading player state.
Module(s): SOTS_ProfileShared.
Key structs: FSOTS_ProfileSnapshot, FSOTS_ProfileTypes, FSOTS_ItemSlotBinding. Central to save/load mechanisms.
SOTS_Stats

Role: Per-character stat component and implementations (Stat storage, change notifications).
Module(s): SOTS_Stats.
Key classes: USOTS_StatsComponent (ActorComponent).
Dependencies: SOTS_ProfileShared.
SOTS_SkillTree

Role: Data-driven skill tree; per-player skill tree component, nodes, progression.
Module(s): SOTS_SkillTree.
Key classes: USOTS_SkillTreeComponent, USOTS_SkillTreeDefinition, USOTS_SkillTreeSubsystem.
Dependencies: SOTS_TagManager, SOTS_GlobalStealthManager, SOTS_FX_Plugin, SOTS_ProfileShared.
SOTS_KillExecutionManager (KEM)

Role: KEM — handling execution/cinematic attacks, CAS selection, helpers for KEM logic and UI.
Module(s): SOTS_KillExecutionManager.
Key classes: SOTS_KEM_ManagerSubsystem, helper actor, blueprint libs.
Dependencies: ContextualAnimation, MotionWarping, OmniTrace, SOTS_GAS_Plugin, SOTS_GlobalStealthManager, SOTS_FX_Plugin, SOTS_TagManager.
SOTS_GlobalStealthManager (GSM)

Role: Tracks global stealth state (aggregation, alert tiers, stealth flags) across player/AI/environment.
Module(s): SOTS_GlobalStealthManager.
Key classes: USOTS_GlobalStealthManagerSubsystem, SOTS_PlayerStealthComponent, SOTS_AIStealthBridgeComponent.
Dependencies: SOTS_TagManager, SOTS_ProfileShared.
SOTS_GAS_Plugin

Role: Core ability/GameplayAbilitySystem integration — blueprint-first; handles ability gating, skills, stealth conditions.
Module(s): SOTS_GAS_Plugin.
Key classes: Ability gating library, ability subsystem, ability registry, stealth bridge subsystem.
Strong dependencies: SOTS_SkillTree, SOTS_SkillTree (gating), SOTS_GlobalStealthManager, SOTS_FX_Plugin, SOTS_TagManager, SOTS_UI, SOTS_INV, SOTS_ProfileShared.
SOTS_FX_Plugin

Role: Global FX manager for Niagara VFX, audio, camera shakes; pooling and FX orchestration.
Module(s): SOTS_FX_Plugin.
Key classes: USOTS_FXManagerSubsystem, FX logging classes.
Dependencies: Niagara, SOTS_TagManager, SOTS_ProfileShared, SOTS_GlobalStealthManager.
SOTS_UI

Role: HUD/notifications/waypoint subsystem and helpers (geometry/UI notifications).
Module(s): SOTS_UI.
Key classes: USOTS_HUDSubsystem, USOTS_NotificationSubsystem, USOTS_WaypointSubsystem.
Dependencies: UMG, Slate, SOTS_TagManager (and others by usage).
SOTS_MMSS (Music Manager)

Role: Music manager, mission-centric track selection, crossfade management, uses data asset mission libraries.
Module(s): SOTS_MMSS.
Key classes: USOTS_MMSSSubsystem, USOTS_MissionMusicLibrary.
Dependencies: SOTS_TagManager, SOTS_ProfileShared.
SOTS_MMSS, SOTS_MissionDirector

SOTS_MissionDirector handles mission scoring & run logging.
SOTS_MissionDirector depends on SOTS_KillExecutionManager, SOTS_FX_Plugin, SOTS_GlobalStealthManager, SOTS_TagManager, SOTS_ProfileShared.
SOTS_INV (Inventory)

Role: Inventory bridge/stub; item serialization for profile persistence (FSOTS_SerializedItem).
Module(s): SOTS_INV.
Key classes: UInvSP_InventoryComponent, SOTS_InventoryBridgeSubsystem.
Dependencies: SOTS_TagManager, SOTS_ProfileShared.
SOTS_AIPerception

Role: Stealth-aware AI perception (sight/hearing + stealth integration).
Module(s): SOTS_AIPerception.
Key classes: USOTS_AIPerceptionSubsystem, USOTS_AIPerceptionComponent, SOTS_AIGuardPerceptionDataAsset.
Dependencies: SOTS_TagManager, SOTS_GlobalStealthManager, SOTS_ProfileShared, SOTS_FX_Plugin, AIModule.
OmniTrace

Role: Utility — path generation & blueprint presets; scripting helper.
Module(s): OmniTrace.
Key classes: OmniTraceBlueprintLibrary, pattern presets.
BEP (Blueprint Exporter)

Role: Editor plugin for blueprint export tooling, Editor target.
Other engine-side & Project plugins: DismembermentSystem, DiscordMessenger, LightProbePlugin, BEP, OmniTrace, etc.

Key Systems & Where They Live
Gameplay Tag Management

SOTS_TagManager — central tag cache and actor tag helpers (GameInstance subsystem)
Usage: SOTS_GAS_Plugin gating, AI perception, FX logic, UI state conditions.
Abilities / GameplayAbilities (GAS)

SOTS_GAS_Plugin (plugin-level) — gating, ability permissions, registries, bridging with SOTS_GlobalStealthManager and SOTS_SkillTree.
Contains several subsystems, blueprint libraries, and data-driven capability.
Stealth / AI Integration

SOTS_GlobalStealthManager — central aggregator for detection levels/tiers and pistol-waterfall logic.
SOTS_AIPerception — AI-sensing integration that factors in GSM.
UI / HUD / Notifications

SOTS_UI — isolated UI subsystems; HUD widget data update bridge via USOTS_HUDSubsystem.
SOTS_UI integrates with SOTS_TagManager and other sub-systems to drive UI.
Stats, Inventory & Profile Snapshot

SOTS_Stats — per-character stats (component).
SOTS_INV — inventory components and slot serialization bridging for profile save.
SOTS_ProfileShared and SOTS_ProfileSubsystem — saved profile serialization and snapshot struct definitions (FSOTS_*). SOTS_ProfileSubsystem in the main game module handles overall save/load.
FX / Audio / Music

SOTS_FX_Plugin — Niagara, audio, camera shakes central manager.
SOTS_MMSS — music manager, mission-driven music selection.
Missions & Scoring

SOTS_MissionDirector — collects weighted score events, logs mission runs, and composes mission summaries; depends on KEM and other systems.
Kill Execution Manager (ex: context-based execution flows)

SOTS_KillExecutionManager — manages execution animations and related rules; integrates with ContextualAnimation, MotionWarping, OmniTrace.
Central hubs & subsystems:

USOTS_GameplayTagManagerSubsystem (TagManager) — central helper used widely across plug-ins.
USOTS_ProfileSubsystem (in main module) & SOTS_ProfileShared — central for persistence.
Global subsystems (GameInstanceSubsystems) such as USOTS_MMSSSubsystem, USOTS_MissionDirectorSubsystem, USOTS_KEM_ManagerSubsystem provide singletons for cross-plugin coordination.
Architecture Diagram (Textual)
Main game (ShadowsAndShurikens) modules depend on:

Core modules: SOTS_ProfileShared, SOTS_Stats, SOTS_MMSS, SOTS_FX_Plugin, SOTS_MissionDirector.
Central services:

SOTS_TagManager -> used by: SOTS_UI, SOTS_GAS, SOTS_AIPerception, SOTS_FX_Plugin, SOTS_GlobalStealthManager, SOTS_INV.
SOTS_ProfileShared -> used by: SOTS_MMSS, SOTS_INV, SOTS_Stats, SOTS_GAS, etc.
SOTS_GlobalStealthManager -> used by: SOTS_KillExecutionManager, SOTS_GAS_Plugin, SOTS_MissionDirector, SOTS_AIPerception.
Example connection flows (typical use):

Player performs ability -> SOTS_GAS_Plugin checks gating via SOTS_SkillTree + SOTS_TagManager -> if successful triggers FX via SOTS_FX_Plugin -> KEM/mission director update calls SOTS_MissionDirector for scoring.
Enemy detection -> SOTS_AIPerception collects sensor data -> queries SOTS_GlobalStealthManager -> decides state changes and writes to the SOTS_TagManager or directly triggers events.
Saving/Loading -> SOTS_ProfileSubsystem collects snapshot from ShadowsAndShurikens world and plugins via SOTS_ProfileShared types.
Complexity / Risk Hotspots — Things to watch
High coupling of many plugins to SOTS_TagManager & SOTS_ProfileShared.

TagManager is used widely; changes to tag handling or caching can impact many systems.
SOTS_ProfileShared structs are central to save/load: changes are risky; must maintain backward compatibility or migration.
Inter-plugin dependency web

SOTS_GAS_Plugin depends on SOTS_SkillTree, SOTS_GlobalStealthManager, SOTS_FX_Plugin, SOTS_TagManager, SOTS_INV, and SOTS_UI (tight coupling).