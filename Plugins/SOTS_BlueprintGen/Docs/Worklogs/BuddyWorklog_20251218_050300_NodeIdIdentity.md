# Buddy Worklog — NodeId identity guidance

goal
- Clarify that NodeId is the sole identity field; avoid duplicate mechanisms and note future migration off NodeComment storage.

what changed
- Updated JSON schema docs to state NodeId is the single supported identity, currently stored via NodeComment, with a planned migration to a dedicated metadata slot.
- Updated discovery-first workflow best practices to reiterate the single-identity rule and planned storage move without changing the field name.

files changed
- Plugins/SOTS_BlueprintGen/Docs/SOTS_BPGen_JSON_Schema.md
- Plugins/SOTS_BlueprintGen/Docs/BPGen_DiscoveryFirst_Workflow.md

notes + risks/unknowns
- Storage migration not yet implemented; existing specs remain unchanged.
- NodeId still lives in NodeComment today; ensure future migration preserves this field name for compatibility.
- No build/editor run; docs-only.
- Binaries/Intermediate folders for SOTS_BlueprintGen were not present, so no deletion performed.

verification status
- UNVERIFIED (docs-only)

follow-ups / next steps
- Implement the dedicated metadata slot for NodeId and migrate storage while keeping the same field name in specs.
- Add linting to flag any parallel identity fields or missing NodeId where expected.
