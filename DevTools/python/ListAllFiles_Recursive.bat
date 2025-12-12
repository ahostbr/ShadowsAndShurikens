@echo off
chcp 65001 >nul
echo Scanning for .py files – completely ignoring any folder that contains "conda" in its name...
echo.

rem Change to the folder where the .bat is located
pushd "%~dp0" >nul

rem Delete old output file
if exist "output.txt" del "output.txt" >nul

rem ONE single PowerShell command – this time we filter out ANY path that contains "conda" anywhere in the folder chain
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$root = (Get-Location).Path; " ^
  "Get-ChildItem -Path $root -Recurse -Filter '*.py' -File -ErrorAction SilentlyContinue " ^
  "| Where-Object { $_.FullName -notmatch '(?i)conda' } " ^
  "| Select-Object -ExpandProperty FullName " ^
  "| Sort-Object " ^
  "| Out-File -FilePath \"$root\output.txt\" -Encoding UTF8; " ^
  "$count = (Get-Content \"$root\output.txt\" | Measure-Object -Line).Lines; " ^
  "Write-Host \"Done! Found $count .py files (all conda folders completely skipped)\""

echo.
echo ==================================================
echo output.txt created – NO .conda files are inside
echo ==================================================
popd
pause