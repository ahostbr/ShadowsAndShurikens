# SOTS DataAsset audit (2025-12-14)

Scope: authored DataAssets/PrimaryDataAssets inside Plugins/. Focused on where they are consumed so designers know where to hook them up.

## Cross-cutting tools
- Editor import helper: `Plugins/SOTS_EdUtil` ships a generic JSON→DataAsset importer (see `SOTS_EdUtil.uplugin` and `SOTS_GenericJsonImporter`).
- Export helper: BEP exporter (`Plugins/BEP/Source/BEP/Public/BlueprintFlowExporter.h`) can dump DataAssets/DataTables to JSON for review.

## Per-plugin inventory

### SOTS_UI
- `USOTS_WidgetRegistryDataAsset` — registry of `FSOTS_WidgetRegistryEntry` rows ([Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_WidgetRegistryDataAsset.h](../Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_WidgetRegistryDataAsset.h)).
- Consumed by `USOTS_UIRouterSubsystem` via soft ref from settings; loaded on subsystem init ([Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp#L25-L74](../Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp#L25-L74)). Author a DefaultWidgetRegistry in settings.

### SOTS_MMSS
- `USOTS_MissionMusicLibrary` — mission id → `FSOTS_MissionMusicSet`, with fallback ([Plugins/SOTS_MMSS/Source/SOTS_MMSS/Public/SOTS_MMSS_MissionMusicLibrary.h](../Plugins/SOTS_MMSS/Source/SOTS_MMSS/Public/SOTS_MMSS_MissionMusicLibrary.h)). Used by MMSS subsystem for track lookups.

### SOTS_GlobalStealthManager
- `USOTS_StealthConfigDataAsset` — global stealth tuning knobs ([Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_StealthConfigDataAsset.h](../Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_StealthConfigDataAsset.h)).
- Loaded/stacked by `USOTS_GlobalStealthManagerSubsystem` (Set/Push/DefaultConfigAsset) to drive scoring.

### SOTS_MissionDirector
- `USOTS_MissionDefinition` (PrimaryDataAsset) — mission objectives, rules, failure conditions, optional `USOTS_StealthConfigDataAsset` override ([Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorTypes.h#L5-L170](../Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorTypes.h#L5-L170)). Consumed by MissionDirector subsystem for runtime state and debrief logging.

### SOTS_GAS_Plugin
- `USOTS_AbilityDefinitionDA` — single ability definition (tags, costs, etc.).
- `USOTS_AbilityDefinitionLibrary` — list of `USOTS_AbilityDefinitionDA` for bulk registration.
- `USOTS_AbilitySetDA` — tagged set of ability grants.
- `USOTS_AbilityMetadataDA` — display strings/icon/category/sort order for UI.
- `USOTS_AbilityCueDA` — VFX/SFX handles per gameplay cue tag.
- All above defined in [Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/Data/SOTS_AbilityDataAssets.h](../Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/Data/SOTS_AbilityDataAssets.h).
- Consumption: `USOTS_AbilityRegistrySubsystem::RegisterAbilityDefinitionsFromDataAsset` consumes the library for runtime registry population ([Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L59-L95](../Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L59-L95)).
- `USOTS_AbilityRequirementLibraryAsset` — tag→`FSOTS_AbilityRequirements` list for requirement checks ([Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/SOTS_GAS_AbilityRequirementLibraryAsset.h](../Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/SOTS_GAS_AbilityRequirementLibraryAsset.h)).

### SOTS_FX_Plugin
- `USOTS_FXCueDefinition` — tag→VFX/SFX/camera shake bundle ([Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXCueDefinition.h](../Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXCueDefinition.h)).
- `USOTS_FXDefinitionLibrary` — list of `FSOTS_FXDefinition` for FX manager lookups ([Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXDefinitionLibrary.h](../Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXDefinitionLibrary.h)).

### SOTS_BodyDrag
- `USOTS_BodyDragConfigDA` — KO/dead drag tuning ([Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Public/SOTS_BodyDragConfigDA.h](../Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Public/SOTS_BodyDragConfigDA.h)).

### SOTS_AIPerception
- `USOTS_AIGuardPerceptionDataAsset` — per-guard suspicion tags/speeds ([Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIGuardPerceptionDataAsset.h](../Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIGuardPerceptionDataAsset.h)).
- `USOTS_AIPerceptionConfig` (PrimaryDataAsset) — archetype sight/hearing thresholds, tag mapping, curves ([Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionConfig.h](../Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionConfig.h)). Referenced by `USOTS_AIPerceptionComponent` (GuardConfig).

### OmniTrace
- `UOmniTracePatternPreset` — FOmniTraceRequest + metadata ([Plugins/OmniTrace/Source/OmniTrace/Public/OmniTracePatternPreset.h](../Plugins/OmniTrace/Source/OmniTrace/Public/OmniTracePatternPreset.h)).
- `UOmniTracePatternLibrary` — array of presets, `FindPresetById` helper ([Plugins/OmniTrace/Source/OmniTrace/Public/OmniTracePresetLibrary.h](../Plugins/OmniTrace/Source/OmniTrace/Public/OmniTracePresetLibrary.h)).
- `USOTS_OmniTraceKEMPresetLibrary` — presets used by KEM warp logic ([Plugins/OmniTrace/Source/OmniTrace/Public/SOTS_OmniTraceKEMPresetLibrary.h](../Plugins/OmniTrace/Source/OmniTrace/Public/SOTS_OmniTraceKEMPresetLibrary.h)).

### SOTS_KillExecutionManager (KEM)
- `USOTS_SpawnExecutionData` — montages + warp point names for spawn-actor backend ([Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_Types.h#L420-L465](../Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_Types.h#L420-L465)).
- `USOTS_KEM_ExecutionDefinition` (PrimaryDataAsset) — main execution authoring surface: tags, selection gates, backend configs, ability requirements, FX tags, warp points ([Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_Types.h#L470-L575](../Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_Types.h#L470-L575)).
- `USOTS_KEM_ExecutionRegistryConfig` — list of execution definitions for startup registration ([Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_Types.h#L610-L625](../Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_Types.h#L610-L625)).

### SOTS_SkillTree
- `USOTS_SkillTreeDefinition` — per-tree nodes/links and lookup helper ([Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Public/SOTS_SkillTreeDefinition.h](../Plugins/SOTS_SkillTree/Source/SOTS_SkillTree/Public/SOTS_SkillTreeDefinition.h)).

### SOTS_Steam
- `USOTS_SteamAchievementRegistry` — achievement definitions with `FindByInternalId` ([Plugins/SOTS_Steam/Source/SOTS_Steam/Public/SOTS_SteamAchievementRegistry.h](../Plugins/SOTS_Steam/Source/SOTS_Steam/Public/SOTS_SteamAchievementRegistry.h)).
- `USOTS_SteamLeaderboardRegistry` — leaderboard definitions with `FindByInternalId` ([Plugins/SOTS_Steam/Source/SOTS_Steam/Public/SOTS_SteamLeaderboardRegistry.h](../Plugins/SOTS_Steam/Source/SOTS_Steam/Public/SOTS_SteamLeaderboardRegistry.h)).

### SOTS_UDSBridge
- `USOTS_UDSBridgeConfig` — UDS discovery hooks, polling cadence, DLWE reflection settings ([Plugins/SOTS_UDSBridge/Source/SOTS_UDSBridge/Public/SOTS_UDSBridgeConfig.h](../Plugins/SOTS_UDSBridge/Source/SOTS_UDSBridge/Public/SOTS_UDSBridgeConfig.h)).

### SOTS_BodyDrag
- `USOTS_BodyDragConfigDA` — drag tuning knobs ([Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Public/SOTS_BodyDragConfigDA.h](../Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Public/SOTS_BodyDragConfigDA.h)).

### Misc / tooling
- OmniTrace presets are referenced by KEM (see `FSOTS_OmniTraceKEMPresetEntry` soft refs).
- BEP/EdUtil are tooling-only and safe to keep editor-only.

## Quick guidance for authors
- Prefer PrimaryDataAsset when you need stable asset IDs for registries (`USOTS_KEM_ExecutionDefinition`, `USOTS_AIPerceptionConfig`).
- When a subsystem expects a soft reference (UI router, KEM registry, ability registry), author the DataAsset in Content and assign it in settings/config DAs rather than hardcoding paths.
- For JSON import/export workflows, use SOTS_EdUtil importer or BEP exporter; no direct code changes required.
