# Buddy Worklog - SOTS_UI ShaderWarmup Level-Scoped Filters (2025-12-17 11:32:57)

**Goal**: Enforce level-scoped warmup queues (no global enumeration) and add dev-only scoping logs.

**Changes**
- Hardened fallback dependency scan to accept only `/Game/` (plus explicit whitelist placeholder) and reject `/Engine/`, `/Script/`, `/Niagara/`, `/Plugins/`.
- Skipped redirectors and editor-only assets; non-material/Niagara assets are filtered out.
- Added dev-only logging summarizing level, deps visited, assets found, accepted, and rejected buckets; warns if zero accepted.

**Files touched**
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp

**Verification**
- Not run (code-only change; no build executed per SOTS law).

**Cleanup**
- Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate not present to delete.
