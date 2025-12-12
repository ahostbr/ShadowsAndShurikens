# BEP – Blueprint Exporter Plugin

BEP is an editor-only plugin that exports Blueprint graphs, input mappings, data assets/tables, and struct schemas to disk for offline analysis (LLMs, Python tooling, diffs). Exports are streamed per asset, batch-GC’d, and include copy/paste snippets for Blueprints. It is intended for technical designers, tools engineers, and analysis scripts that need a static view of gameplay logic and data.

## Installation & Enablement
- Location: `Plugins/BEP` (Editor plugin, enabled for this project).
- UPlugin: `BEP.uplugin` (type `Editor`, loads by default) with `EnhancedInput` enabled for IMC exports.
- Build: `BEP.Build.cs` includes `UnrealEd`, `GraphEditor`, `Kismet`, `AssetRegistry`, Slate/ToolMenus/ContentBrowser so the exporter and UI load inside the editor only.

## Export Layout
Exports are rooted under a sanitized output root (default `C:/BEP_EXPORTS` or `<ProjectSaved>/BEPExport`). Structure per run:
- `BlueprintFlows/` – per-Blueprint graph dumps (`*_Flow.json|csv|txt`) and `Snippets/` subfolder for copy/paste text.
- `IMC/` – Input Mapping Context exports (`*_IMC.json|csv|txt`).
- `Data/` – JSON dumps of data assets/tables/curves, etc.
- `StructSchemas.txt` – all project `UScriptStruct`/`UUserDefinedStruct` property listings under `/Game`.

Example Blueprint flow JSON (truncated):

```jsonc
{
	"asset_type": "Blueprint",
	"asset_name": "BP_SOTS_Player",
	"asset_path": "/Game/SOTS/Characters/BP_SOTS_Player.BP_SOTS_Player",
	"graphs": [
		{
			"graph_name": "EventGraph",
			"graph_type": "Ubergraph",
			"nodes": [
				{
					"title": "Event BeginPlay",
					"class": "K2Node_Event",
					"inputs": {},
					"pins": [
						{
							"name": "Exec",
							"category": "exec",
							"direction": "Out",
							"default_value": "",
							"resolved_value": ""
						}
					]
				}
			],
			"edges": []
		}
	]
}
```

## BlueprintFlows JSON Schema
Top-level fields: `asset_type`, `asset_name`, `asset_path`, `graphs[]`.
Each graph:
- `graph_name`, `graph_type` (Ubergraph | Function | Macro | DelegateSignature).
- `nodes[]` with `title`, `class`, `pins[]`, and `inputs` (map of input pin name → resolved value).
- `edges[]` with `from_node_title`, `from_pin`, `to_node_title`, `to_pin`.
Pins include: `name`, `category`, `direction`, `links[]`, plus `default_value` (literal default) and `resolved_value` (either default or source node/pin description for linked pins).
`inputs` is a map where key = input pin name, value = `resolved_value`.

## Pin Value Resolution
- Unlinked pins: literal default in priority order – `DefaultObject` name, then `DefaultTextValue`, then `DefaultValue` string.
- Linked pins: first linked pin’s owning node title + pin name (omits `.ReturnValue` suffix when empty/ReturnValue). This is first-link only; multi-link resolution is future work.
- `nodes[].inputs` is populated for input pins with non-empty `resolved_value` for quick lookups.

## Graph Coverage
`GetAllGraphsForBlueprint` aggregates Ubergraph, FunctionGraphs, MacroGraphs, and DelegateSignatureGraphs with de-duplication. All exporters and snippet generation iterate this unified set.

## Snippet Exports
- Location: `BlueprintFlows/Snippets/<Blueprint>_<Graph>_Snippet.txt`.
- Content: standard `Begin Object ... End Object` node format produced by `FEdGraphUtilities::ExportNodesToText`, identical to what you get when copying nodes in the Blueprint editor and pasting into a text editor. Select-all + copy from the file and paste directly into a Blueprint graph.

## Input Mapping Context Exports
- Location: `IMC/<ContextName>_IMC.(json|csv|txt)`.
- JSON fields: `asset_type`, `asset_name`, `asset_path`, `Mappings[]` with `Action`, `Key`.

## Data Exports (Data/)
- Serialized via `FJsonObjectConverter::UStructToJsonObjectString` for many data-like assets (DataAssets, DataTables, curves, config structs, etc.).
- Skips: Blueprints/IMCs (handled elsewhere), worlds/levels, audio, meshes, raw anim sequences/montages, and classes with unsupported locator structs.

## StructSchemas.txt
Lists all project structs under `/Game`: each line shows property type and name per struct. Useful for schema diffing and codegen aids.

## Using the BEP Exporter UI
1) Open Window → **BEP Exporter** (Nomad tab).
2) Set Root Path (`/Game/...`).
3) Set Output Root (`C:/BEP_EXPORTS/...` or accept default Saved/BEPExport path).
4) Choose Output Format (Text/JSON/CSV/All).
5) Optionally set Excluded Class Patterns (comma or newline), MaxAssetsPerRun.
6) Click **Run BEP Export**. A warning appears when exporting full `/Game`.
7) BEP never builds a single giant JSON blob in memory: each asset is exported to disk and released before moving on to the next.

## Content Browser Integration
- Right-click a folder → **Export Folder with BEP**. Runs JSON export into `<Saved>/BEPExport/<FolderName>` using the same batching and exclusion rules as the UI/console.

## Console Commands
- `BEP.ExportAll [RootPath] [OutputDir] [Format]`
- `BEP.ExportFolder <RootPath> [Format] [OutputDir]`
Formats: `text|json|csv` (default text). OutputDir defaults to `<Saved>/BEPExport[/<SanitizedRoot>]`. These are editor-only exec commands on the BEP module and are not available in packaged builds.

## Performance & Safety
- Processes assets one-by-one; respects `MaxAssetsPerRun` (default 500).
- Runs `CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true)` every 50 exports per category.
- Default exclusions skip non-logic assets: textures, materials, meshes, raw anim assets, FX systems, audio, and select misc heavy types. User patterns are applied on top.
- BEP never intentionally accumulates all exported JSON in RAM; each asset is written out and released.
- For large projects, narrow RootPath and/or lower MaxAssetsPerRun; avoid full `/Game` unless necessary.

## Extending BEP
- Add new exporters under `BlueprintFlowExporter.cpp` (or separate files) and route them from `ExportAll`.
- Extend JSON schema in the per-node/pin serialization blocks.
- Update default exclusions in `GetDefaultExcludedClassNames` as needed.
- Add UI fields to `SBEPExportPanel` and plumbing in `ExportWithSettings`.

## Known Limitations / Future Work
- `resolved_value` only reports the first linked pin; future work may add multi-link or expression-tree resolution.
- Some asset classes are intentionally skipped (see default exclusions) to keep exports lean.
- Struct schema dump is `/Game` only; engine/plugin structs are omitted by design.
- CSV output escapes fields but still inherits Excel/CSV quirks for very large text values.
