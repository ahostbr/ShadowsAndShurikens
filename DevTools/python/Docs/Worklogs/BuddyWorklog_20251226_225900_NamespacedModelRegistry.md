# Buddy Worklog — 2025-12-26 22:59:00 — NamespacedModelRegistry

## Goal
Support model registry env vars that are named after the model alias, under the `SOTS_MODEL_` namespace (e.g. `SOTS_MODEL_GPT_5_2_MINI=gpt-5.2-mini`).

## What changed
- Updated `normalize_model_name()` in `DevTools/python/sots_agents_runner.py` to resolve model aliases via:
  1) built-in alias table
  2) `SOTS_MODEL_<ALIAS>` env var (preferred namespaced registry)
  3) `<ALIAS>` env var (back-compat)

## Example
- Registry:
  - `SOTS_MODEL_GPT_5_2_MINI=gpt-5.2-mini`
  - `SOTS_MODEL_GPT_5_2_CODEX=gpt-5.2-codex`
  - `SOTS_MODEL_GPT_5_2=gpt-5.2`
- Selection:
  - `SOTS_MODEL_TRIAGE=GPT_5_2_MINI`
  - `SOTS_MODEL_CODE=GPT_5_2_CODEX`

## Files changed
- DevTools/python/sots_agents_runner.py

## Verification
- UNVERIFIED: no end-to-end agent run executed.
- Static: no Python diagnostics reported.
