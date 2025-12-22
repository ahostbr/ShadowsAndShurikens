# Worklog - SOTS_UI Ownership & Adapter Sweep

- Verified the router is the exclusive owner of widget pushes/pops, input mode, and UI navigation layers before documenting the ownership rules in `SOTS_UI_OwnershipAndAdapters.md`.
- Captured how Back/Escape (CloseTopModal) and the ReturnToMainMenu confirm flow are handled inside `USOTS_UIRouterSubsystem`, then described the InvSP/ProHUD adapter surfaces that must be used instead of direct widget calls.
- Flagged the remaining dev-only `AddToViewport` callers in `Plugins/SOTS_Debug/...` and `Plugins/LightProbePlugin/...` with TODOs so future sweeps can move them through the router.
