# SAS Tag Schema Locked (SOTS)

**Status:** LOCKED BASELINE (v1)  
**Owner:** SOTS_TagManager  
**Applies to:** All SOTS plugins + project content  
**Root Namespace:** `SAS.*`  
**Goal:** Provide a complete, forward-looking tag universe so systems never invent ad-hoc tags mid-implementation.

---

## Rules

### 1) One root, one style
- All new tags **must** live under `SAS.*`.
- Use `UpperCamel` per segment (e.g., `SAS.Stealth.Detection.Detected`).
- No “freeform” tag branches inside other plugins.

### 2) Tags represent **state**, not events
- Tags answer: “What is true right now?”
- Events should travel through the message/broadcast layer, not as transient tags.

### 3) Continuous values must snap to bins
If a system produces a 0–1 float (light, detection, perception), also expose a **bin tag** so logic and UI can react deterministically.

### 4) Debug tags exist, but are non-shipping
- Debug tags may be read in Shipping/Test, but any behavior gated solely by debug tags must be compiled out or hard-disabled for Shipping/Test builds.

### 5) Change control
- Any new tag branch or new meaning requires:
  1) Update this file  
  2) Update the tag registry (ini/data)  
  3) Run tag usage report (DevTools) and ensure no “unknown roots” appear

---

## Canonical Roots

### SAS.Player.*
Player control, life, posture, movement, traversal.

- `SAS.Player.Control.Ninja`
- `SAS.Player.Control.Dragon`
- `SAS.Player.Control.Cutscene`
- `SAS.Player.Control.Disabled`

- `SAS.Player.Life.Alive`
- `SAS.Player.Life.Dead`

- `SAS.Player.Posture.Standing`
- `SAS.Player.Posture.Crouching`
- `SAS.Player.Posture.Prone` *(reserved)*

- `SAS.Player.Movement.Walk`
- `SAS.Player.Movement.Run`
- `SAS.Player.Movement.Sprint`
- `SAS.Player.Movement.SilentStep` *(perk/gear reserved)*

- `SAS.Player.State.Aiming`
- `SAS.Player.State.Aiming.Head`
- `SAS.Player.State.Aiming.Body`
- `SAS.Player.State.Aiming.Leg` *(reserved)*

- `SAS.Player.Traversal.Parkour.Active`
- `SAS.Player.Traversal.Parkour.LedgeHang`
- `SAS.Player.Traversal.Parkour.ClimbUp`
- `SAS.Player.Traversal.Cover.InCover`
- `SAS.Player.Traversal.Cover.PeekLeft`
- `SAS.Player.Traversal.Cover.PeekRight`

---

### SAS.Stealth.*
Player-facing stealth truth and drivers.

**Detection state**
- `SAS.Stealth.Detection.Undetected`
- `SAS.Stealth.Detection.Suspected`
- `SAS.Stealth.Detection.Detected`
- `SAS.Stealth.Detection.Escaping`
- `SAS.Stealth.Detection.Lost`

**Visibility bins**
- `SAS.Stealth.Visibility.Hidden`
- `SAS.Stealth.Visibility.Low`
- `SAS.Stealth.Visibility.Medium`
- `SAS.Stealth.Visibility.High`

**Drivers (why detection changed)**
- `SAS.Stealth.Driver.Light`
- `SAS.Stealth.Driver.Sound`
- `SAS.Stealth.Driver.LineOfSight`
- `SAS.Stealth.Driver.Proximity`
- `SAS.Stealth.Driver.DragonExposed`

**Light bins (snap from 0–1)**
- `SAS.Stealth.Light.Dark`
- `SAS.Stealth.Light.Dim`
- `SAS.Stealth.Light.Lit`
- `SAS.Stealth.Light.Floodlit`

**Noise bins**
- `SAS.Stealth.Noise.Silent`
- `SAS.Stealth.Noise.Low`
- `SAS.Stealth.Noise.Moderate`
- `SAS.Stealth.Noise.Loud`

---

### SAS.AI.*
Perception results, alertness tiers, search state.

**Senses**
- `SAS.AI.Sense.Sight`
- `SAS.AI.Sense.Hearing`
- `SAS.AI.Sense.Damage`
- `SAS.AI.Sense.Special.Dragon` *(reserved)*

**Reasons (GSM/AIPerception reports)**
- `SAS.AI.Reason.Sight`
- `SAS.AI.Reason.Hearing`
- `SAS.AI.Reason.Shadow`
- `SAS.AI.Reason.Damage`
- `SAS.AI.Reason.Generic`

