Goal
- Start SPINE_A groundwork: add discovery descriptor structs to BPGen types.

What changed
- Added discovery structs (pin descriptors, spawner descriptor, discovery result) to SOTS_BPGenTypes.h for SPINE_A node discovery payloads.

Files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h

Notes / risks / unknowns
- Discovery behavior not implemented yet; only data types added.
- JSON field names are documented inline but unverified against downstream callers.

Verification status
- UNVERIFIED (no build/run; types only).

Cleanup
- Deleted Plugins/SOTS_BlueprintGen/Binaries and Plugins/SOTS_BlueprintGen/Intermediate.

Follow-ups / next steps
- Implement discovery surface (library/API) that returns FSOTS_BPGenNodeDiscoveryResult populated from node spawner search.
- Wire search inputs (search text, max results, optional blueprint context) and optional pin inspection.
- Add tests/smoke to confirm JSON parity with SPINE_A expectations.
