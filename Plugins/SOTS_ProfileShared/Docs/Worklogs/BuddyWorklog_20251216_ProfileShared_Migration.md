Worklog (2025-12-16)
- Goal: move profile persistence files that were mistakenly added under DevTools/python/sots_mcp_server into the live SOTS_ProfileShared plugin.

What changed
- Added profile persistence surface to SOTS_ProfileShared: Public `SOTS_ProfileSnapshotProvider.h`, `SOTS_ProfileSaveGame.h`, `SOTS_ProfileSubsystem.h` with Private implementations under the same module.
- The subsystem handles slot naming (`SOTS_Profile_<SlotIndex>_<ProfileName>`), save/load via `UGameplayStatics`, pawn transform capture/restore, optional stats reflection via a `SOTS_StatsComponent`, and provider build/apply iteration with stable priority ordering.
- Verified SOTS_INV `SOTS_InventoryProvider` and `UInvSPInventoryProviderComponent` already matched the dev copy; no changes needed there.
- Cleared `Plugins/SOTS_ProfileShared/Binaries` and `Plugins/SOTS_ProfileShared/Intermediate` after adding the new files.

Verification
- No build or runtime verification performed yet (UE not launched). Pending compile once integrated with provider registrations in owning systems.

Follow-ups
- Wire existing slice owners (GSM, MissionDirector, MMSS, FX, Ability, SkillTree, Inventory) into `USOTS_ProfileSubsystem` using `ISOTS_ProfileSnapshotProvider` in their subsystems and re-clean their plugin artifacts after changes.
