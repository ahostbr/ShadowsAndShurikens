# BEP NodeJSON Auto Comments Workflow

## Golden path
1. Select nodes in the Blueprint graph.
2. Open **BEP: Node JSON Panel…**.
3. Click **Export Comment JSON** (or **Export Comment JSON + Template** to also write a CSV with GUIDs).
4. Feed the Comment JSON to your AI using one of the prompt templates below.
5. Paste AI output as CSV (`node_guid,comment`).
6. Click **Import Comments CSV…** to spawn comment boxes near the matching nodes.

## Comment JSON schema (readable)
- `schema_version` (int)
- `engine` (string)
- `asset` (optional asset path)
- `graph` (optional graph name)
- `exported_utc` (ISO8601)
- `nodes`: array of
  - `id` (NodeGuid, DigitsWithHyphens)
  - `x`,`y` (ints, node position)
  - `title` (optional)
  - `class` (optional class path)
  - `comment` (optional existing comment)

## CSV schema for import
- Required columns: `node_guid`, `comment` (case-insensitive). If no header, first column = guid, second = comment.
- Extra columns are ignored.
- Quotes are supported for commas/quotes in comments.

Example:
```
node_guid,comment
2e5f3d38-1234-4a8a-9f0c-111122223333,"Add clamp before divide"
58ab44ee-9876-4c1d-8fd2-abcdefabcdef,Cache result for reuse
```

## Prompt templates

**Prompt A: Generate CSV comments from Comment JSON**
```
You are assisting with Blueprint readability.
Input: a Comment JSON describing selected nodes (positions, optional titles/class/comment).
Task: produce a CSV with columns node_guid,comment. Keep comments concise, actionable, and neutral. Do not invent functionality; ask for TODO where unclear. Escape commas/quotes using CSV rules.
Output: raw CSV only.
```

**Prompt B: Grouped comment boxes by topic**
```
You are an engineering reviewer creating grouped comment boxes.
Input: Comment JSON of selected Blueprint nodes.
Task: cluster nodes by purpose (e.g., input handling, effects, AI). For each cluster, emit multiple CSV rows sharing the same group label in the comment prefix. Format: node_guid,comment where comment starts with [GroupName] and a short note. Escape commas/quotes per CSV rules. Keep comments under 120 characters.
Output: raw CSV only.
```

## Troubleshooting
- **No active graph / no selection**: focus the Blueprint graph and select nodes.
- **Missing NodeGuid**: nodes without valid GUIDs are skipped; refresh GUIDs in Blueprint if needed.
- **Caps hit**: comment creation caps at 500 per run; skipped rows listed in the status box.
- **CSV commas/quotes**: wrap fields containing commas or quotes; double embedded quotes.
