@echo off
setlocal

rem Generates a directory index (ASCII tree) for the folder containing this script.
set "ROOT=%~dp0"
pushd "%ROOT%" >nul

set "OUTBASE=LLM_DIRECTORY_INDEX"
set "FULL=%ROOT%%OUTBASE%_FULL.txt"

echo Directory index root: "%ROOT%" > "%FULL%"
tree /F /A >> "%FULL%"

rem Chunk the full index into smaller files for LLM-friendliness, then zip them.
set "CHUNKSIZE=1000"
set "ZIP=%ROOT%%OUTBASE%_CHUNKS.zip"

powershell -NoProfile -Command ^
  "$Root = '%ROOT%';" ^
  "$Full = '%FULL%';" ^
  "$ChunkSize = %CHUNKSIZE%;" ^
  "$OutBase = '%OUTBASE%';" ^
  "$Zip = '%ZIP%';" ^
  "$lines = Get-Content -Path $Full;" ^
  "for ($i = 0; $i -lt $lines.Count; $i += $ChunkSize) {" ^
  "  $end = [Math]::Min($i + $ChunkSize - 1, $lines.Count - 1);" ^
  "  $chunkLines = $lines[$i..$end];" ^
  "  $chunkIndex = '{0:D3}' -f ($i / $ChunkSize + 1);" ^
  "  $chunkPath = Join-Path $Root ($OutBase + '_' + $chunkIndex + '.txt');" ^
  "  Set-Content -Path $chunkPath -Value $chunkLines -Encoding UTF8;" ^
  "}" ^
  "if (Test-Path $Zip) { Remove-Item $Zip -Force; }" ^
  "$chunkFiles = Get-ChildItem -Path $Root -Filter ($OutBase + '_*.txt');" ^
  "if ($chunkFiles) { Compress-Archive -Path $chunkFiles.FullName -DestinationPath $Zip; }"

popd >nul
echo Directory index written to "%FULL%"
echo Chunked files and zip (if any) written with base "%OUTBASE%_*" and "%ZIP%"

endlocal
