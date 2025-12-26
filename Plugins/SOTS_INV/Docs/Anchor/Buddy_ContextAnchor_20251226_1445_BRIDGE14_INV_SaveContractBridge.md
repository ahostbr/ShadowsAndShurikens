Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1445 | Plugin: SOTS_INV | Pass/Topic: BRIDGE14_SaveContractParticipant | Owner: Buddy
Scope: Opt-in bridge registering SOTS_INV as a SOTS.CoreSaveParticipant emitting/applying opaque inventory snapshot bytes

DONE
- Added `USOTS_INVCoreBridgeSettings` with `bEnableSOTSCoreSaveParticipantBridge` and `bEnableSOTSCoreBridgeVerboseLogs` (defaults OFF)
- Added `FINV_SaveParticipant : ISOTS_CoreSaveParticipant` (id=INV) emitting/applying fragment `INV.Inventory` by serializing `FSOTS_InventoryProfileData`
- Module wiring: Startup registers / Shutdown unregisters via `FSOTS_CoreSaveParticipantRegistry` when enabled
- Dependency wiring: SOTS_INV now depends on SOTS_Core

VERIFIED
- (none) No build/editor run

UNVERIFIED / ASSUMPTIONS
- Assumes `FSOTS_InventoryProfileData` UStruct serialization is compatible with SaveContract payload round-trip

FILES TOUCHED
- Plugins/SOTS_INV/Source/SOTS_INV/SOTS_INV.Build.cs
- Plugins/SOTS_INV/SOTS_INV.uplugin
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_INVModule.cpp
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_INVCoreBridgeSettings.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_INVCoreBridgeSettings.cpp
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_INVCoreSaveParticipant.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_INVCoreSaveParticipant.cpp
- Plugins/SOTS_INV/Docs/SOTS_INV_SOTSCoreSaveContractBridge.md

NEXT (Ryan)
- Enable Core save participant queries and INV bridge setting; run `SOTS.Core.BridgeHealth` and confirm participant id `INV` is present
- Trigger save that requests fragments and confirm `INV.Inventory` fragment is produced and re-applied without errors

ROLLBACK
- Revert the files listed above; plugin behavior is default-OFF so toggles can also be disabled in config
