# BPGen Snippet Runner & Coverage (DevTools)

This document summarizes the DevTools-side helpers for running BPGen snippet packs via the Unreal BPGen commandlet and for reporting snippet coverage.

## Files and Modules

- `DevTools/python/sots_bpgen_runner.py`
  - Runs individual BPGen jobs or entire snippet packs by invoking `UnrealEditor-Cmd.exe` with `SOTS_BPGenBuildCommandlet`.
  - Uses a placeholder `DEFAULT_UE_CMD` (update to your local Unreal install).
  - Attempts to infer the project path as `../ShadowsAndShurikens.uproject` (override by editing if needed).
  - Logs per-run details when `log_dir` is provided (default used by CLI: `DevTools/bpgen_snippets/logs`).

- `DevTools/python/sots_bpgen_snippets.py`
  - Discovers packs under `DevTools/bpgen_snippets/packs/`.
  - Loads `pack_meta.json` and enumerates snippets (`*_Job.json` + optional `*_GraphSpec.json`).
  - Prints discoveries and warnings; never fails silently.

- `DevTools/python/sots_bpgen_coverage.py`
  - Loads the central snippet index (`DevTools/bpgen_snippets/index/snippet_index.json`).
  - Builds a node-class coverage map and prints a human-readable report.
  - Includes notes for future auto-indexing and UE integration.

- `DevTools/python/sots_tools.py`
  - New subcommands:
    - `bpgen_packs` — list available snippet packs.
    - `bpgen_run_pack <pack_name> [--ue=PATH/TO/UnrealEditor-Cmd.exe]` — run all snippets in a pack.
    - `bpgen_coverage` — print node coverage from `snippet_index.json`.

## Snippet Repository Structure

- Root: `DevTools/bpgen_snippets/`
  - `packs/` — each pack has `pack_meta.json`, `jobs/`, `graphspecs/`.
    - Example starter pack: `packs/common_basics/pack_meta.json`
  - `index/` — central index for coverage:
    - `snippet_index.json` (version `0.1.0`, empty `snippets` list by default)
  - `logs/` — default location for BPGen run logs.

## Usage Examples

- List packs:
  ```
  python sots_tools.py bpgen_packs
  ```

- Run a pack (update UE path as needed):
  ```
  python sots_tools.py bpgen_run_pack common_basics --ue "C:/Path/To/UnrealEditor-Cmd.exe"
  ```
  Logs will be written under `DevTools/bpgen_snippets/logs`.

- Coverage report:
  ```
  python sots_tools.py bpgen_coverage
  ```

## Notes / TODO
- Set `DEFAULT_UE_CMD` in `sots_bpgen_runner.py` to your local UnrealEditor-Cmd.exe path.
- Confirm the inferred `.uproject` path or adjust in `sots_bpgen_runner.py`.
- `snippet_index.json` is manual/semi-manual today; future work can auto-update it by scanning packs and UE node exports to track missing coverage.
