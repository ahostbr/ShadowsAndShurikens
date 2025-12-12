# SOTS_EdUtil

Editor-only plugin providing lightweight utilities. Current feature set is a generic JSON-to-DataAsset importer exposed to Blueprints.

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

### Usage (Blueprint)
1) Create/open an Editor Utility Blueprint or any editor context that can call `CallInEditor` functions.
2) Call `ImportJson`:
   - `TargetAsset`: the DataAsset instance to fill.
   - `JsonFilePath`: absolute or relative path to the JSON file on disk.
   - `ArrayPropertyName` (optional): name of the array property if the root JSON is an array.
   - `bMarkDirty` (optional): leave true to mark the asset dirty and trigger `PostEditChange`.
3) Compile and press the Call In Editor button, or call from a scripted editor tool.

### Notes
- Works only in editor builds (`WITH_EDITOR`).
- No dependencies on Blutility or EditorScriptingUtilities; kept minimal (Core, CoreUObject, Engine, Json, JsonUtilities).
- Keep JSON keys aligned with the DataAsset property names and types to avoid mapping failures.
