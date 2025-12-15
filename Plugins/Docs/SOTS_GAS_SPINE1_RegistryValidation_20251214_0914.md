# SOTS_GAS SPINE_1 — Registry Determinism & Validation (2025-12-14 09:14)

## Current state (pre-change) evidence
- Ability registry existed but relied on ad-hoc manual registration with no bootstrap or ready gate: Register* helpers only, no init-time load/validate ([Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L41-L88](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L41-L88) before patch).

## Chosen deterministic model
- **Model A (config-driven explicit libraries + optional defs).** Registry now loads ordered soft references to `USOTS_AbilityDefinitionLibrary` and optional individual `USOTS_AbilityDefinitionDA` assets from config on init; no asset scanning.

## New config surface (default OFF)
- `AbilityDefinitionLibraries`, `AdditionalAbilityDefinitions` ([Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/Subsystems/SOTS_AbilityRegistrySubsystem.h#L69-L76](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/Subsystems/SOTS_AbilityRegistrySubsystem.h#L69-L76)).
- Validation toggles: `bValidateAbilityRegistryOnInit`, `bWarnOnDuplicateAbilityTags`, `bWarnOnMissingAbilityAssets`, `bWarnOnInvalidAbilityDefs`, `bWarnOnMissingAbilityTags`, `bWarnOnRegistryNotReady` ([.../SOTS_AbilityRegistrySubsystem.h#L78-L91](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/Subsystems/SOTS_AbilityRegistrySubsystem.h#L78-L91)). Defaults false.

## Behavior changes
- **Deterministic bootstrap:** Init builds registry from configured libraries/defs; maps reset then populated in order ([.../SOTS_AbilityRegistrySubsystem.cpp#L11-L29](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L11-L29), [.../SOTS_AbilityRegistrySubsystem.cpp#L234-L286](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L234-L286)).
- **Validation (dev-only, opt-in):** `ValidateRegistry` walks all defs; checks invalid tags and missing AbilityClass; duplicate detection occurs during registration with first-wins policy and optional warnings ([.../SOTS_AbilityRegistrySubsystem.cpp#L296-L333](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L296-L333)).
- **Duplicate policy:** first definition wins; later duplicates skipped with optional warning ([.../SOTS_AbilityRegistrySubsystem.cpp#L41-L63](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L41-L63)).
- **Missing/invalid asset warnings:** Config-gated warnings for invalid tags, failed soft-loads, or missing `AbilityClass` during validation ([.../SOTS_AbilityRegistrySubsystem.cpp#L234-L286](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L234-L286), [.../SOTS_AbilityRegistrySubsystem.cpp#L296-L333](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L296-L333)).
- **Registry ready gate:** `bRegistryReady` set only after build/validation; lookups and queries early-out with optional dev-only warning; BP helper `IsAbilityRegistryReady` plus `OnAbilityRegistryReady` event ([.../SOTS_AbilityRegistrySubsystem.h#L58-L66](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/Subsystems/SOTS_AbilityRegistrySubsystem.h#L58-L66), [.../SOTS_AbilityRegistrySubsystem.cpp#L118-L172](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L118-L172), [.../SOTS_AbilityRegistrySubsystem.cpp#L343-L373](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L343-L373)).
- **Missing tag reporting:** Optional dev-only warning when requests hit unknown tags ([.../SOTS_AbilityRegistrySubsystem.cpp#L345-L373](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L345-L373)).

## What the gate blocks
- Any definition lookup or requirements evaluation returns false if registry is not ready; activation callers indirectly respect this via `GetAbilityDefinitionByTag` gating ([.../SOTS_AbilityRegistrySubsystem.cpp#L118-L172](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityRegistrySubsystem.cpp#L118-L172)).

## Notes
- Defaults preserve prior behavior: empty config means no automatic load; manual Register* calls still work. Validation and warnings are off by default to keep Shipping/Test clean.
