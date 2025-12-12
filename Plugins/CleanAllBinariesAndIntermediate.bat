@echo off
setlocal ENABLEDELAYEDEXPANSION

REM Use the folder where this .bat file lives as the root
set "ROOT=%~dp0"
REM Remove trailing backslash if present
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

echo ==========================================
echo   Delete all Binaries and Intermediate
echo   Starting in: %ROOT%
echo ==========================================
echo.

choice /M "Are you sure you want to delete ALL 'Binaries' and 'Intermediate' folders under this directory?"
if errorlevel 2 (
    echo.
    echo Aborted.
    goto :EOF
)

echo.
echo Searching for 'Binaries' and 'Intermediate' folders...
echo.

REM Delete all "Binaries" folders (the folder itself + contents)
for /D /R "%ROOT%" %%G in (Binaries) do (
    if /I "%%~nxG"=="Binaries" (
        echo Deleting folder: "%%G"
        rmdir /S /Q "%%G"
    )
)

REM Delete all "Intermediate" folders (the folder itself + contents)
for /D /R "%ROOT%" %%G in (Intermediate) do (
    if /I "%%~nxG"=="Intermediate" (
        echo Deleting folder: "%%G"
        rmdir /S /Q "%%G"
    )
)

echo.
echo Done.
echo.

endlocal
