# BPGen Module Boundaries (SPINE_P)

- **Module types**: `SOTS_BlueprintGen` is Editor-only; no runtime modules. `SOTS_BPGen_Bridge` is Editor-only and depends on BPGen.
- **Editor-only includes**: Keep UnrealEd/BlueprintEditor headers behind Editor modules; no runtime references to editor headers.
- **Apply gating**: Mutations are still guarded by env var `SOTS_ALLOW_APPLY=1` in BPGen API; bridge must not bypass it.
- **Network scope**: Bridge defaults to loopback-only. Do not enable remote binds for packaged builds unless explicitly opted in via config.
- **Content**: Plugins do not ship assets. Examples/templates remain JSON/text only (redistributable).
- **Derived from VibeUE**: Structure mirrors VibeUE’s MCP/tooling layout but renamed for BPGen; keep 1:1 parity where possible.
