# BEP NodeJSON Auto Comments

## Workflow (Golden Path)
1. In the Blueprint graph, select the nodes you want annotated.
2. Open the **BEP Node JSON** tab (Window → Tools → BEP: Node JSON Panel…).
3. Click **Export Comment JSON**. This writes a light JSON describing the selected nodes (stable GUIDs, positions, optional titles/class/comments).
  - Use **Export Comment JSON + Template** to also emit a CSV template with all node GUIDs and empty comments.
4. Use that JSON in your external AI prompt to generate comments.
5. AI returns a CSV with columns `node_guid,comment`.
6. Click **Import Comments CSV…** and pick the CSV. BEP spawns comment boxes near the matching nodes and selects them for quick adjustment.

## Comment JSON schema
```json
{
  "schema_version": 1,
  "engine": "UE5.x",
  "asset": "<optional asset path>",
  "graph": "<optional graph name>",
  "exported_utc": "<ISO8601 UTC timestamp>",
  "nodes": [
    {
      "id": "<NodeGuid DigitsWithHyphens>",
      "x": <int>,
      "y": <int>,
      "title": "<optional node title>",
      "class": "<optional class path>",
      "comment": "<optional existing comment>"
    }
  ]
}
```
- File naming: `Comments_<UTC>.json` (UTC = `YYYYMMDD_HHMMSS`).
- Output folder: `<BEPExportRoot>/BEP/NodeJSON/<Blueprint>/<Graph>/`
- Template helper: `Comments_<UTC>.template.csv` (same folder) when using the template button.

## CSV schema for import
- Required columns: `node_guid`, `comment` (case-insensitive). If no header, first column = guid, second = comment.
- Extra columns are ignored.
- Quotes are supported for commas in comments.

Example:
```
node_guid,comment
2e5f3d38-1234-4a8a-9f0c-111122223333,"Add clamp before divide"
58ab44ee-9876-4c1d-8fd2-abcdefabcdef,Cache result for reuse
```

## Behavior and safety notes
- Deterministic ordering: nodes sorted by Guid in the export.
- Export templating: template CSV lists all GUIDs + empty comment column; quoting is RFC4180-friendly.
- Import caps creation to 500 comments per run; skipped rows are listed in the panel status.
- Import is undoable (single scoped transaction) and selects all created comment boxes for easy repositioning.
- Failures (no graph, no selection, invalid guids, missing nodes) surface both in the panel status and via toasts.

## Troubleshooting
- **No active graph / no selection**: ensure a Blueprint graph window is focused and nodes are selected.
- **Missing NodeGuid**: nodes without valid GUIDs are skipped; refresh GUIDs in the Blueprint if needed.
- **Caps hit**: comment creation is capped (500); rerun with smaller batches if truncated.
- **CSV commas/quotes**: wrap comment fields containing commas or quotes; double any embedded quotes.

## Tips
- Keep AI output RFC4180-ish: quote fields that contain commas or quotes (double up quotes inside quoted fields).
- Ensure nodes retain valid `NodeGuid` values; unnamed/invalid nodes will be skipped.