**Perception results**
- `SAS.AI.Perception.HasLOS.Player`
- `SAS.AI.Perception.KnowsLastKnown.Player`
- `SAS.AI.Perception.LostTrack.Player`

**Focus (who the AI is working on)**
- `SAS.AI.Focus.Player`
- `SAS.AI.Focus.Unknown`

**Alertness tiers**
- `SAS.AI.Alert.Calm`
- `SAS.AI.Alert.Investigating`
- `SAS.AI.Alert.Searching`
- `SAS.AI.Alert.Alerted`
- `SAS.AI.Alert.Hostile`

**Search mechanics**
- `SAS.AI.Search.HasSearchArea`
- `SAS.AI.Search.Expanding`
- `SAS.AI.Search.GivingUp`

---

### SAS.Interaction.*
Interactable classification + intent + gating.

**Kinds**
- `SAS.Interaction.Kind.Door`
- `SAS.Interaction.Kind.Container`
- `SAS.Interaction.Kind.Loot`
- `SAS.Interaction.Kind.Body`
- `SAS.Interaction.Kind.Switch`
- `SAS.Interaction.Kind.Trap`
- `SAS.Interaction.Kind.NPC`

**Intent**
- `SAS.Interaction.Intent.Open`
- `SAS.Interaction.Intent.Close`
- `SAS.Interaction.Intent.Pickup`
- `SAS.Interaction.Intent.HideBody`
- `SAS.Interaction.Intent.DragBody`
- `SAS.Interaction.Intent.Assassinate`
- `SAS.Interaction.Intent.Knockout`

**Gates**
- `SAS.Interaction.Gate.RequiresKey`
- `SAS.Interaction.Gate.Lockpick`
- `SAS.Interaction.Gate.Skill`
- `SAS.Interaction.Gate.Tool`

---

### SAS.Item.*
Inventory categorization and equip intent (content-agnostic).

- `SAS.Item.Category.Tool`
- `SAS.Item.Category.Consumable`
- `SAS.Item.Category.Crafting`
- `SAS.Item.Category.KeyItem`
- `SAS.Item.Category.Quest`

- `SAS.Item.EquipSlot.Primary`
- `SAS.Item.EquipSlot.Secondary`
- `SAS.Item.EquipSlot.Utility`

- `SAS.Item.QuickSlot.1`
- `SAS.Item.QuickSlot.2`
- `SAS.Item.QuickSlot.3`
- `SAS.Item.QuickSlot.4`
- `SAS.Item.QuickSlot.5`
- `SAS.Item.QuickSlot.6`
- `SAS.Item.QuickSlot.7`
- `SAS.Item.QuickSlot.8`

---

### SAS.Effect.*
Status effects and delivery methods (for poisons, tools, traps).

**Effects**
- `SAS.Effect.Poison.Damage`
- `SAS.Effect.Poison.Nausea`
- `SAS.Effect.Poison.Sleep`
- `SAS.Effect.Poison.Paralysis`
- `SAS.Effect.Poison.Panic`

**Delivery**
- `SAS.Effect.Delivery.Shuriken`
- `SAS.Effect.Delivery.Dart`
- `SAS.Effect.Delivery.Arrow`
- `SAS.Effect.Delivery.Trap`

---

### SAS.Ability.*
Custom ability system identity + gating + grouping.

**Identity**
- `SAS.Ability.Ninja.*` *(leaf tags owned by ability registry)*
- `SAS.Ability.Dragon.*` *(leaf tags owned by ability registry)*

**Gating**
- `SAS.Ability.Gate.SkillTree`
- `SAS.Ability.Gate.ItemRequired`
- `SAS.Ability.Gate.StealthOnly`
- `SAS.Ability.Gate.OnlyWhenUndetected`

**Cooldown groups**
- `SAS.Ability.CooldownGroup.Dragon.Offense`
- `SAS.Ability.CooldownGroup.Dragon.Utility`
- `SAS.Ability.CooldownGroup.Ninja.Utility`

**Targets**
- `SAS.Ability.Target.Enemy`
- `SAS.Ability.Target.World`
- `SAS.Ability.Target.Self`

---

### SAS.KEM.*
KillExecutionManager classification and requirements (data-driven).

**Execution type**
- `SAS.KEM.Execution.Lethal`
- `SAS.KEM.Execution.NonLethal`

