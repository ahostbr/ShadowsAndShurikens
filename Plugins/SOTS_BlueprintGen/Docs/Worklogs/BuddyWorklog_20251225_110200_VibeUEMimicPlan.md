Send2SOTS
# Buddy Worklog — 2025-12-25 — Plan: VibeUE Parity in BPGen

## Goal
- Produce an evidence-based plan to recreate VibeUE’s MCP capabilities inside BPGen (SOTS_BlueprintGen + SOTS_BPGen_Bridge), targeting full tool-surface parity and equivalent behavior.

## Scope
- Plugins/Systems: `SOTS_BlueprintGen`, `SOTS_BPGen_Bridge`, existing Python MCP server(s)
- Constraints: Planning only (no code changes in this pass)

## Evidence used
- VibeUE tool surface & architecture: `Plugins/VibeUE/README.md` (canonical 44 tools + TCP server on port 55557)
- BPGen tool surface & architecture:
  - `Plugins/SOTS_BlueprintGen/Docs/API/BPGen_ActionReference.md`
  - `Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md`
  - `Plugins/SOTS_BlueprintGen/Docs/GettingStarted.md`

## Output
- A phased implementation plan (continued below) including a parity matrix, architecture options, and sequencing.

## Continuation: Ground Truth Entry Surfaces (SOTS)

### Single MCP entrypoint (canonical)
- The suite’s canonical MCP entrypoint is the unified stdio server:
  - `DevTools/python/sots_mcp_server/server.py`
- That server intentionally exposes **three tool families** under one MCP connection:
  1) Core `sots_*` workspace/report/diagnostic tools (read-only by default)
  2) `sots_agents_*` tools (prompt runner surface; legacy aliases exist)
  3) `bpgen_*` (and `manage_bpgen` alias) forwarding to the BPGen bridge

### BPGen contract (canonical)
- Canonical BPGen “one true entry” (Unreal-side): `USOTS_BPGenAPI::ExecuteAction|ExecuteBatch` (JSON in/out)
- Canonical BPGen “bridge” transport: SOTS_BPGen_Bridge TCP NDJSON (tool/action/params/request_id)
- Canonical DevTools/MCP forwarding: `bpgen_*` MCP tools call the bridge; MCP apply is gated.

## Continuation: Gating / Safety Expectations

### Apply gates
- Global mutation gate: `SOTS_ALLOW_APPLY`
- BPGen apply gate: `SOTS_ALLOW_BPGEN_APPLY` (defaults to `SOTS_ALLOW_APPLY`)
- DevTools runner gate: `SOTS_ALLOW_DEVTOOLS_RUN` (defaults allow; read-only scripts can bypass when off)

### Dry-run policy
- BPGen mutation wrappers should support `dry_run` wherever practical (canonicalize → dry_run → apply → describe/list → compile/save).

## Continuation: What “VibeUE parity” should mean (explicit target)

### Two possible targets (must pick one)
1) **Upstream VibeUE canonical surface** (documented as ~44 tools)
2) **SOTS-integrated VibeUE surface** (the subset currently exposed/used in this project)

Decision is required because “100% parity” differs radically by target.

### Parity scope buckets
For each VibeUE tool/behavior, classify into exactly one bucket:
- **BPGen-native**: already expressible via existing BPGen actions (`apply_graph_spec`, `delete_node/link`, `replace_node`, `ensure_*`, `compile/save`, etc.)
- **BPGen-extension**: requires adding new bridge actions (Unreal-side services + bridge protocol + MCP wrapper)
- **DevTools/MCP-only**: belongs on the unified MCP server but not in BPGen (e.g., local file/report utilities)
- **Out of scope**: cannot/should not be automated due to safety/ownership constraints

## Continuation: Mapping VibeUE prompt workflows onto BPGen

### VibeUE “prompt grammar” to preserve
VibeUE’s test prompts consistently encode these workflows:
- **Discover → Create (exact variant)** using a stable key (VibeUE uses `spawner_key`)
- **Describe → Connect** using discovered node ids + pin names
- **User verification checkpoint** (open editor, confirm pins/graph, confirm compile success)
- **Cleanup last** (do not delete assets until review)

### BPGen equivalent workflow
- BPGen already aligns well to a spec-first workflow:
  - `canonicalize_graph_spec` → `apply_graph_spec` → `describe/list` → `compile_blueprint` → `save_blueprint`
- The parity plan should preserve the *intent* of VibeUE prompts, even if the mechanics shift from node-by-node RPC to GraphSpec application.

## Continuation: Phased Implementation Plan (expanded)

### Phase 0 — Freeze the target + create a parity matrix
- Collect the definitive VibeUE tool list (target option 1 or 2).
- Create a parity table with columns:
  - VibeUE tool name / action
  - Required inputs/outputs
  - Determinism needs (stable ids, ordering)
  - Safety classification (read-only vs mutating)
  - Proposed implementation bucket (BPGen-native / BPGen-extension / DevTools-only / out-of-scope)

### Phase 1 — Prompt-pack parity (no new Unreal bridge actions)
Goal: achieve maximum “felt parity” using the unified MCP server + existing BPGen actions.
- Port the *test prompt structure* (Prereqs/Setup/Steps/Expected/Cleanup) into a SOTS prompt pack.
- Convert VibeUE “node-by-node” steps into BPGen GraphSpec recipes.
- Standardize the verification sequence:
  - canonicalize → (optional dry_run) → apply → describe/list → compile/save
- Add explicit failure modes: what to do on link failure, node spawn failure, or compile failure.

### Phase 2 — Add missing BPGen actions required for Blueprint graph parity
Goal: cover any Blueprint graph operations that VibeUE exposes but BPGen cannot express yet.
- For each gap, decide whether it should be:
  - a new **bridge action** (Unreal-side implementation + NDJSON contract + MCP wrapper), or
  - a **GraphSpec feature** (schema/version bump + canonicalize rules).
- Keep determinism + idempotency rules aligned with SPINE_N expectations.

### Phase 3 — UMG + Asset parity (likely requires Unreal-side services)
Goal: address VibeUE families like `manage_umg_widget` and `manage_asset`.
- Current BPGen contract surfaces are primarily Blueprint graph-centric; full UMG/Asset parity likely requires adding Unreal-side editor services comparable to VibeUE’s.
- If approved, add a dedicated set of BPGen bridge actions for:
  - widget hierarchy add/remove + slot property ops
  - asset search/open/duplicate/import/export
- Enforce the same SOTS gating model (read-only by default; apply requires explicit env gates).

### Phase 4 — Unify the client experience under the single MCP server
Goal: “one MCP server to rule them all” for SOTS usage.
- Ensure all new parity tools are registered on `DevTools/python/sots_mcp_server/server.py` (canonical).
- Maintain aliases for legacy naming, but keep `sots_help` as the source of truth.

## Continuation: Acceptance Criteria (definition of done)
- Parity matrix exists and each tool is assigned to a bucket.
- Prompt pack exists with smoke tests that:
  - create assets in a dedicated folder
  - apply changes deterministically
  - compile/save when allowed
  - include a user verification checkpoint
- Gating is respected:
  - no mutations when `SOTS_ALLOW_APPLY=0`
  - `dry_run` produces a non-mutating preview for key operations

## Verification
- UNVERIFIED: No build/editor validation performed (planning only).

## Notes
- BPGen already has safety/audit/batch/session primitives; VibeUE parity likely requires adding new bridge actions/services beyond GraphSpec apply.
