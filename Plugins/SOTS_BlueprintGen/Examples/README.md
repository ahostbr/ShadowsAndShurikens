# BPGen Examples (SPINE_P)

This folder contains redistributable JSON samples (no uassets) mirroring the VibeUE test prompt style but renamed for BPGen.

- **BPGEN_Example_HelloWorld_*.json**: Minimal function with PrintString.
- **BPGEN_AdvancedDoor_*.json**: More complex actor flows (editor-only reference).
- **BPGEN_BeginPlayNotification_*.json**: Demonstrates the new `create_blueprint_asset` action plus a BeginPlay graph that waits three seconds before calling `USOTS_UIRouterSubsystem::ShowNotification`, which surfaces the same text through both SOTS UI and the ProHUD v2 adapter.

For turnkey starts, prefer the templates in `Templates/` and copy/update paths for your assets. After apply, compile/save the Blueprint in the editor.
