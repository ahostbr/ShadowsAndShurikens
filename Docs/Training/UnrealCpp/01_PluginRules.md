# SOTS Unreal Plugin C++ Rules (MANDATORY)

## Architecture
- SOTS project has no /Source game module. Plugins must not assume project-level classes.
- Prefer Subsystems/Components/Interfaces over hard class dependencies.
- No Tick by default. Event-driven, timers only when necessary.
- Cross-plugin communication via explicit seams/interfaces and TagManager boundary rules.

## Code hygiene
- Minimize includes; prefer forward decls.
- Guard debug/editor-only code (compile guards + cvars).
- EndPlay cleanup: unregister delegates, clear timers, release scoped tags/handles.

## Logging
- Use plugin-specific log category (LogSOTS_<Plugin>).
- Warnings for misconfig, Errors for “cannot proceed”.

## Config/Settings
- Use UDeveloperSettings for project settings surfaces.
- Never silently fall back; log missing settings.

## Deliverables
- Minimal diffs. No refactors unless required.
- Provide exact file paths and what changed.
