# BPGen Data Asset Creation Flow

This document describes how `create_data_asset` works from the BPGen bridge down into the engine so you can explain the flow to another LLM or use it as a reference blueprint when automating updates.

## 1. Trigger point
1. A BPGen client (e.g., `sots_bpgen_bridge_client.py`) calls the bridge on `SOTS_BPGen_Bridge` with `action = "create_data_asset"`.
2. Parameters include:
   - `asset_path` (e.g., `/Game/SOTS/Data/TestDA.uasset`).
   - `asset_class_path` (The UE class, such as `/Script/SOTS_FX_Plugin.SOTS_FXCueDefinition`).
   - `create_if_missing` / `update_if_exists` (booleans). 
   - Optional `properties` array of `{ name, value }` overrides that target UPROPERTY names.

## 2. Bridge handling
1. `SOTS_BPGenBridgeServer.cpp` (see `Action = create_data_asset`) deserializes the JSON payload into `FSOTS_BPGenDataAssetDef` and loops the property array.
2. The bridge enforces the requirements, handles dry-run gating, then forwards the struct to `USOTS_BPGenBuilder::CreateDataAssetFromDef` and serializes the resulting `FSOTS_BPGenAssetResult` back to the client.
3. Audits, errors, and warnings appear in `Plugins/SOTS_BPGen_Bridge/Saved/BPGenAudit` for traceability.

## 3. Builder flow
1. `USOTS_BPGenBuilder::CreateDataAssetFromDef` normalizes the package name, loads/creates the package, and instantiates or updates the data asset class from `AssetClassPath`.
2. Before saving, it iterates `AssetDef.Properties` calling `TrySetDataAssetProperty` for each override (string-valued map) to mutate the underlying UObject.
3. After mutations, the builder saves the package with `SavePackage`, logs the result, and returns success/failure plus any errors.

## 4. Property override logic
`TrySetDataAssetProperty` uses reflection to detect property types:
- Primitive types (`float`, `int`, `bool`, `enum`, `name`, `string`, `text`) parse the incoming string value.
- `FObjectPropertyBase` loads referenced assets via `StaticLoadObject`.
- `FSoftObjectProperty` / `FSoftClassProperty` store an `FSoftObjectPtr` instead of instantiating synchronously.
- `FStructProperty` checks for `FGameplayTag`: it resolves `FGameplayTag::RequestGameplayTag(Name)` and copies the resulting struct, allowing strings like `Test.Test` to set `CueTag` or other tag fields.

> Because the bridge now interprets gameplay-tag strings (with redirects defined in `Config/DefaultGameplayTags.ini`), callers can alias `Test.Test` to `SAS.Test.Test` and the builder will store the resolved tag.

## 5. Best practices for automation
- Confirm the editor (or bridge binary) is running when issuing commands; otherwise the socket will time out.
- Rebuild UE (if you changed Bridge/Builder code) so the editor loads the new runtime behavior before calling `create_data_asset`.
- For data assets that already exist, use `update_if_exists: true` to edit fields without losing other serialized data.
- Verify results by opening the target asset in the editor; the builder writes the asset at `AssetPath` and logs success in `FSOTS_BPGenAssetResult`.

## 6. Troubleshooting
- Missing tags: ensure your gameplay tag config includes the requested name (or redirect) so `FGameplayTag::RequestGameplayTag` returns valid data.
- Soft references: provide valid `/Game/...` paths that match the asset registry so soft-object/soft-class values can be cached.
- Errors surface in the bridge response and `Saved/Logs`; copy them into your automation logs for easier triage.
- If a property fails, `TrySetDataAssetProperty` adds a descriptive error and the bridge bundles it into the JSON response along with `bSuccess = false`.

Use this doc to teach other agents how BPGen modifies data assets, or to craft new automation flows. Ping the bridge with a JSON payload like the one above, and it will call the builder -> reflection helper -> save path exactly as described.