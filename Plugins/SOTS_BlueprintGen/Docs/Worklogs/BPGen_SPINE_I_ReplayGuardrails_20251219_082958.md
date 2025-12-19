# BPGen SPINE I - Replay Guardrails (2025-12-19)

Scope
- Added bridge guardrails: auth token, rate limit, request size error codes; dry_run previews for mutating actions.
- Added MCP wrappers for delete/replace and DevTools auth-token injection.
- Updated protocol + replay docs for regression workflows.

VibeUE refs
- Plugins/VibeUE/Source/VibeUE/Private/Bridge.cpp (protocol/version response pattern).
- Plugins/VibeUE/Source/VibeUE/Private/Services/Blueprint/BlueprintNodeService.cpp (transaction/delete patterns used as reference).

Notes
- Dry-run responses include preview metadata and skip mutations.
- Auth token config: DefaultEngine.ini [SOTS_BPGen_Bridge] AuthToken or SOTS_BPGEN_AUTH_TOKEN env.
