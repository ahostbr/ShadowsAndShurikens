# Prompt ID
- Not provided (Return to Main Menu flow request).

# Goal
- Add a router-driven “Return to Main Menu” flow with confirmation and handoff to a non-UI game flow owner.

# What changed
- Added a `RequestReturnToMainMenu` helper in the UI router that builds the standard confirm dialog payload using registry-driven widget ids.
- Introduced a broadcast (`OnReturnToMainMenuRequested`) so the router only signals intent and stays decoupled from gameplay travel.
- Added a `GameFlow` game instance subsystem to listen for the router signal and perform the map travel to the configured (or default) main menu map.

# Files changed
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
- Plugins/SOTS_UI/Docs/SOTS_UI_Prompt4_UIContracts.md
- Source/ShadowsAndShurikens/ShadowsAndShurikens.Build.cs
- Source/ShadowsAndShurikens/Public/SOTS_GameFlowSubsystem.h
- Source/ShadowsAndShurikens/Private/SOTS_GameFlowSubsystem.cpp

# Notes
- Confirm dialog uses `GetDefaultConfirmDialogTag`/registry resolution; callers can optionally override the body text.
- Game flow subsystem falls back to `GameMapsSettings::GetGameDefaultMap` when `MainMenuMap` is unset; configure the actual main menu map for shipping.

# Verification
- Not run (no builds or automated tests executed).

# Cleanup confirmation
- Deleted Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate; no build triggered.

# Follow-ups
- Point the pause/options UI button to call `USOTS_UIRouterSubsystem::RequestReturnToMainMenu` so the new router path is used end-to-end.
- Set `MainMenuMap` in config to the authoritative main menu level once finalized.
