# Buddy Worklog — SPINE_E NodeId reuse

goal
- add idempotent node handling for BPGen graph apply (NodeId create/update/skip)

what changed
- persist bpgen node identity in NodeComment via helpers; added find-by-id helpers
- ApplyGraphSpecToFunction now reuses existing nodes when NodeId matches + updates allowed, skips when updates disallowed, warns when create_or_update=false but NodeId already exists
- link stage skips connections for nodes intentionally skipped and records created/updated/skipped NodeIds in apply result

files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

notes + risks/unknowns
- repurposing NodeComment for NodeId may clash with existing comments; no other storage available yet
- graph clearing is removed; legacy nodes not mentioned in specs now persist (intentional for idempotent passes)
- existing connections on reused nodes are not broken unless link flags request it

verification status
- UNVERIFIED (no build/editor run)

cleanup
- Plugins/SOTS_BlueprintGen/Binaries not present (no deletion needed)
- Plugins/SOTS_BlueprintGen/Intermediate not present (no deletion needed)

follow-ups / next steps
- add inspector/describe surfaces + compile/refresh helpers for SPINE_E
- consider a dedicated metadata slot for NodeId to avoid comment collision
- extend docs/examples for create-or-update flow
