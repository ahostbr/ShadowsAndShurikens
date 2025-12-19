# Buddy Worklog — Plugin Worklog Rehoming (2025-12-18 23:50:00)

1) Goal
- Keep plugin-specific worklog documents under their owning plugin folders so future readers can find context without walking the shared `Plugins/Docs` folder.

2) What Changed
- Created missing `Docs/Worklogs` directories for LightProbePlugin, SOTS_GlobalStealthManager, SOTS_SkillTree, SOTS_UDSBridge, SOTS_Debug, and SOTS_AIPerception.
- Moved the matching `BuddyWorklog_20251213_*` and `BuddyWorklog_20251214_*` files from `Plugins/Docs` into the respective plugin `Docs/Worklogs` folders; the shared CharacterBase and Suite shipping audit logs remain in the shared directory because they span multiple modules.

3) Files Changed
- Plugins/Docs/BuddyWorklog_20251213_141230_LightProbePlugin_DebugWidget_ShippingGate.md (moved)
- Plugins/LightProbePlugin/Docs/Worklogs/BuddyWorklog_20251213_141230_LightProbePlugin_DebugWidget_ShippingGate.md (added)
- Plugins/Docs/BuddyWorklog_20251213_141800_SOTS_GlobalStealthManager_ProfileSlice_Wiring.md (moved)
- Plugins/SOTS_GlobalStealthManager/Docs/Worklogs/BuddyWorklog_20251213_141800_SOTS_GlobalStealthManager_ProfileSlice_Wiring.md (added)
- Plugins/Docs/BuddyWorklog_20251213_142500_SOTS_INV_FacadeSurface.md (moved)
- Plugins/SOTS_INV/Docs/Worklogs/BuddyWorklog_20251213_142500_SOTS_INV_FacadeSurface.md (added)
- Plugins/Docs/BuddyWorklog_20251213_152800_SOTS_SkillTree_GatingHelpers.md (moved)
- Plugins/SOTS_SkillTree/Docs/Worklogs/BuddyWorklog_20251213_152800_SOTS_SkillTree_GatingHelpers.md (added)
- Plugins/Docs/BuddyWorklog_20251213_160500_SOTS_UDSBridge_ReflectionBindings.md (moved)
- Plugins/SOTS_UDSBridge/Docs/Worklogs/BuddyWorklog_20251213_160500_SOTS_UDSBridge_ReflectionBindings.md (added)
- Plugins/Docs/BuddyWorklog_20251213_140400_SOTS_Debug_ShippingGate_DebugWidgets.md (moved)
- Plugins/SOTS_Debug/Docs/Worklogs/BuddyWorklog_20251213_140400_SOTS_Debug_ShippingGate_DebugWidgets.md (added)
- Plugins/Docs/BuddyWorklog_20251214_205520_AIPercSuspicion.md (moved)
- Plugins/SOTS_AIPerception/Docs/Worklogs/BuddyWorklog_20251214_205520_AIPercSuspicion.md (added)
- Plugins/Docs/BuddyWorklog_20251214_205118_AIPercAPI.md (moved)
- Plugins/SOTS_AIPerception/Docs/Worklogs/BuddyWorklog_20251214_205118_AIPercAPI.md (added)

4) Notes / Risks / Unknowns
- No behavior code was modified; just document relocation. Risks are limited to potential references to the old paths; verify any scripts referencing `Plugins/Docs` for these filenames.

5) Verification Status
- Not run (file reorganization only; no builds/tests triggered).

6) Cleanup Confirmation
- No binaries/intermediate touched beyond what previous logs mention; no additional cleanup required.

7) Follow-ups / Next Steps
- Update any doc generation scripts or references that hard-code `Plugins/Docs/BuddyWorklog_*` if they expect the files in that location.
