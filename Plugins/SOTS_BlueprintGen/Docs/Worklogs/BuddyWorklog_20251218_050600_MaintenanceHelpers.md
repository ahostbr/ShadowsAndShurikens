# Buddy Worklog — Maintenance helpers preference

goal
- Make the SPINE_E maintenance helpers (list/describe/compile/save/refresh) the standard surface and flag older apply-only flows for retirement once MCP/bridge fully routes through the new helpers.

what changed
- Updated the discovery-first workflow doc to direct verification through the SPINE_E maintenance helpers and mark legacy apply-only flows as deprecated pending removal when MCP/bridge exclusively use the new path.

files changed
- Plugins/SOTS_BlueprintGen/Docs/BPGen_DiscoveryFirst_Workflow.md

notes + risks/unknowns
- Actual removal of legacy apply-only flows is deferred until MCP/bridge migration is complete; no code removed yet.
- No build/editor run; docs-only.
- Binaries/Intermediate folders for SOTS_BlueprintGen were not present, so no deletion performed.

verification status
- UNVERIFIED (docs-only)

follow-ups / next steps
- Confirm MCP/bridge routes through the maintenance helpers, then remove deprecated apply-only endpoints.
- Add lint/checklist to prevent use of legacy apply-only surfaces once removal date is set.
