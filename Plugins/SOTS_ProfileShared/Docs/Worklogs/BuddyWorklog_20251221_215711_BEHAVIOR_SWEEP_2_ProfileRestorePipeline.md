# Buddy Worklog - ProfileShared Behavior Sweep 2 (Restore Pipeline)

Goal
- Finish snapshot apply/restore sequencing, provider arbitration logging, and restore completion eventing without ownership violations.

What changed
- Added `OnProfileRestored` delegate to fire after provider apply completes in `ApplySnapshotToWorld`.
- Added provider arbitration cache/logging for highest-priority provider when multiple providers are registered.
- Centralized ProfileShared logging under a plugin log category and added log-once behavior for missing providers.
- Updated snapshot policy doc to reflect restore sequencing and TotalPlaySeconds query behavior.

Files changed
- Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSubsystem.h
- Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSubsystem.cpp
- Plugins/SOTS_ProfileShared/Docs/SOTS_ProfileShared_SnapshotPolicy.md

Notes / decisions
- Provider arbitration logging is verbose and only emits when the winning provider changes.
- Apply order remains: transform -> stats -> provider apply -> OnProfileRestored.

Verification
- Not run (no build/UE run per constraints).

Cleanup
- Deleted Plugins/SOTS_ProfileShared/Binaries and Plugins/SOTS_ProfileShared/Intermediate.
