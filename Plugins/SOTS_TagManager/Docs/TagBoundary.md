# TagBoundary — When TagManager is REQUIRED (and when it’s not)

## Why this page exists
SOTS uses GameplayTags heavily, but **TagManager is not a “route every tag operation” rule**.

TagManager’s real job is to keep plugins **decoupled** by making cross-plugin state changes observable and consistent.

This page defines the boundary so:
- we don’t over-engineer (rewriting everything through TagManager), and
- we don’t under-engineer (plugins silently hard-coupling via direct tag mutation on shared actors).

---

## Definitions

### Shared runtime actor-state tags (a “TagBoundary”)
A tag is a **TagBoundary** tag when **all** are true:

1) The tag is written to a **shared actor**:
- Player
- Dragon
- Guards
- Pickups / world items intended to be read by other plugins

2) Another plugin is expected to:
- react to it,
- gate logic on it,
- or treat it as shared truth.

These tags exist to carry **cross-plugin meaning** (decoupling).

### Local/internal tags (NOT a TagBoundary)
A tag is **local/internal** when it is:
- only used inside one plugin’s implementation,
- stored in a private container that is not treated as shared truth,
- or purely asset/config metadata.

These tags can remain direct.

---

## The Rule (the whole point)

### MUST use TagManager / TagLibrary when…
A plugin **writes/reads** shared runtime actor-state tags on Player/Dragon/Guards/Pickups **that other plugins are expected to observe/gate on**.

**Examples (typical boundary tags):**
- GSM writes Player stealth tier tags (other systems react)
- AIPerception writes guard alert/perception tags (other systems gate)
- BodyDrag writes “Dragging” state tags (UI/interaction/animation gate)
- SkillTree writes unlock/state tags that other systems gate abilities on

### OK to bypass TagManager when…
Tag usage is **internal/local** and not part of a cross-plugin contract.

**Examples (typical local tags):**
- internal “phase” tags inside one plugin’s component/subsystem
- tags stored on a settings asset for editor organization
- metadata tags on items/DA rows that never become shared runtime actor-state

---

## Hard locks that apply at TagBoundaries

### 1) Shared actors MUST expose tags
Player, Dragon, Guards, Pickups must expose tags (typically via IGameplayTagAssetInterface).

### 2) No bypass at the boundary
For boundary tags, do not mutate shared actor tag containers directly “because it’s easier”.
Use TagManager APIs so the boundary is consistent and observable.

### 3) Reactive tag-change events are required
Boundary tags must be observable via TagManager tag-added/tag-removed events.
Prefer reacting to events over polling.

### 4) Scoped/handle-based loose tags
For transient boundary tags (projectiles, temporary states, short-lived AI):
- AddLooseTag returns a handle
- RemoveLooseTag(handle) removes exactly what was added
- EndPlay cleanup is a safety backstop

---

## Ownership model: “One writer, many readers”
For each boundary tag group, lock down:
- **Writer-of-truth** (which plugin/component sets/clears it)
- **Consumers** (which plugins are allowed to gate/react)
- **Semantics** (what the tag *means*, when it’s applied/removed)

Avoid two plugins “helpfully” writing the same tag.

---

## How to document boundary contracts (required for code-completeness passes)

Every plugin should add a small section in its docs:

### TagBoundary Contract
**Publishes (writes to shared actors):**
- Tag: `X.Y.Z` on [Player|Dragon|Guard|Pickup] — meaning — add/remove conditions — writer component

**Consumes (reads shared actors):**
- Tag: `A.B.C` on [Player|Dragon|Guard|Pickup] — meaning — what behavior it gates

**Events consumed:**
- OnTagAdded / OnTagRemoved for tag groups (or explicit individual tags)

---

## Seed examples (living list — expand as you scan plugins)

### GlobalStealthManager (GSM)
- Publishes (Player):
  - `SOTS.Stealth.State.*` (tier/state)
  - `SOTS.Stealth.Light.*` (light band)
- Consumers:
  - UI (stealth feedback), AIPerception (awareness), MissionDirector (progress events), FX (stings)

### AIPerception
- Publishes (Guards):
  - `SAS.AI.Alert.*` (alert state)
  - `SAS.AI.Perception.*` (LOS, known last known, etc.)
- Consumers:
  - AIBT behavior selection, GSM awareness aggregation, UI/FX as needed

### BodyDrag
- Publishes (Player / Body target):
  - `SAS.BodyDrag.State.Dragging`
  - `SAS.BodyDrag.Target.Dead` / `SAS.BodyDrag.Target.KO`
- Consumers:
  - Interaction gating, movement/animation gating, UI prompts

### Interaction
- Consumes (Player):
  - RequiredPlayerTags / BlockedPlayerTags gates (boundary reads)
- Publishes:
  - Prefer delegates/intents for UI; only publish boundary actor-state tags if other plugins must gate on interaction state.

### Ability system (SOTS_GAS_Plugin / custom)
- Consumes (Player):
  - RequiredOwnerTags (HasAll) / BlockedOwnerTags (HasAny)
- Consumes (Inventory boundary):
  - RequiredInventoryTags supports any-of and all-of (definition-level; runtime reads depend on inventory provider)

---

## Special note: InvSP UI integration (not a TagBoundary)
InvSP’s UI stack/navigation/back-close is a sealed third-party “black box”.
SOTS_UI may open/close it only via the InvSP Adapter entrypoints.

If you need lifecycle awareness, use SOTS_UI delegates (ExternalMenuOpened/Closed) carrying a MenuId tag.
This is **UI lifecycle signaling**, not shared runtime actor-state gating.

---

## Quick review checklist (use during plugin passes)
- Does this plugin set/clear tags on Player/Dragon/Guard/Pickup?
  - If YES, are those tags meant to be observed by another plugin?
    - If YES → MUST route through TagManager and expose tag-change events.
- Are there any direct container mutations on shared actors?
  - If YES at boundary → replace with TagManager calls.
- Are transient tags applied without handles?
  - If YES at boundary → convert to handle-based scoped tags + EndPlay cleanup.
- Is boundary tag ownership documented (one writer)?
  - If NO → add TagBoundary Contract section.
