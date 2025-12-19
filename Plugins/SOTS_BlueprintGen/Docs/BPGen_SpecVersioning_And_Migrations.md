# BPGen Spec Versioning and Migrations

This document defines how BPGen GraphSpec versioning, canonicalization, migrations, and repair modes work (SPINE_M).

## Spec versioning

GraphSpec roots now include explicit schema metadata:

- `spec_schema`: `"SOTS_BPGen_GraphSpec"`
- `spec_version`: `1`

Use the bridge action `get_spec_schema` to query the current version and supported versions.

## Canonicalization

Canonicalization normalizes a GraphSpec before apply or tooling:

- Normalizes `target.target_type` to the canonical uppercase form.
- Ensures stable `node_id` values when missing (generated from spawner_key + index).
- Deterministically sorts nodes and links (same ordering as SPINE_I inspector output).
- Applies migration maps when enabled.

Bridge action: `canonicalize_spec` returns:

- `canonical_spec`
- `diff_notes`
- `spec_migrated`
- `migration_notes`

## Migration maps

BPGen uses data-driven migration maps stored under:

- `Plugins/SOTS_BlueprintGen/Content/BPGenMigrations/pin_aliases.json`
- `Plugins/SOTS_BlueprintGen/Content/BPGenMigrations/node_class_aliases.json`
- `Plugins/SOTS_BlueprintGen/Content/BPGenMigrations/function_path_aliases.json`

### Pin aliases

```json
[
  {
    "node_class": "K2Node_CallFunction",
    "function_path": "/Script/Engine.KismetSystemLibrary:PrintString",
    "pin_aliases": {
      "InString": ["String", "Text", "InText"],
      "Then": ["then", "Out"]
    }
  }
]
```

### Node class aliases

```json
[
  { "old": "K2Node_SomethingOld", "new": "K2Node_SomethingNew" }
]
```

### Function path aliases

```json
[
  { "old": "/Script/OldModule.OldLib:Conv_Foo", "new": "/Script/NewModule.NewLib:Conv_Foo" }
]
```

## Repair modes

`repair_mode` is optional on the GraphSpec root and affects apply behavior when node ids drift:

- `none`: No repair attempts.
- `soft`: If a node_id is missing, try to match by spawner_key + node_class + approximate position.
- `aggressive`: If spawner_key is missing, attempt to recover via function_path/node_class and position (using migrations).

When a repair is applied, the apply result includes `repair_steps`.

## Tooling

DevTools helpers:

- `DevTools/python/bpgen_spec_upgrade.py` uses `canonicalize_spec` and writes an upgraded spec + report.
- `DevTools/python/bpgen_spec_lint.py` validates required fields and flags risky patterns.
