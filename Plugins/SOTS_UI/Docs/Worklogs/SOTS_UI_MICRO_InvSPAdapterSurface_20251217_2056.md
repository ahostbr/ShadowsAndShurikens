# Worklog - SOTS_UI MICRO InvSPAdapter Surface (2025-12-17 20:56)

## What changed
- Expanded USOTS_InvSPAdapter C++ surface to match the locked adapter checklist (toggle, shortcut visibility, pickup notifications) while keeping InvSP isolation and existing functions intact.
- Added safe default implementations (no-ops; Toggle calls OpenInventory) to avoid behavior changes until BP adapter maps them.
- Added brief doc comment and GameplayTag include for item-tag-based notifications.

## Files touched
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_InvSPAdapter.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_InvSPAdapter.cpp

## Notes
- No builds run.
- Cleanup: removed Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate (already absent).
