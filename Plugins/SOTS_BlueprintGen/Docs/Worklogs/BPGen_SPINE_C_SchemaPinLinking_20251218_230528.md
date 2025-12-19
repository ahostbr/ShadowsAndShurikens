Goal
- SPINE_C: use K2 schema-first pin connections with optional break policies; keep legacy MakeLinkTo as fallback.

What changed
- FSOTS_BPGenGraphLink now supports link policies: bBreakExistingFrom, bBreakExistingTo, bUseSchema.
- Added schema-first connector helper that tries UEdGraphSchema::TryCreateConnection, applies optional break flags, and falls back to MakeLinkTo with warnings.
- ApplyGraphSpecToFunction now routes link creation through the helper; legacy behavior preserved via fallback.

Files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

Notes / risks / unknowns
- Schema rejection now emits warnings; no build/run performed.
- bUseSchema defaults to true; callers can disable per link if needed.

Verification status
- UNVERIFIED (no build/run).

Cleanup
- Pending (delete Binaries/Intermediate for SOTS_BlueprintGen).

Follow-ups / next steps
- Extend pin resolution diagnostics (list available pins when not found).
- Add debug dump helper gated by editor flag if needed.
- Proceed to SPINE_E/F/G for inspection, bridge, MCP surface.
