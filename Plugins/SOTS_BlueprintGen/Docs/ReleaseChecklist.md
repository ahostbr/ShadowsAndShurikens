# BPGen Release Checklist (SPINE_P)

- [ ] Editor build succeeds with `SOTS_BlueprintGen` + `SOTS_BPGen_Bridge` enabled; Shipping/Test exclude both.
- [ ] Default bind is loopback; safe_mode ON; auth token optional but documented.
- [ ] `SOTS_ALLOW_APPLY` gate respected; dangerous operations require `dangerous_ok`.
- [ ] Templates in `Templates/GraphSpecs`, `Templates/Jobs`, `Templates/Replays` run via MCP + bridge end-to-end.
- [ ] Replay runner (if used) passes with current spec_version.
- [ ] Docs present: GettingStarted, FAQ, ActionReference, ErrorCodes, ResultSchemas, ShippingSafety, ModuleBoundaries, ThirdPartyNotices, Attribution.
- [ ] No Epic sample or non-redistributable assets included; examples are JSON/text only.
- [ ] Binaries/Intermediate cleaned for touched plugins before distribution.
