# BPGen SPINE_J AutoFix + Conversions + Recipes (2025-12-19 01:35:30)

Summary
- Added apply-time auto-fix flags and `auto_fix_steps` reporting.
- AutoFix covers pin alias matching, swap-direction recovery, and conversion-node insertion.
- Added recipe actions that expand into GraphSpecs (print/branch/set/bind).

AutoFix toggles
- `auto_fix`: enable heuristics.
- `auto_fix_max_steps`: step cap per apply (default 5).
- `auto_fix_insert_conversions`: allow conversion node insertion.

Conversion table v0.1
- Bool <-> Int: `/Script/Engine.KismetMathLibrary:Conv_BoolToInt` / `Conv_IntToBool`
- Int <-> Float: `/Script/Engine.KismetMathLibrary:Conv_IntToFloat` / `Conv_FloatToInt`
- Name <-> String: `/Script/Engine.KismetStringLibrary:Conv_NameToString` / `Conv_StringToName`
- Text <-> String: `/Script/Engine.KismetTextLibrary:Conv_StringToText` / `Conv_TextToString`

Recipes (transparent expansion)
- `recipe_print_string`: PrintString call with entry/result wiring.
- `recipe_branch_on_bool`: Branch node with Condition default value.
- `recipe_set_variable`: Ensure variable, then VariableSet node + exec chain.
- `recipe_widget_bind_text`: Ensure binding, then apply `Conv_StringToText` graph.

Notes
- Pin alias matching mirrors VibeUE `FCommonUtils::SuggestBestPinMatch`.
- Pin type diagnostics mirror VibeUE `FBlueprintReflection::GetPinTypeDescription`.
- Conversion insertion only runs when `auto_fix_insert_conversions=true`.
