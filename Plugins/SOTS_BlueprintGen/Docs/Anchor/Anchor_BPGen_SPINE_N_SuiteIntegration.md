[CONTEXT_ANCHOR]
ID: 20251219_000000 | Plugin: SOTS_BlueprintGen | Pass/Topic: SPINE_N Suite Integration | Owner: Buddy
Scope: BPGen One True Entry JSON API + docs/prompts/job schema

DONE
- Added `USOTS_BPGenAPI::ExecuteAction/ExecuteBatch` JSON envelope entry (apply/canonicalize graph specs, function skeleton, node/link edits, struct/enum assets) with env gate `SOTS_ALLOW_APPLY`.
- Authored integration contract, shipping safety, release notes for BPGen and BPGen_Bridge.
- Added DevTools job schema + submit/collect helpers and prompt index.

VERIFIED
- Code paths unbuilt; structural review only (UNVERIFIED for runtime/editor).

UNVERIFIED / ASSUMPTIONS
- MCP/bridge parity assumed via shared action names; no editor run.
- Env gate assumed sufficient for apply lockdown.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Public/SOTS_BPGenAPI.h
- Plugins/SOTS_BlueprintGen/Private/SOTS_BPGenAPI.cpp
- Plugins/SOTS_BlueprintGen/Docs/Contracts/SOTS_BPGen_IntegrationContract.md
- Plugins/SOTS_BlueprintGen/Docs/ShippingSafety.md
- Plugins/SOTS_BlueprintGen/Docs/ReleaseNotes_BPGen.md
- Plugins/SOTS_BPGen_Bridge/Docs/ShippingSafety.md
- Plugins/SOTS_BPGen_Bridge/Docs/ReleaseNotes_BPGenBridge.md
- DevTools/prompts/BPGen/INDEX.md
- DevTools/docs/BPGenJobSchema.md
- DevTools/python/bpgen_submit_job.py
- DevTools/python/bpgen_collect_reports.py

NEXT (Ryan)
- Build editor; run MCP/bridge roundtrip using ExecuteBatch with canonicalize+apply; confirm graph materializes.
- Confirm SOTS_ALLOW_APPLY gate blocks mutations when unset.
- Ensure prompt pack references new action names; run a sample job through DevTools inbox.

ROLLBACK
- Remove `SOTS_BPGenAPI.*`; delete added docs/prompts/scripts; revert commit.
