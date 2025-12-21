# Buddy Worklog — BlueprintGen decouple from bridge

## goal
Remove BlueprintGen’s direct dependency on SOTS_BPGen_Bridge to break the plugin/module circular reference.

## what changed
- Removed Control Center tab/menu from BlueprintGen module; deleted the Control Center widget files from BlueprintGen.
- Dropped SOTS_BPGen_Bridge from BlueprintGen Build.cs and from the uplugin plugin dependencies.
- Cleaned BlueprintGen Binaries and Intermediate directories after edits.

## files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BlueprintGen.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BlueprintGen.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/SOTS_BlueprintGen.Build.cs
- Plugins/SOTS_BlueprintGen/SOTS_BlueprintGen.uplugin
- (Deleted) Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SSOTS_BPGenControlCenter.cpp
- (Deleted) Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SSOTS_BPGenControlCenter.h
- (Deleted) Plugins/SOTS_BlueprintGen/Binaries
- (Deleted) Plugins/SOTS_BlueprintGen/Intermediate

## notes + risks/unknowns
- Control Center UI now lives in SOTS_BPGen_Bridge; BlueprintGen no longer exposes that tab.
- Build not run; dependency break is UNVERIFIED until UBT completes.

## verification status
- Not built or run.

## follow-ups / next steps
- Rebuild; ensure circular dependency warning disappears and BlueprintGen compiles.
- Open Control Center from the bridge plugin to verify UI parity.
