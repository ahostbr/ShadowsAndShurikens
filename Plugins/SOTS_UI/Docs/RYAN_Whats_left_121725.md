What’s left (practical “ship it” checks) is mostly wiring + runtime validation, not more architecture:

Confirm the real runtime widget + registry entry exists (not just the seed JSON), and the widget actually binds to the subsystem delegates (progress/finished/cancel).

Create at least one per-level LoadList DA (DA_ShaderWarmup_MapName) and set DefaultShaderWarmupLoadList in UI settings, so you’re not hitting fallback constantly.

Timing sanity test: make sure the screen doesn’t pop right before MissionDirector calls OpenLevel. If you see even a 1-frame flicker, adjust so the warmup screen is held until PreLoadMap starts MoviePlayer (or until PostLoadMapWithWorld completes).

Shipping behavior check: verify the PSO “remaining” path actually behaves on your build (if it’s always 0, warmup still works as “preload/touch,” but it won’t “wait”).

Cancel UX policy: decide if Back/Escape should cancel warmup (and abort travel) or be disabled on that screen, then enforce it in the router policy.