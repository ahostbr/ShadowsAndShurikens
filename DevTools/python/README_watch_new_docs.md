# SOTS DevTool — watch_new_docs.py (interactive)

This tool watches for new/updated documentation logs under:
- `Docs/**`
- `Plugins/*/Docs/**`

It tracks an ACK baseline in:
- `DevTools/python/_state/watch_new_docs_state.json`

## Install
Extract the zip into your project root so the script lands at:
- `DevTools/python/watch_new_docs.py`

## Run (DEFAULT = interactive watch)
```bat
E:\SAS\ShadowsAndShurikens\.venv_mcp\Scripts\python.exe DevTools\python\watch_new_docs.py
```

### Behavior (interactive)
- Polls every **10 seconds**
- **Clears** the console each poll
- Lists **all NEW** and **all MODIFIED** docs since last baseline
- Controls:
  - **Z** = zip new+modified docs (asks you to type `YES` to confirm)
  - **A** = ACK baseline (clears lists)
  - **Q** = quit
- After zipping:
  - It **auto-ACKs** the baseline
  - Prompts: watch again or exit

## Outputs
- Reports + lists + zips:
  - `DevTools/python/_reports/new_docs_watch/`
- Zips are created as:
  - `DevTools/python/_reports/new_docs_watch/SOTS_new_docs_<timestamp>.zip`

## Optional: scan once
```bat
E:\SAS\ShadowsAndShurikens\.venv_mcp\Scripts\python.exe DevTools\python\watch_new_docs.py scan
```
