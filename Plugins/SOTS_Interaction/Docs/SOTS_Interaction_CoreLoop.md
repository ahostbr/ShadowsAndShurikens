# SOTS Interaction Core Loop

## Roles
- **Subsystem (USOTS_InteractionSubsystem)**: Owns scans/trace/LOS, scoring, candidate selection, option gating, and emits intents. No UI creation.
- **Driver (USOTS_InteractionDriverComponent)**: Player-facing handle; auto-updates focus, surfaces focus/score/options getters, and forwards TryInteract/ExecuteOption to the subsystem. Emits focus/options/interact delegates only.
- **Data (Hybrid provider)**: Component-first (`USOTS_InteractableComponent.Options` + defaults/meta/tags/LOS/range), interface fallback (`ISOTS_InteractableInterface::GetInteractionOptions`). If both exist, component data wins; interface can fill when component has no options. Interaction data is returned as `FSOTS_InteractionData` (context + options + provider flags + score).

## Contracts
- **Selection**: Subsystem traces (configurable shape/channel/radius/max distance) -> builds context -> gathers options (component > interface) -> gates by TagManager union view -> scores (distance + facing) -> caches/broadcasts candidate changes. Options change without target change still fires candidate change.
- **LOS/Trace**: `FSOTS_InteractionTraceConfig` is the single tuning surface (max distance, channel, shape + radius, optional socket aim). OmniTrace is used when available unless forced legacy. Global LOS toggle respected; per-option/component LOS overrides honored at execute.
- **Tag gating**: Boundary gating uses TagManager union view only. Component required/blocked tags and per-option required/blocked tags for player/target are evaluated via `EvaluateTagGates` (HasAll/HasAny). Fail-open if TagManager unavailable.
- **Execution**: Driver TryInteract picks requested/default option, broadcasts OnInteractRequested, then calls subsystem execute/request. Subsystem rechecks range/LOS (with option overrides) and interface CanInteract before ExecuteInteraction.
- **UI rule**: Plugin never creates widgets or pushes UI; only emits intents/delegates and data (prompt/options/fail payloads, driver delegates).

## Data Types
- `FSOTS_InteractionOption`: Option tag, display text, blocked reason, meta tags, required/blocked player/target tags, LOS override, max distance override, priority.
- `FSOTS_InteractionContext`: Player/target refs, interaction tag, distance, hit result, score.
- `FSOTS_InteractionData`: Context + options + provider flags + score.
- `FSOTS_InteractionTraceConfig`: MaxDistance, bRequireLineOfSight, TraceChannel, TraceShape (Sphere/Line), SphereRadius, TargetSocketName.

## Usage Notes
- Call `Driver_UpdateNow`/`ForceRefreshFocus` to manually refresh; or enable auto-update.
- Consume driver delegates: OnFocusChanged(old,new), OnOptionsChanged(actor, options), OnInteractRequested(actor, verb), OnUIIntentForwarded(payload).
- Use `GetCurrentInteractionData`/`GetFocusedOptions` to drive UI; UI creation lives in SOTS_UI.
- Tag schema lock applies (SAS.* root); add new tags only when referenced and missing.
