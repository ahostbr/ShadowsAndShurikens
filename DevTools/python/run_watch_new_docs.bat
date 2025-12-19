@echo off
REM Runs watch_new_docs in interactive mode (default) using the project's venv.
REM Update the python path if your venv differs.
set PY=E:\SAS\ShadowsAndShurikens\.venv_mcp\Scripts\python.exe
"%PY%" watch_new_docs.py
pause
