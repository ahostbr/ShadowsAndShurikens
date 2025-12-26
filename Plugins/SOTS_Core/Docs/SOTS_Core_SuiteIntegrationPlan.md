# SOTS_Core Suite Integration Plan (SPINE + BRIDGE → C++-wired suite)

**Purpose:** Convert the suite’s “BP-only glue + timing guesses” into deterministic **C++ wiring** via **SOTS_Core**, similar to how CGF functioned as the project spine — while remaining **add-only**, **disabled-by-default**, and **rollback-safe** until you explicitly switch systems over.

This plan assumes:
- No project `/Source` module (all C++ lives in plugins).
- Adoption happens via **reparent-only** first (no BP graph edits).
- Each plugin bridge is **OFF-by-default** and can be enabled independently.
- Verification is driven by **Core diagnostics** (no guessing).

---

## 1) Design rules (locked)

### 1.1 SOTS_Core is the “engine spine” (not gameplay)
SOTS_Core owns:
- UE-native anchor base classes: GameMode/GameState/PlayerState/PlayerController/HUD/PlayerCharacter (player-only)
- A single lifecycle event bus (subsystem) + optional delegate bridges
- ModularFeatures-based listener dispatch (opt-in)
- Save/Restore contract interfaces (participants + opaque fragments + save-block status)
- Diagnostics + health reports + versioning guardrails

SOTS_Core does **not** own:
- Save IO, profile folders, slot policy, or restore orchestration (ProfileShared does)
- UI creation, input mapping, gameplay behaviors, stealth logic, objectives, etc.

### 1.2 ProfileShared remains IO authority
ProfileShared owns:
- Profile folder canonical structure and metadata
- Snapshot build/apply orchestration
- Save/restore policy and versioning for profile files

ProfileShared **consumes** Core lifecycle + identity + save-contract queries, but does not become the suite-wide lifecycle broadcaster.

### 1.3 Bridge principle: opt-in + inert defaults
Every BRIDGE integration must:
- Early-out unless enabled by settings (default OFF)
- Be state-only first (cache pointers + fire delegates)
- Avoid gameplay changes until later “wiring passes”
- Unregister cleanly on shutdown
- Provide clear logs/diagnostics *only when enabled*

---

## 2) Implementation phases (end-to-end)

### Phase 0 — Pre-flight guardrails (no behavior change)
**Goal:** Ensure integration is reversible and observable.

Checklist:
- [ ] Create/maintain a “scoreboard” section in SOTS_Core docs noting:
  - implemented SPINE passes
  - implemented BRIDGE passes
  - adoption status (reparented? yes/no)
  - enabled toggles (what’s ON?)
- [ ] Confirm the suite still runs in legacy mode with all Core/bridge toggles OFF.
- [ ] Define a single “enablement order” (see Phase 4) and stick to it.

Acceptance:
- No runtime behavior changes introduced yet.

---

### Phase 1 — Implement SOTS_Core SPINE1–10 (foundation)
**Goal:** Produce a dependency-root C++ spine that can exist harmlessly until adopted.

SPINE scope (Core-only):
- SPINE1: SOTS_Core plugin + ASOTS_* bases + lifecycle subsystem event bus
- SPINE2: settings + snapshot + duplicate suppression + optional world delegate bridge + BP helper library
- SPINE3: ModularFeatures listener interface + optional dispatch (default OFF)
- SPINE4: map/travel lifecycle bridge (default OFF, delegate-evidence only)
- SPINE5: primary player identity snapshot (no OnlineSubsystem)
- SPINE6: save/restore contract interfaces + registry helpers (default OFF)
- SPINE7: diagnostics console commands + health report
- SPINE8: compile-only automation tests (WITH_AUTOMATION_TESTS)
- SPINE9: versioning + config schema migration (log-only)
- SPINE10: hardening audit + GoldenPath adoption docs (verify + rollback)

Acceptance (Core-only):
- Diagnostics commands exist and are null-safe.
- Dispatch/bridges are OFF by default.
- No other plugin depends on SOTS_Core yet.

---

### Phase 2 — Implement BRIDGE1–15 (suite plugs into Core, still inert)
**Goal:** Allow plugins to subscribe to deterministic lifecycle and/or save contracts *without* changing gameplay.

