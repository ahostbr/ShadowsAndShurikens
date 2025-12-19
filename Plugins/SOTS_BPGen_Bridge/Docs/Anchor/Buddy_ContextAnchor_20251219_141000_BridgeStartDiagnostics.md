[CONTEXT_ANCHOR]
ID: 20251219_141000 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: Start_Diagnostics | Owner: Buddy
Scope: Improve bridge start feedback and error surfacing in UI.

DONE
- Added LastStartError/LastStartErrorCode population for socket subsystem missing, socket create fail, bind parse fail, and bind/listen failure (with socket error code) in FSOTS_BPGenBridgeServer::Start.
- Control center logs start success/failure instead of always saying "start requested.".
- Deleted Binaries/Intermediate for SOTS_BPGen_Bridge and SOTS_BlueprintGen after edits.

VERIFIED
- None (not built/run).

UNVERIFIED / ASSUMPTIONS
- Start diagnostics surface in UI; no runtime validation yet.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SSOTS_BPGenControlCenter.cpp
- Plugins/SOTS_BPGen_Bridge/Binaries (deleted)
- Plugins/SOTS_BPGen_Bridge/Intermediate (deleted)
- Plugins/SOTS_BlueprintGen/Binaries (deleted)
- Plugins/SOTS_BlueprintGen/Intermediate (deleted)

NEXT (Ryan)
- Build ShadowsAndShurikensEditor and hit Start Bridge in Control Center; check Status box for running=true or last_start_error/error_code if bind fails.

ROLLBACK
- Revert the two source files and regenerate build outputs.
