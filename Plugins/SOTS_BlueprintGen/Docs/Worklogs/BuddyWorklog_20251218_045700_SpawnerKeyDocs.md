# Buddy Worklog — SpawnerKey-first spec guidance

goal
- Nudge BPGen specs to prefer `spawner_key` + `FunctionPath`, mark NodeType as legacy with a short sunset.

what changed
- Updated JSON schema doc to surface `SpawnerKey`, `PreferSpawnerKey`, NodeId flags, and add a sunset notice that NodeType is legacy except for the small synthetic set; examples now include `SpawnerKey` + `FunctionPath` for PrintString.
- Refreshed discovery-first workflow best practices to require spawner_key + FunctionPath and restrict NodeType to synthetic nodes during the sunset.

files changed
- Plugins/SOTS_BlueprintGen/Docs/SOTS_BPGen_JSON_Schema.md
- Plugins/SOTS_BlueprintGen/Docs/BPGen_DiscoveryFirst_Workflow.md

notes + risks/unknowns
- NodeType remains in structs for now; removal will need a follow-up sweep once specs migrate.
- Examples still include NodeType on entry/result for clarity; migration off NodeType everywhere else depends on spec updates.
- No build/editor run; docs only.
- Binaries/Intermediate folders for SOTS_BlueprintGen were not present, so no deletion needed.

verification status
- UNVERIFIED (docs only; no build/editor run)

follow-ups / next steps
- Update any remaining example GraphSpec JSON files to mirror the spawner_key-first guidance.
- Communicate the short NodeType sunset to spec authors and remove NodeType acceptance after migration.
- Consider an automated lint to flag NodeType usage outside the synthetic set.