#### 2A) Lifecycle bridges (deterministic “ready” seams)
- BRIDGE1: SOTS_Input listens (PC BeginPlay / Pawn Possessed) → calls existing internal init seams only
- BRIDGE2: ProfileShared listens → state-only delegates (OnWorldStartPlay, OnPrimaryPlayerReady)
- BRIDGE3: SOTS_UI listens (HUDReady) → RegisterHUDHost seam (no widget creation)
- BRIDGE9: MissionDirector listens → state-only world/player/travel seams
- BRIDGE11: AIPerception listens → state-only world/player seams
- BRIDGE12: GSM listens → state-only world/player seams
- BRIDGE13: FX listens → state-only world/player seams (or proven-safe init seam only)
- BRIDGE15: Steam listens → double-gated deterministic “safe init” seam

#### 2B) Save-contract bridges (participants + query surface)
- BRIDGE6: KEM save-block participant (reason text)
- BRIDGE7: Stats fragment participant (opaque bytes; seam-proven or stub)
- BRIDGE8: SkillTree fragment participant (opaque bytes; seam-proven or stub)
- BRIDGE14: INV fragment participant (opaque bytes; stub-safe until seam exists)
- BRIDGE10: UI router helper `QueryCanSaveViaCoreContract` (no actual saving)

#### 2C) Consistency + verification
- BRIDGE4: normalize toggle naming + dependencies + docs + enablement-order doc
- BRIDGE5: `SOTS.Core.BridgeHealth` diagnostics command

Acceptance (bridged, inert):
- With all bridge toggles OFF, behavior remains unchanged.
- With toggles ON, Core can list listeners/participants, but plugins remain state-only unless explicitly wired further.

---

### Phase 3 — GoldenPath adoption (make Core “real” like CGF)
**Goal:** Make Core lifecycle come from real UE anchors rather than pre-reparent delegate bridges.

**Reparent-only (no BP graph edits):**
- BP_CGF_GameMode → ASOTS_GameModeBase  
- BP_CGF_GameState → ASOTS_GameStateBase  
- BP_CGF_PlayerState → ASOTS_PlayerStateBase  
- BP_CGF_PlayerController → ASOTS_PlayerControllerBase  
- BP_CGF_HUD → ASOTS_HUDBase  
- BP_CGF_Player → ASOTS_PlayerCharacterBase  

Notes:
- This is the “CGF-style” move: the spine becomes **native**.
- Keep SPINE2 WorldDelegateBridge OFF initially unless you need it for early coverage.

Acceptance:
- Core snapshot populates deterministically via base class overrides.
- `SOTS.Core.Health` and `SOTS.Core.DumpLifecycle` show real PC/Pawn/HUD events.

Rollback:
- Reparent back to previous BP parents (documented in SPINE10 GoldenPath).

---

### Phase 4 — Enablement order (turn on wiring gradually)
**Goal:** Convert systems one at a time, verifying each.

Recommended order:
1) **Core only**
   - [ ] Ensure SOTS_Core is loaded
   - [ ] Run: `SOTS.Core.Health`
2) **Enable Core listener dispatch**
   - [ ] Enable `bEnableLifecycleListenerDispatch`
   - [ ] Run: `SOTS.Core.DumpListeners` and `SOTS.Core.BridgeHealth`
3) **Enable bridges one at a time (low-risk first)**
   - [ ] BRIDGE3 UI HUD host registration (visibility-only)
   - [ ] BRIDGE1 Input lifecycle (init seam only)
   - [ ] BRIDGE2 ProfileShared lifecycle (state-only delegates)
   - [ ] BRIDGE9 MissionDirector lifecycle (state-only)
   - [ ] BRIDGE11/12/13 (AIPerception/GSM/FX state-only)
   - [ ] BRIDGE15 Steam (only if needed, double-gated)
4) **Enable Save Contract querying**
   - [ ] Core: enable save participant queries (if gated)
   - [ ] BRIDGE6 KEM participant (save-block)
   - [ ] BRIDGE10 UI “can save?” query helper (still no saving)
5) **Enable map/travel bridge last**
   - [ ] SPINE4 `bEnableMapTravelBridge` (only after basics are stable)

Acceptance:
- Each toggle produces **observable** diagnostics changes with no surprise gameplay side-effects.

---

### Phase 5 — Replace BP-only orchestration with Core-driven C++ wiring (the “full intent”)
**Goal:** Move the originally intended BP glue into deterministic plugin C++ wiring.

This is the “real integration” step: bridges stop being state-only and begin driving existing init paths.

#### 5A) Input becomes lifecycle-driven (deterministic)
Target outcome:
- SOTS_Input binds to PC/Pawn via Core events rather than timing heuristics.
- Legacy fallback remains behind “bridge OFF”.

