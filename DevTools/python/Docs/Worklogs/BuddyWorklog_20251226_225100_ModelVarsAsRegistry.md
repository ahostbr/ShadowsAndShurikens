# Buddy Worklog — 2025-12-26 22:51:00 — ModelVarsAsRegistry

## Goal
Support env vars named after models (e.g. `GPT_5_2_CODEX=gpt-5.2-codex`) so configs can reference model names symbolically.

## What changed
- Updated `normalize_model_name()` in `sots_agents_runner.py` to resolve model names in this order:
  1) raw OpenAI ids like `gpt-*` / `o*` (pass-through)
  2) built-in alias table (`GPT_5_2`, `GPT_5_2_MINI`, `GPT_5_2_CODEX`)
  3) environment-defined alias: if an env var named like the key exists, its value is used

## Example config pattern
- Define model vars:
  - `GPT_5_2=gpt-5.2`
  - `GPT_5_2_CODEX=gpt-5.2-codex`
- Then select per lane:
  - `SOTS_MODEL_CODE=GPT_5_2_CODEX`

## Files changed
- DevTools/python/sots_agents_runner.py

## Verification
- UNVERIFIED: no end-to-end agent run executed.
- Static: no Python diagnostics reported.
