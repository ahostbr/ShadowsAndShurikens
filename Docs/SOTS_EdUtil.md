# SOTS_EdUtil

Editor-only plugin providing lightweight utilities. Feature set includes a generic JSON-to-DataAsset importer (Blueprint/CallInEditor) and a pre-made importer tab (Slate widget).

## Module
- Plugin: `SOTS_EdUtil`
- Module type: Editor (no runtime dependency)
- Content: none (`CanContainContent=false`)

## Generic JSON Importer
- Class: `USOTS_GenericJsonImporter` (Blueprint function library)
- Function: `ImportJson(UDataAsset* TargetAsset, FString JsonFilePath, FName ArrayPropertyName = NAME_None, bool bMarkDirty = true)`
- Callable: `CallInEditor`, `BlueprintCallable` (Editor only)

### Behavior
- Loads a JSON file from disk and maps fields onto the provided DataAsset using reflection (`FJsonObjectConverter`).
- Accepts object or array roots:
  - Object root: applied directly to the asset properties.
  - Array root: writes into an array property on the asset.
    - If `ArrayPropertyName` is set, that array is used.
    - If unset, and the asset has an array named `Actions`, that array is used.
    - If unset and the asset has exactly one array property, that array is used.
    - Otherwise the import fails with a log warning.
- Marks the package dirty and calls `PostEditChange` when `bMarkDirty` is true (default).
- Logs warnings for missing asset, bad path, parse failures, or struct mapping failures.

## Built-in Importer Widget
- Tab: **SOTS JSON Importer** (Nomad tab, registered by the plugin)
- Location: Window → Developer Tools → SOTS JSON Importer (searchable via tab search)
- UI fields: DataAsset picker, JSON file path with Browse, optional Array Property Name, and a mark-dirty checkbox.
- Action: `Import JSON` button calls `USOTS_GenericJsonImporter` with the provided inputs.

### Usage (Blueprint)
1) Create/open an Editor Utility Blueprint or any editor context that can call `CallInEditor` functions.
2) Call `ImportJson`:
   - `TargetAsset`: the DataAsset instance to fill.
   - `JsonFilePath`: absolute or relative path to the JSON file on disk.
   - `ArrayPropertyName` (optional): name of the array property if the root JSON is an array.
   - `bMarkDirty` (optional): leave true to mark the asset dirty and trigger `PostEditChange`.
3) Compile and press the Call In Editor button, or call from a scripted editor tool.

### Usage (Built-in Widget)
1) Open the tab: Window → Developer Tools → SOTS JSON Importer (or search for "SOTS JSON Importer" in the tab palette).
2) Pick the target DataAsset in the asset picker.
3) Set the JSON file path (use Browse) and optionally the array property name.
4) Leave "Mark asset dirty" enabled unless you intentionally want a dry run.
5) Click **Import JSON**; progress and issues log to Output Log.

### Notes
- Works only in editor builds (`WITH_EDITOR`).
- No dependencies on Blutility or EditorScriptingUtilities; kept minimal (Core, CoreUObject, Engine, Json, JsonUtilities).
- Keep JSON keys aligned with the DataAsset property names and types to avoid mapping failures.
