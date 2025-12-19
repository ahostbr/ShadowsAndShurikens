# Buddy Worklog — SOTS_FX_Plugin SPINE2 Library Resolution

**Goal**: Make FX cue resolution deterministic (priority → registration order), keep soft-ref discovery, and surface clear diagnostics without touching pooling or adding UI.

**Discovery (BEP exports)**
- `DevTools/python/ad_hoc_regex_search.py --pattern "FXCue|TriggerFX|TriggerAttachedFX|RequestFXCue" --root BEP_EXPORTS/SOTS` → no matches; no Blueprint callers in SOTS exports.

**Current registry shape (pre-change)**
- Libraries registered via `Libraries`, `LibraryRegistrations` (priority), and `SoftLibraryRegistrations`; registry built into `RegisteredLibraryDefinitions` and sorted into `SortedLibraries`.
- Resolution already flows through `TryResolveCue` and policy gating through `EvaluateFXPolicy`.

**Changes**
- Added deterministic library order logging (priority desc, registration order asc) gated by `bDebugLogCueResolution` and emitted after each registry rebuild.
- Debug failure logs now include the resolved library order string when a cue tag cannot be found.

**Files touched**
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp

**Deterministic resolution rule**
- Priority (higher first), then registration order (stable sort). First match wins; duplicates are ignored beyond the earliest registered entry.

**Soft-ref discovery**
- Existing soft-library registration remains (sync load during init via `SoftLibraryRegistrations`). No new discovery added.

**Diagnostics**
- `bDebugLogCueResolution` now logs the library order on rebuild and includes the ordered list when resolution fails.

**Verification**
- Not run (per instructions).

**Cleanup**
- Deleted Plugins/SOTS_FX_Plugin/Binaries and Plugins/SOTS_FX_Plugin/Intermediate after edits.
