# Buddy Worklog — UI Router Interaction Verb Policy (20251221_235900)

Goal
- Gate the deferred/stub logging for interaction verbs so debug playtests stop seeing spam, while keeping the router behavior the same for callers that expect no routing changes.

What changed
- Added a CVar-driven gate (`sots.ui.LogUnhandledInteractionVerbs`) plus one-time-per-verb logging for unhandled `Interaction.Verb.*` values.
- Updated `HandleInteractionActionRequested` to leave non-pickup verbs untouched, matching the existing expectation that `Interaction.Verb.Execute`, `Interaction.Verb.DragStart`, and `Interaction.Verb.DragStop` belong to SOTS_Interaction/SOTS_BodyDrag/SOTS_KillExecutionManager.
- Documented the policy for router verb ownership and debugging guidance in `SOTS_UIRouter_InteractionVerbPolicy.md`.

Files changed
- [Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp](Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp)
- [Plugins/SOTS_UI/Docs/SOTS_UIRouter_InteractionVerbPolicy.md](Plugins/SOTS_UI/Docs/SOTS_UIRouter_InteractionVerbPolicy.md)

Notes + risks/unknowns
- Semantics preserved: the router still treats Execute/Drag verbs as no-ops and does not forward them, so existing callers continue to see the same return flow.

Verification status
- UNVERIFIED (no build/run).

Cleanup
- Plugins/SOTS_UI/Binaries/ deleted.
- Plugins/SOTS_UI/Intermediate/ deleted.

Follow-ups / next steps
- Ryan: verify that enabling `sots.ui.LogUnhandledInteractionVerbs` surfaces exactly one log per verb per session.
- Ryan: ensure the owning gameplay subsystems still handle Execute/Drag without the router forwarding them elsewhere.