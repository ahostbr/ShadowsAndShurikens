# Buddy Worklog — SOTS_FX_Plugin SPINE2 Library Resolution

**Goal**
- Make FX cue resolution deterministic with priority + registration order, add safe registration APIs, optional soft-ref loading, and better reporting.

**What changed**
- Added per-library priority metadata and explicit registration lists (hard + soft) with stable ordering by priority desc then registration order.
- Implemented RegisterLibrary / RegisterLibraries / RegisterLibrarySoft / UnregisterLibrary APIs with duplicate handling and optional debug logging.
- Rebuilt registry construction to cache source library metadata for each tag and surface failure reasons including searched library count.
- Added dev-only cue resolution debug logging flag.

**Files touched**
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXDefinitionLibrary.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp

**Notes / verification**
- BEP export search for "FXCue/TriggerFX/RequestFX" found only CGF references; no SOTS-specific assumptions observed.
- No builds/tests run (per instructions).

**Cleanup**
- Requested deletion of Plugins/SOTS_FX_Plugin/Binaries and Intermediate (Remove-Item, code 1 but no errors emitted).

**Follow-ups**
- If any libraries should be registered via soft references, populate SoftLibraryRegistrations with priorities.
- Set bDebugLogCueResolution/bLogLibraryRegistration as needed for troubleshooting during development builds.
