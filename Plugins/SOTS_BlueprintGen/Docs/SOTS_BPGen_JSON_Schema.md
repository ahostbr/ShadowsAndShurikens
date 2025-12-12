# SOTS BlueprintGen JSON Schema (BRIDGE 1)

Reference for authoring BPGen JSON consumed by `SOTS_BPGenBuildCommandlet` and `USOTS_BPGenBuilder`. Field names match the `FSOTS_BPGen*` structs exactly (PascalCase) and are parsed via `FJsonObjectConverter`.

## Job JSON overview
- Root object keys:
  - `Function` (required): `FSOTS_BPGenFunctionDef` describing which Blueprint/function to create/update.
  - `StructsToCreate` (optional array): `FSOTS_BPGenStructDef` entries to create/update user-defined structs.
  - `EnumsToCreate` (optional array): `FSOTS_BPGenEnumDef` entries to create/update user-defined enums.
  - `GraphSpec` (optional): inline `FSOTS_BPGenGraphSpec` describing the function body.
- External GraphSpec: when `-GraphSpecFile=<path>` is passed to the commandlet, that JSON is loaded as `FSOTS_BPGenGraphSpec` and **takes precedence** over any inline `GraphSpec`.
- Processing order: structs → enums → function skeleton → graph spec (if provided). Errors in any step stop the run.

## Struct shape details

### FSOTS_BPGenFunctionDef (required in Job root)
- `TargetBlueprintPath` (string, required): Full asset object path (e.g. `"/Game/Dev/BP_Sample.BP_Sample"`).
- `FunctionName` (string, required): Function to create/update on the Blueprint.
- `Inputs` (array<`FSOTS_BPGenPin`>, optional): Input parameters.
- `Outputs` (array<`FSOTS_BPGenPin`>, optional): Output parameters/return values.

### FSOTS_BPGenStructDef
- `AssetPath` (string, required): Long package path for the struct asset.
- `StructName` (string, optional): Struct name; defaults to object name derived from `AssetPath` if omitted/empty.
- `Members` (array<`FSOTS_BPGenPin`>, required): Struct fields in order.

### FSOTS_BPGenEnumDef
- `AssetPath` (string, required): Long package path for the enum asset.
- `EnumName` (string, optional): Enum name; defaults to object name derived from `AssetPath` if omitted/empty.
- `Values` (array<string>, required): Ordered enum entries.

### FSOTS_BPGenPin
- `Name` (string, required for struct members/variables; optional for some graph uses): Logical pin/field name.
- `Category` (string, required): Pin category, e.g. `bool`, `int`, `float`, `string`, `struct`, `object`.
- `SubCategory` (string, optional): Further refinement, e.g. `byte`, `int64`, `SoftObject`.
- `SubObjectPath` (string, optional): Asset/class path when the category/subcategory needs a type (struct/object).
- `ContainerType` (string, optional, default `"None"`): One of `None`, `Array`, `Set`, `Map`.
- `Direction` (string, optional, default `"Input"`): One of `Input`, `Output`. Relevant when a pin is directional.
- `DefaultValue` (string, optional): Literal default value where supported.

### FSOTS_BPGenGraphNode
- `Id` (string, required): Unique per-graph identifier used by links.
- `NodeType` (string, required): K2 node class name (e.g. `K2Node_FunctionEntry`, `K2Node_CallFunction`, `K2Node_VariableGet`).
- `FunctionPath` (string, optional, required for call-function/array-function nodes): Full function path (e.g. `/Script/Engine.KismetSystemLibrary:PrintString`).
- `StructPath` (string, optional, required for make/break struct nodes): Full struct path (e.g. `/Script/Engine.Vector`).
- `VariableName` (string, optional, required for variable get/set, some events, select options): Variable or event name.
- `NodePosition` (vector2, optional, default `0,0`): Graph editor position `{ "X": 0, "Y": 0 }`.
- `ExtraData` (object<string,string>, optional): Free-form key/values for node-specific defaults (pin literal values, select options, container hints, etc.).

### FSOTS_BPGenGraphLink
- `FromNodeId` (string, required): Source node id.
- `FromPinName` (string, required): Source pin name.
- `ToNodeId` (string, required): Target node id.
- `ToPinName` (string, required): Target pin name.

### FSOTS_BPGenGraphSpec
- `Nodes` (array<`FSOTS_BPGenGraphNode`>, optional): Nodes to spawn; empty allowed.
- `Links` (array<`FSOTS_BPGenGraphLink`>, optional): Directed pin connections; empty allowed.

