# Recovery and Verification

Checklist after apply
- `bpgen_list` to confirm node count and ids.
- `bpgen_describe` on each critical `node_id` to confirm pins, defaults, and links.
- `bpgen_compile` and review warnings; then `bpgen_save` if clean.

Common fixes
- Missing pin: describe the node → adjust pin name in `graph_spec` → re-apply.
- Schema rejected link: check pin types in describe; add/adjust conversion node; re-apply.
- Wrong spawner: rediscover and swap `spawner_key`, keep same `node_id`, re-apply.
- Stale pins after type change: `bpgen_refresh` → re-apply → compile.

Error-handling pattern
1) Capture response `errors`/`warnings`.
2) Re-run `bpgen_list`/`bpgen_describe` to see real graph state.
3) Update spec (pin names/types/spawner_key) and re-apply.
4) Finish with compile + save.
