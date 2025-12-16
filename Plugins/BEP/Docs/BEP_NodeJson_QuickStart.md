# BEP NodeJSON Quick Start

## What it is
Export selected Blueprint graph nodes to compact JSON for auditing or AI pipelines, plus helper flows for comment round-tripping.

## Where to find it
- Menu: **Window → BEP Tools → BEP: Node JSON Panel…**
- Menu actions: export/copy/golden samples/self-check under **Window → BEP Tools**

## Presets
- **SuperCompact**: smallest JSON, compact keys, no titles/comments.
- **Compact**: compact keys with titles, class dict, pin decls.
- **AI Minimal**: human-readable (pretty print), titles, class info, no extras.
- **AI Audit**: verbose pins/unconnected pins/titles/comments for review.

## Output folders
- Root: `<BEPExportRoot>/BEP/NodeJSON/`
- Per selection: `<Blueprint>/<Graph>/SelectedNodes_<UTC>.json` + `SelectedNodes_<UTC>.summary.txt`
- Comments: `<Blueprint>/<Graph>/Comments_<UTC>.json` [+ `.template.csv` when requested]
- Golden samples: `GoldenSamples/Golden_SuperCompact.json`, `Golden_AIAudit.json`, `Golden_Comments.json`, `Golden_Comments.template.csv`, `GoldenSamples.summary.txt`

## Common pitfalls
- **No selection**: select nodes in an open Blueprint graph first.
- **No active graph**: the command is disabled when no Blueprint graph is focused.
- **Caps hit**: counts are clamped to settings caps; summaries/status note when caps trigger.
- **Paths**: exporter falls back to `<Project>/BEP_EXPORTS/` if no custom root is set.
- **Modules missing**: run the **BEP: Node JSON (Self Check)** menu action for diagnostics.