### Result types (builder outputs)
- `FSOTS_BPGenAssetResult`: `bSuccess` (bool), `AssetPath` (string), `Message` (string).
- `FSOTS_BPGenApplyResult`: `bSuccess` (bool), `TargetBlueprintPath` (string), `FunctionName` (string), `ErrorMessage` (string), `Warnings` (array<string>).

## Examples (ready-to-run JSON)
- **BPGEN_Example_HelloWorld**  
  - Job: `Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_HelloWorld_Job.json`  
  - GraphSpec: `Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_HelloWorld_GraphSpec.json`  
  - Demonstrates: simple `PrintString` flow wired from entry to result.  
  - Run via commandlet:  
    ```
    UEEditor-Cmd.exe "ShadowsAndShurikens.uproject" -run=SOTS_BPGenBuildCommandlet ^
      -JobFile="Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_HelloWorld_Job.json" ^
      -GraphSpecFile="Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_HelloWorld_GraphSpec.json"
    ```

- **BPGEN_Example_TraceUtility**  
  - Job: `Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_TraceUtility_Job.json`  
  - GraphSpec: `Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_TraceUtility_GraphSpec.json`  
  - Demonstrates: line trace, branch on hit result, and hit/miss logging.  
  - Run via commandlet:  
    ```
    UEEditor-Cmd.exe "ShadowsAndShurikens.uproject" -run=SOTS_BPGenBuildCommandlet ^
      -JobFile="Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_TraceUtility_Job.json" ^
      -GraphSpecFile="Plugins/SOTS_BlueprintGen/Examples/BPGEN_Example_TraceUtility_GraphSpec.json"
    ```

These examples can also be fed to a future in-editor BPGen Runner using the same Job/GraphSpec files.

## Minimal end-to-end examples

### Inline GraphSpec Job JSON
```json
{
  "Function": {
    "TargetBlueprintPath": "/Game/BP_Test_BPGen.BP_Test_BPGen",
    "FunctionName": "HelloFromLLM",
    "Inputs": [
      { "Name": "bPrintToScreen", "Category": "bool", "DefaultValue": "true" }
    ],
    "Outputs": []
  },
  "StructsToCreate": [],
  "EnumsToCreate": [],
  "GraphSpec": {
    "Nodes": [
      { "Id": "Entry", "NodeType": "K2Node_FunctionEntry", "NodePosition": { "X": -400, "Y": 0 } },
      {
        "Id": "PrintString",
        "NodeType": "K2Node_CallFunction",
        "FunctionPath": "/Script/Engine.KismetSystemLibrary:PrintString",
        "NodePosition": { "X": 0, "Y": 0 },
        "ExtraData": { "InString": "Hello from BPGen", "bPrintToScreen": "true" }
      },
      { "Id": "Result", "NodeType": "K2Node_FunctionResult", "NodePosition": { "X": 400, "Y": 0 } }
    ],
    "Links": [
      { "FromNodeId": "Entry", "FromPinName": "then", "ToNodeId": "PrintString", "ToPinName": "execute" },
      { "FromNodeId": "PrintString", "FromPinName": "then", "ToNodeId": "Result", "ToPinName": "execute" }
    ]
  }
}
```

### External GraphSpec example (separate file)
Job JSON (no inline GraphSpec, assumes `-GraphSpecFile=<path>`):
```json
{
  "Function": {
    "TargetBlueprintPath": "/Game/BP_Test_BPGen.BP_Test_BPGen",
    "FunctionName": "HelloFromLLM",
    "Inputs": [],
    "Outputs": []
  },
  "StructsToCreate": [],
  "EnumsToCreate": []
}
```

External GraphSpec JSON:
```json
{
  "Nodes": [
    { "Id": "Entry", "NodeType": "K2Node_FunctionEntry", "NodePosition": { "X": -300, "Y": 0 } },
    {
      "Id": "PrintString",
      "NodeType": "K2Node_CallFunction",
      "FunctionPath": "/Script/Engine.KismetSystemLibrary:PrintString",
      "NodePosition": { "X": 50, "Y": 0 },
      "ExtraData": { "InString": "External spec hello" }
    },
    { "Id": "Result", "NodeType": "K2Node_FunctionResult", "NodePosition": { "X": 400, "Y": 0 } }
  ],
  "Links": [
    { "FromNodeId": "Entry", "FromPinName": "then", "ToNodeId": "PrintString", "ToPinName": "execute" },
    { "FromNodeId": "PrintString", "FromPinName": "then", "ToNodeId": "Result", "ToPinName": "execute" }
  ]
}
```
