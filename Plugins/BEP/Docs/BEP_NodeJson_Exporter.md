# BEP NodeJSON Exporter

## Output layout
- Root: `<BEPExportRoot>/BEP/NodeJSON/`
- Selection exports: `<Blueprint>/<Graph>/SelectedNodes_<UTC>.json` + `SelectedNodes_<UTC>.summary.txt`
- Comment exports: `<Blueprint>/<Graph>/Comments_<UTC>.json` [+ `Comments_<UTC>.template.csv` when requested]
- Golden samples: `GoldenSamples/Golden_SuperCompact.json`, `Golden_AIAudit.json`, `Golden_Comments.json`, `Golden_Comments.template.csv`, `GoldenSamples.summary.txt`
- UTC format: `YYYYMMDD_HHMMSS`. Filenames and folders are sanitized for safety.

## Panel actions
- **Quick Presets**: SuperCompact, Compact, AI Minimal, AI Audit (applies + saves settings).
- **Export JSON (Selection)**: writes Node JSON + summary file.
- **Copy JSON (Selection)**: copies JSON to clipboard.
- **Export JSON + Copy Path**: writes JSON + summary, copies absolute JSON path.
- **Preview / Dry Run**: counts nodes/edges with caps applied (no files written).
- **Export Comment JSON**: writes comment JSON.
- **Export Comment JSON + Template**: writes comment JSON + template CSV (GUIDs + empty comments).
- **Import Comments CSV…**: spawns comment boxes from a CSV (undoable).
- **BEP Golden Samples**: writes fixed golden outputs + consolidated summary.
- **Open/Copy**: open output folder, open last file, copy last saved path, copy last summary.

## Caps and safety
- Settings: `MaxNodesHardCap` (default 500), `MaxEdgesHardCap` (default 2000), `bWarnOnCapHit`.
- Preview shows seed selection, expanded nodes, final nodes, exec/data edges, depths, and whether caps would hit.
- When caps hit: deterministic clamp (sorted by id) then edges rebuilt and clamped; status + summary note the clamp.

## Summary file format (`SelectedNodes_*.summary.txt`)
- Timestamp (UTC)
- Preset + key toggles (compact/pretty/class_dict/legend)
- ExecDepth / DataDepth
- SeedSelection, ExpandedNodes, FinalNodes
- ExecEdges, DataEdges, TotalEdges
- Cap values and whether hit
- Engine tag string
- Absolute JSON path

## Golden samples
- Runs SuperCompact + AIAudit + Comments into `GoldenSamples/` with the consolidated `GoldenSamples.summary.txt`.
- Intended for regression diffs; overwrites on each run; uses current selection and settings caps.

## Troubleshooting
- **No active graph / selection**: focus the Blueprint graph and select nodes.
- **Caps hit**: raise caps in settings or reduce selection; check summaries for clamp info.
- **Paths**: exporter falls back to `<Project>/BEP_EXPORTS/` if no user setting is defined.
- **CSV quoting**: wrap comma/quote-containing fields; double embedded quotes.
