# SOTS Plugin Suite

This repo is a mirror of the **`Plugins/`** folder for my Unreal Engine project  
**Shadows And Shurikens (SOTS)**.

Each top-level folder in this repo is a self-contained Unreal Engine plugin.  
Some of them are general-purpose (things like tracing, tag management, UI), and  
others are tightly focused on SOTS systems like stealth, missions, and profiles.

> ⚠️ Status: Work in progress. APIs may change without notice while SOTS is in development.

---

## Supported Engine Version

- Target engine: **Unreal Engine 5.7** (project-level)
- Plugins are designed to live in a **project’s `Plugins/` folder**, not as engine-wide installs.

---

## Plugin Catalog

### Core SOTS Systems

- **SOTS_TagManager**  
  Central Gameplay Tag manager for SOTS. Provides a `GameInstanceSubsystem` and helper
  libraries so every other plugin can share a single tag vocabulary.

- **SOTS_GlobalStealthManager**  
  Global stealth scoring and visibility system used by SOTS. Owns the high-level
  “how visible is the player right now?” calculations and exposes them to other plugins.

- **SOTS_AIPerception**  
  Bridge between Unreal’s `AIPerception` and the SOTS stealth layer. Keeps AI sensing
  and SOTS’ stealth scores in sync.

- **SOTS_MissionDirector**  
  Mission / objective orchestration: tracks mission state, goals, and outcomes.  
  Designed around SOTS’ discrete mission structure (no open world).

- **SOTS_ProfileShared**  
  Shared types and helpers for save profiles, metadata and cross-plugin profile logic.

- **SOTS_Stats**  
  Stats/attribute layer for SOTS (health-like values, difficulty scaling hooks, etc.).

- **SOTS_SkillTree**  
  Planned skill-tree system that will gate certain abilities and upgrades.  
  (API is still evolving while the core game solidifies.)

### Gameplay & Presentation

- **SOTS_KillExecutionManager**  
  Kill Execution Manager (KEM). Handles selecting and playing contextual execution
  animations, spawning helper actors, and coordinating with traces / tags.

- **SOTS_INV**  
  Inventory integration for SOTS, wired around a diegetic inventory concept  
  (bamboo scroll, quick-select, etc.) and external inventory plugins.

- **SOTS_UI**  
  UI and HUD integration for SOTS: menu stack, in-world UI hooks, notification systems,
  and other shared UI utilities.

- **SOTS_MMSS**  
  Music / MetaSound System glue for SOTS. Connects stealth state, mission state, and
  other systems to music and ambience playback.

- **SOTS_FX_Plugin**  
  Global FX manager for visual and audio cues that need to be shared across abilities,
  executions, stealth states, etc.

- **SOTS_GAS_Plugin**  
  Bridge between SOTS and Unreal’s Gameplay Ability System / related tooling.  
  Used as a foundation for the custom ability approach inside SOTS.

### Utility / Supporting Plugins

- **LightProbePlugin**  
  Light / shadow probing utilities. Used by SOTS stealth systems to reason about how
  “safe” or exposed a location is.

- **OmniTrace**  
  Advanced tracing and path-sampling helpers. Provides reusable Blueprint & C++ tools
  for line traces, multi-traces, and debug visualization.

- **BlueprintCommentLinks**  
  Small utility plugin that helps with Blueprint comment / navigation ergonomics.

- **DiscordMessenger**  
  Utility for sending messages from the game/editor to a Discord channel (for logging,
  build notifications, etc.).

- **BEP**  
  Internal helper/experiment plugin. Contents and APIs may change frequently.

- **DismembermentSystem**  
  Third-party dismemberment plugin mirrored here alongside the rest of the suite.

---