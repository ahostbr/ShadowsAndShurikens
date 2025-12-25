Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1102 | Plugin: SOTS_BlueprintGen | Pass/Topic: VIBEUE_PARITY_PLAN | Owner: Buddy
Scope: Evidence + planning for full VibeUE MCP parity implemented inside BPGen plugins

DONE
- Collected evidence for VibeUE vs BPGen tool surfaces and transport:
  - VibeUE: canonical 44-tool surface; TCP plugin listener documented on port 55557.
  - BPGen: NDJSON bridge protocol with actions including apply_graph_spec, ensure_*, UMG ensure, introspection, graph_edits, batch/sessions, safety/audit.
- Extended the parity plan directly in `BuddyWorklog_20251225_110200_VibeUEMimicPlan.md`, including:
  - Unified MCP single-entrypoint context (three tool families: `sots_*`, `sots_agents_*`, `bpgen_*`)
  - Gating/dry-run expectations (`SOTS_ALLOW_APPLY`, `SOTS_ALLOW_BPGEN_APPLY`, `SOTS_ALLOW_DEVTOOLS_RUN`)
  - Phased plan + explicit parity target definition and acceptance criteria

VERIFIED
- Documentation evidence exists in:
  - `Plugins/VibeUE/README.md`
  - `DevTools/python/sots_mcp_server/Docs/MCP_SINGLE_SERVER_PARITY.md`
  - `Plugins/SOTS_BlueprintGen/Docs/Contracts/SOTS_BPGen_IntegrationContract.md`
  - `Plugins/SOTS_BlueprintGen/Docs/API/BPGen_ActionReference.md`
  - `Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md`

UNVERIFIED / ASSUMPTIONS
- Exact 1:1 runtime parity is feasible without compromising BPGen safety model.
- Any missing VibeUE behaviors can be expressed either as BPGen recipes or new bridge actions.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BuddyWorklog_20251225_110200_VibeUEMimicPlan.md
- Plugins/SOTS_BlueprintGen/Docs/Anchor/Buddy_ContextAnchor_20251225_1102_VibeUEMimicPlan.md

NEXT (Ryan)
- Confirm which “VibeUE” target we mean: upstream canonical 44 tools vs the current SOTS-integrated reduced surface.
- Approve whether to implement parity via (A) Python shim only or (B) new BPGen bridge actions/services.

ROLLBACK
- No code changes in this pass; delete this plan worklog/anchor if undesired.
