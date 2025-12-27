# Buddy Worklog — 2025-12-26 22:46:00 — ModelAliases

## Goal
Allow SOTS agents runner model configuration to use stable alias constants like `GPT_5_2` and `GPT_5_2_CODEX` instead of raw OpenAI model IDs.

## What changed
- Added `MODEL_ALIASES` + `normalize_model_name()` in `sots_agents_runner.py`.
- Applied normalization for:
  - Env-based defaults: `SOTS_MODEL_TRIAGE`, `SOTS_MODEL_CODE`, `SOTS_MODEL_DEVTOOLS`, `SOTS_MODEL_EDITOR`
  - Per-call override payload field: `model`

## Aliases added
- `GPT_5_2` -> `gpt-5.2`
- `GPT_5_2_MINI` -> `gpt-5.2-mini`
- `GPT_5_2_CODEX` -> `gpt-5.2-codex`

## Files changed
- DevTools/python/sots_agents_runner.py

## Notes / Risks / Unknowns
- Unknown/unsupported aliases are passed through unchanged so the upstream API can reject/accept.
- UNVERIFIED: did not execute an agents call end-to-end (MCP runner currently errors when calling Runner.run_sync under active event loop).

## Next steps (Ryan)
- If you want more aliases (e.g. GPT_4_1, O1, etc.), list them and Buddy can extend `MODEL_ALIASES`.
- If you want to set defaults in VS Code MCP config, add env vars like `SOTS_MODEL_CODE=GPT_5_2_CODEX`.