#### 5B) UI becomes HUD-host deterministic
Target outcome:
- SOTS_UI can assert a valid HUD host (no “render nowhere”).
- Later, any UI creation paths can be hardened to require host readiness.

#### 5C) ProfileShared becomes lifecycle-consumer + IO authority
Target outcome:
- ProfileShared uses:
  - Core primary identity snapshot to select profile folder/identity
  - Core “primary player ready” seam as the safe moment to apply/restore
- ProfileShared remains the only plugin that writes to disk.

#### 5D) Saving becomes contract-composed (decoupled)
Target outcome:
- ProfileShared asks Core:
  - “can we save?” (participants can block + reason)
  - “who has payload fragments?” (Stats/SkillTree/INV)
- Participants implement fragments without ProfileShared depending on them directly.

#### 5E) Deterministic readiness for gameplay systems
Target outcome:
- MissionDirector, GSM, AIPerception, FX, Steam establish references on Core-ready events
- This eliminates adapter/timing dependence and reduces “init order” bugs

Acceptance:
- Suite can run in “Core-driven mode” for these systems with legacy behavior disabled by config.

---

### Phase 6 — De-risk + clean retirement (later)
During add-only stages:
- Do not delete legacy code paths.
- Keep both paths:
  - Legacy path (default)
  - Core-driven path (opt-in)
- Add diagnostics to identify whether a subsystem is running legacy vs Core-driven.

Later (post-stabilization):
- Deprecate legacy paths, then remove in a planned cleanup pass.

---

## 3) Verification toolbox (must use)
Use these to prove wiring, not guess.

Core commands (from SPINE7 + BRIDGE5):
- `SOTS.Core.Health`
- `SOTS.Core.DumpSettings`
- `SOTS.Core.DumpLifecycle`
- `SOTS.Core.DumpListeners`
- `SOTS.Core.DumpSaveParticipants`
- `SOTS.Core.BridgeHealth`

Expected signals as you progress:
- Snapshot shows World/PC/Pawn/HUD readiness
- Listener list grows when bridges are enabled
- Save participant list grows when participant bridges are enabled

---

## 4) Coverage map (SPINE/BRIDGE → suite outcomes)

Lifecycle coverage:
- Input: BRIDGE1
- UI: BRIDGE3
- ProfileShared: BRIDGE2
- MissionDirector: BRIDGE9
- AIPerception: BRIDGE11
- GSM: BRIDGE12
- FX: BRIDGE13
- Steam: BRIDGE15
- (Optional future): LightProbePlugin, MMSS, OmniTrace, Debug

Save contract coverage:
- KEM blocks saving: BRIDGE6
- Stats snapshot fragment: BRIDGE7
- SkillTree fragment: BRIDGE8
- Inventory fragment: BRIDGE14
- UI “can save?” query: BRIDGE10
- ProfileShared becomes the composer/IO authority later (Phase 5D)

---

## 5) Deliverables checklist (what “done” means)

Core foundation (Phase 1):
- [ ] SOTS_Core builds cleanly (when you choose to build later)
- [ ] Diagnostics report stable outputs and safe null behavior
- [ ] Settings are default OFF for bridges/dispatch

Suite bridges (Phase 2):
- [ ] Each bridged plugin compiles with SOTS_Core dep (no circular deps)
- [ ] Each bridge can be enabled independently
- [ ] BridgeHealth reports listeners/participants

Adoption (Phase 3):
- [ ] Reparent-only complete
- [ ] Core snapshot populates via ASOTS_* base classes

Full integration (Phase 5):
- [ ] Core-driven mode verified for Input/UI/ProfileShared save orchestration readiness
- [ ] Save Contract used as decoupling boundary (no direct ProfileShared → KEM/Stats/SkillTree/INV coupling)

---

## 6) Rollback policy (always available)
Rollback must remain simple and safe:
- Turn OFF all bridge settings (per plugin)
- Turn OFF Core dispatch/map bridges
- Reparent BPs back to original parents (if you adopted)
- No BP graph edits required for rollback

---

## 7) Notes for “next remaining BRIDGEs” (optional)
If you want suite-wide completeness beyond BRIDGE15:
- LightProbePlugin → Core lifecycle (world/player ready)
- SOTS_MMSS → Core lifecycle + travel
- OmniTrace → Core lifecycle (debug readiness)
- SOTS_Debug → Core lifecycle (debug readiness)
- SOTS_GAS_Plugin → Core lifecycle + optional fragment (only if still authoritative for persistence)

