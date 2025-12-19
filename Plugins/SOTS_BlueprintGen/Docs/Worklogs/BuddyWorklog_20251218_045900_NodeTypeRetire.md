# Buddy Worklog — Retire NodeType helpers

goal
- Remove legacy NodeType-only spawn helpers for standard K2 nodes now that spawner parity exists.
- Keep only the minimal synthetic nodes (knot/select/dynamic-cast) and force spawner_key for everything else.

what changed
- Deleted bespoke Spawn* helpers for standard K2 nodes (call function, array/function calls, variables, branch, loops/macros, switch/literal helpers, delegate/multigate, make/break struct, events, gameplay tag/name literals).
- NodeType fallback now only permits knot/select/dynamic-cast; other NodeType specs log a warning requiring spawner_key.
- Removed unused helpers (ResolveStructPath, StandardMacro paths, etc.) tied to the deprecated NodeType chain.

files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

notes + risks/unknowns
- Specs still using NodeType without spawner_key will now skip with warnings; migrate to spawner_key.
- Remaining synthetic nodes are knot/select/dynamic-cast only; confirm no other gaps need bespoke handling.
- No build/editor run; compilation impact unverified.

verification status
- UNVERIFIED (no build/editor run)

cleanup
- Plugins/SOTS_BlueprintGen/Binaries not present (nothing to delete)
- Plugins/SOTS_BlueprintGen/Intermediate not present (nothing to delete)

follow-ups / next steps
- Sweep specs for NodeType usage and convert to spawner_key.
- Validate in-editor that minimal synthetic set suffices; add targeted helpers only if spawner cannot cover a case.
- Consider moving NodeId storage off NodeComment separately.
