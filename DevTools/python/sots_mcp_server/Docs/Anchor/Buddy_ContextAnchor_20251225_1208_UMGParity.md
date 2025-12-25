Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1208 | Plugin: sots_mcp_server | Pass/Topic: UMGParity | Owner: Buddy
Scope: Add BPGen-backed UMG authoring parity in unified MCP (VibeUE compat)

DONE
- Added BPGen UMG tools in DevTools/python/sots_mcp_server/server.py:
  - bpgen_ensure_widget (ensure_widget)
  - bpgen_set_widget_properties (set_widget_properties)
  - bpgen_ensure_binding (ensure_binding)
- Implemented manage_umg_widget actions:
  - add_component → bpgen_ensure_widget
  - set_property → bpgen_set_widget_properties
- Added best-effort UE ImportText conversion helpers for common property value shapes.
- Updated parity matrix entry for manage_umg_widget to Partial.

VERIFIED
- UNVERIFIED (no UE/bridge run). Syntax-only check performed.

UNVERIFIED / ASSUMPTIONS
- Assumes component_type like "Button" maps to class path "/Script/UMG.Button" when no explicit path provided.
- Assumes ImportText conversion covers enough common cases; complex types may still require explicit ImportText strings.

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

NEXT (Ryan)
- Run UE with BPGen bridge enabled and call manage_umg_widget.add_component to create a widget under a known panel; confirm widget tree + variable flag.
- Call manage_umg_widget.set_property for a few properties (including Slot.*) using explicit ImportText strings when needed; confirm ImportText parsing succeeds.
- Validate bpgen_ensure_binding directly for a Text binding; confirm binding entry and optional graph apply.

ROLLBACK
- Revert the two touched files above (or git revert the change).
