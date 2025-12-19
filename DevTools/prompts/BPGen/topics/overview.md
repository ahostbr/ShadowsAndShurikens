# BPGen Overview

Workflow loop
1) `bpgen_ping` to confirm bridge reachability.
2) Discover nodes: `bpgen_discover(blueprint_asset_path, search_text, max_results, include_pins?)`.
3) Build specs with `spawner_key` + `node_id` for every important node.
4) Apply graph: `bpgen_apply(blueprint_asset_path, function_name, graph_spec)`.
5) Verify: `bpgen_list` then `bpgen_describe` for each `node_id`.
6) Compile + save: `bpgen_compile` then `bpgen_save`.

Ground rules
- Use stable `node_id` everywhere; re-apply should update, not duplicate.
- Prefer `spawner_key`; keep `function_path` in spec as debug fallback.
- Use schema-first links; fix pin names/types via describe output.
- If something fails, re-discover and re-describe before retrying.
