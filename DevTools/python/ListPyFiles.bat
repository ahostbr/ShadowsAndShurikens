@echo off
setlocal EnableDelayedExpansion

rem Ensure we work in the folder where the .bat is located
cd /d "%~dp0"

rem Clear output file and write full paths of .py files (current folder only)
> output.txt (
    for %%F in (*.py) do (
        echo %cd%\%%F
    )
)

echo Done! Listed all .py files in this folder only.
echo Paths saved to output.txt
echo.
echo Found files:
type output.txt
echo.
pause