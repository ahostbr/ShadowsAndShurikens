# BPGen Replay & Regression Harness (SPINE_I)

Run scripted NDJSON sessions against the BPGen bridge to generate deterministic reports and diffs.

## Running a replay
1) Start the UE editor with SOTS_BPGen_Bridge enabled.
2) Execute the runner:
```
python DevTools/python/bpgen_replay_runner.py --replay DevTools/replays/BPGen/01_function_print_string.jsonl --out DevTools/reports/BPGen/01_function_print_string_report.json --diff DevTools/reports/BPGen/01_function_print_string_diff.txt --expect DevTools/replays/BPGen/01_function_print_string_expected.json
```
- `--host/--port` override bridge location (default 127.0.0.1:55557).
- Reports always include `summary.ok/failed`, action histogram, and error_code histogram.
- Params automatically set `client_protocol_version=1.0` for protocol gating.
- If the bridge requires auth, set `SOTS_BPGEN_AUTH_TOKEN` so DevTools auto-injects `params.auth_token`.

## Adding a new golden replay
1) Create an NDJSON file under `DevTools/replays/BPGen/` with one BPGen request per line (`tool:"bpgen"`, `action`, `params`).
2) Run the replay once to capture a report.
3) Copy the report into `<name>_expected.json` (keeps future runs diffable).
4) Keep requests minimal and deterministic (sorted outputs, stable node ids).

## Interpreting diffs
- Reports compare `action`, `ok`, `error_code`, `errors`, `warnings` per response.
- A non-empty diff file means behavior changed; rerun `describe_node` / `refresh_nodes` / `compile_blueprint` as quick repair steps.

## Included replays (coverage sweep)
- 01_function_print_string: function ensure + graph apply + inspect + compile/save.
- 02_eventgraph_custom_event: event graph patch and verification.
- 03_macro_create_and_use: macro creation plus function caller.
- 04_construction_script_patch: construction script edits.
- 05_umg_widget_tree_and_binding: widget ensure + binding graph apply.
- 06_refactor_by_node_id: replace/delete primitives + link cleanup.