**Requirements**
- `SAS.KEM.Requirement.Unseen`
- `SAS.KEM.Requirement.BehindTarget`
- `SAS.KEM.Requirement.TargetDistracted`
- `SAS.KEM.Requirement.TargetSeated`
- `SAS.KEM.Requirement.TargetSleeping`

**Style**
- `SAS.KEM.Style.Quick`
- `SAS.KEM.Style.Silent`
- `SAS.KEM.Style.Brutal` *(aesthetic reserved)*

---

### SAS.Mission.*
MissionDirector phases, objectives, fail reasons.

**Phases**
- `SAS.Mission.Phase.Infiltration`
- `SAS.Mission.Phase.Objectives`
- `SAS.Mission.Phase.Exfiltration`
- `SAS.Mission.Phase.Complete`
- `SAS.Mission.Phase.Failed`

**Objective state**
- `SAS.Mission.Objective.Active`
- `SAS.Mission.Objective.Complete`
- `SAS.Mission.Objective.Optional`
- `SAS.Mission.Objective.Failed`

**Fail reasons**
- `SAS.Mission.Fail.Detected`
- `SAS.Mission.Fail.DragonLost`
- `SAS.Mission.Fail.PlayerDeath`
- `SAS.Mission.Fail.Time`

---

### SAS.UI.*
Routing, screens, layers, modals, prompts.

**Layers**
- `SAS.UI.Layer.HUD`
- `SAS.UI.Layer.Menu`
- `SAS.UI.Layer.Modal`
- `SAS.UI.Layer.Overlay`

**Screens**
- `SAS.UI.Screen.MainMenu`
- `SAS.UI.Screen.Pause`
- `SAS.UI.Screen.Settings`
- `SAS.UI.Screen.Inventory`
- `SAS.UI.Screen.MapScroll`
- `SAS.UI.Screen.LoadGame`
- `SAS.UI.Screen.SaveGame`

**Modals**
- `SAS.UI.Modal.Confirm`
- `SAS.UI.Modal.Warning`
- `SAS.UI.Modal.Error`

**Prompts**
- `SAS.UI.Prompt.Interact`
- `SAS.UI.Prompt.Locked`
- `SAS.UI.Prompt.Pickpocket`
- `SAS.UI.Prompt.Execution`

---

### SAS.FX.* / SAS.SFX.* / SAS.Music.*
Presentation routing (FX/SFX) and music roles/states.

**FX families**
- `SAS.FX.Stealth.*`
- `SAS.FX.Execution.*`
- `SAS.FX.Interaction.*`
- `SAS.FX.Environment.*`

**SFX families**
- `SAS.SFX.UI.*`
- `SAS.SFX.Stealth.*`
- `SAS.SFX.World.*`

**Music (locked schema)**
- `SAS.Music.Global.MainMenu`
- `SAS.Music.Global.Credits`
- `SAS.Music.Global.Safehouse`

- `SAS.Music.Role.Main`
- `SAS.Music.Role.Tension`
- `SAS.Music.Role.Alert`
- `SAS.Music.Role.StealthHeavy`
- `SAS.Music.Role.Exploration`
- `SAS.Music.Role.Safehouse`

- `SAS.Music.Role.Boss.Intro`
- `SAS.Music.Role.Boss.Loop`
- `SAS.Music.Role.Boss.Phase2`
- `SAS.Music.Role.Boss.Outro`

*(Optional convenience states)*
- `SAS.Music.State.Exploration`
- `SAS.Music.State.Tension`
- `SAS.Music.State.Alert`

---

### SAS.Debug.*
Dev-only toggles and rendering.

- `SAS.Debug.Enabled`
- `SAS.Debug.Draw.Stealth`
- `SAS.Debug.Draw.Perception`
- `SAS.Debug.Draw.Parkour`
- `SAS.Debug.UI.Bypass`

---

## Reserved roots (future-safe)
These roots are allowed but should stay empty until needed:
- `SAS.Net.*` *(reserved; SOTS is single-player)*
- `SAS.Analytics.*`
- `SAS.Accessibility.*`
- `SAS.PhotoMode.*`

---

## Notes
- Leaf tags under `SAS.Ability.Ninja.*` and `SAS.Ability.Dragon.*` are owned by the Ability Registry and should be generated/declared with a consistent naming policy per ability definition.
- Content-specific tagging belongs under `SAS.Content.*` (mission IDs, boss IDs, item IDs), not mixed into gameplay state tags.