# SOTS_CANON_NOTES.md
_Shadow And Shurikens (SOTS) — Canon + Pillars + System Ownership Notes_

> **Rule 0 (anti-drift):** This file is the single source of truth for SOTS narrative + high-level gameplay pillars.
> If anything conflicts with old chat logs / archive dumps, **this file wins**.

---

## Table of Contents
- [Narrative Canon](#narrative-canon)
- [Core Gameplay Pillars](#core-gameplay-pillars)
- [Dragon Companion](#dragon-companion)
- [Player Toolkit](#player-toolkit)
- [Abilities + Progression](#abilities--progression)
- [Traversal](#traversal)
- [UI + Inventory Philosophy](#ui--inventory-philosophy)
- [Difficulty + Saving Rules](#difficulty--saving-rules)
- [Stealth Backbone](#stealth-backbone)
- [Plugin Suite Ownership Locks](#plugin-suite-ownership-locks)
- [Dev Workflow Guardrails](#dev-workflow-guardrails)
- [Quick “Do Not Drift” Checklist](#quick-do-not-drift-checklist)

---

## Narrative Canon

### Opening (LOCKED CANON)
- Player is **meditating in a hot spring** with the dragon.
- They **go to a door** together.
- They are **ambushed**.
- The player **witnesses their father being mortally injured**.
- The dragon **gets the player to safety**.
- The father **dies**, **passing the dragon to the player**.

> **Deprecated (do not use):** Any older “wake in bed / intruder lends dragon” intro is obsolete.

### Main Arc
- The player’s dragon is **the last of its kind**.
- An **ancient clan** is dedicated to **eradicating dragons**.
- **Each boss holds a piece of dragon essence**; the player must reclaim them to restore the dragon’s power and legacy.
- The player’s **father died protecting the dragon**; the journey includes vengeance + uncovering the truth behind his death.

### Post-Grandmaster Hook
- After killing the **Grandmaster**, the dragon is **fully restored**.
- With full power returned, the dragon can **resurrect its long-lost lover**, opening a new chapter beyond the revenge arc.

---

## Core Gameplay Pillars
- **Pure stealth**: no traditional combat loop. Eliminations and escapes are stealth/tool/dragon-driven.
- **Diegetic feedback** over HUD meters whenever possible (stealth communicated via world + dragon presence).
- **Mission-select structure**, not open world.
- **Realistic UE5 look** with historical + mythological flavor.

---

## Dragon Companion

### Stealth Meter (Diegetic)
- The dragon acts as the player’s stealth meter:
  - Undetected → dragon clearly visible.
  - Rising detection risk → dragon fades/distorts.
  - Rare edge cases: if the dragon is “caught out,” it can mean death for both.

### Power Loop (Core Balance Rule)
- Player-performed **knockouts / stealth kills** replenish dragon power.
- Dragon-performed kills **do not** replenish dragon power.
- Goal: strategic dragon use that still keeps the player “doing the stealth work.”

---

## Player Toolkit

### Ninja Tools (Grounded, Physical)
- Poisons, darts, arrows, traps, lures—items are physical tools, not abstract spell consumables.

### Crafting Direction (Design Intent)
- Collect ingredients (plants/bushes/mushrooms/trees) → craft poisons → apply to tools.
- Support lethal and non-lethal outcomes (e.g., nausea/sickness to disrupt patrol routes).

### “Dragon Whispers” (Design Intent)
- A subtle command influence: enemies may be nudged to walk somewhere, drop something, hesitate, etc.
- Should feel like manipulation, not mind-control spectacle.

---

## Abilities + Progression
- Custom **player-only** ability system inspired by GAS principles (but **not** using GAS itself).
- Ability component name: `AC_SOTS_Abilitys`.
- Skill tree system exists to gate abilities and progression (unlock-driven).
- Ability system must integrate with:
  - Inventory-triggered abilities
  - Skill unlock gating
  - Clean data structures for UI + progression logic

---

## Traversal
- ParkourComponent exists and is foundational for traversal and ledge movement.
- Advanced/rare traversal moments can be ability-driven (separate from baseline parkour).

---

## UI + Inventory Philosophy

### UI Authority
- `SOTS_UI` is the **single UI hub/router** allowed to create/push/pop/focus widgets.
- All other systems communicate via **UI intents + payloads**.

### Inventory (Diegetic)
- Inventory is a physical **bamboo scroll** carried on the player.
- Full inventory access is only when **safe / not detected**.
- In unstable contexts (rooftops, danger, etc.) use **quick-select** instead.
- No weight-based inventory system.

### InvSP Integration Rule
- InvSP remains **self-contained** for internal menu navigation/input.
- `SOTS_UI` only pushes/pops InvSP’s top-level windows via adapters (no deep rewrites unless later locked).

---

## Difficulty + Saving Rules
- **Easy:** unlimited saves anywhere.
- **Medium:** saves only at designated save locations.
- **Hard:** only one save location per mission.
- **Very Hard:** no saves; mission must be completed in a single run.
- All modes save meta-progression after mission completion.

---

## Stealth Backbone

### Light + Detection Flow (High Level)
- LightProbePlugin → PlayerStealthComponent (light level input)
- PlayerStealthComponent → GSM (Global Stealth Manager) for global tier/alertness
- AI uses UAIPerception for sensing, but stealth tier logic belongs to GSM + PlayerStealthComponent ecosystem.

### Behavior Layer Clarification
- AIBT is a **behavior layer**, not the stealth brain.
- Stealth/detection ownership stays in GSM + AIPerception + tag/FX reporting.

### Breadcrumbs (UDSBridge)
- Breadcrumb trails are short-lived and capped:
  - ~30s validity
  - max ~30 points
- Consumed by AIPerception → AIBT.

---

## Plugin Suite Ownership Locks

### Project Shape
- Project is a Blueprint-only shell: **no game module assumptions**.
- All C++ lives in plugins; plugins must be component/interface/subsystem-driven.

### Suite Ownership (Golden Rules)
- Travel chain: **ProfileShared → UI → MissionDirector**
- Save triggers: autosave/time, UI-initiated save, checkpoint
- UI owns UI input policy; Input routing lives in `SOTS_Input` (PlayerController-owned spine)
- FX: one-shots via FXManager; pool overflow rejects + logs (no silent fail)
- Tag authority: `SOTS_TagManager` is the cross-plugin runtime tag authority

### Input Buffering Rule (Critical)
- Buffer windows are **montage-scoped** (Execution/Vanish/QTE style).
- Queue size = 1 (latest wins).
- Auto-clear on montage end/cancel/abort.

---

## Dev Workflow Guardrails

### CODE REVIEW STAGE Policy
- Code-only surface verification.
- Shipping hygiene and DA/config wiring are deferred.
- GameplayTags audit is **add-only** and **not deferred**.

### Context Anchors
- After major locks/sweeps: produce a short copy-paste `[CONTEXT_ANCHOR]` and route it into the correct plugin `Docs/Anchor/` folder.

### “Don’t Remove Features” Guard
- When adding functionality to tools/scripts, never silently drop existing features. Additions must be strictly additive unless explicitly approved.

---

## Quick “Do Not Drift” Checklist
- ✅ Opening is hot spring meditation → door → ambush → father mortally injured → dragon rescues → father passes dragon.
- ✅ Pure stealth (no traditional combat loop).
- ✅ Dragon is the stealth meter + power loop reward structure.
- ✅ Diegetic bamboo scroll inventory; full inventory only when safe.
- ✅ SOTS_UI is the only system that can push/pop/focus widgets.
- ✅ ProfileShared→UI→MissionDirector travel chain.
- ✅ TagManager is cross-plugin boundary authority; internal tags can remain local.
- ✅ Input buffering only via montage windows, queue size 1, latest wins.

---
_End of file_