# Buddy Worklog - UI Router External Menus (20251222_010157)

Goal
- Ensure the UI router honors sealed external menus (InvSP) with explicit lifecycle events and no Back/Escape close interference.

Changes
- Added external menu tracking/delegates in the router and wired inventory/container open/close to emit ExternalMenuOpened/Closed.
- Blocked generic PopWidget from closing external menus; closures now route through explicit lifecycle notifications.
- Added InvSP adapter callable helpers to forward external menu open/close notifications back to the router.
- External menu tracking now clears on widget removal and subsystem deinit.

Notes
- Lawfile sections read (UI router ownership, InvSP access rules, KEM contract); no new policy introduced.
- No build/run executed.

Files touched
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_InvSPAdapter.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_InvSPAdapter.cpp

Cleanup
- Deleted Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate.
