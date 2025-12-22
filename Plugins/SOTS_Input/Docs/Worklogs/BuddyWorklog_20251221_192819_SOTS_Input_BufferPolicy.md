# Buddy Worklog 20251221_192819 — SOTS Input Buffer Policy

**Goal**
- Capture the locked-input-buffer policy (Execution/Vanish windows, queue behavior, PC router ownership, device-change routing) so downstream UIs/handlers know the source of truth.

**What changed**
- Extended SOTS_Input_BufferWindows.md with router ownership, queue semantics, and device-change responsibilities so the doc points to the PlayerController router/buffer components and SOTS_UI input-mode policy.
- Removed the generated Plugins/SOTS_Input/Binaries and /Intermediate folders per the cleanup requirement.

**Files changed**
- Plugins/SOTS_Input/Docs/SOTS_Input_BufferWindows.md

**Notes / Risks / Unknowns**
- Documentation only; no runtime verification performed. Device-change ownership is inferred from the router delegate/API, but I did not confirm a SOTS_UI binding in shipped assets.

**Verification**
- UNVERIFIED (doc and cleanup only; no build/editor run).

**Follow-ups / Next steps**
1. Confirm whether SOTS_UI actually hooks `OnInputDeviceChanged` so the documented event flow matches runtime.
2. Coordinate with Ryan if any other plugin relies on the buffer windows to ensure the addition won't conflict with future buffer policy updates.
