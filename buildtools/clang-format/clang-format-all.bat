@echo off
setlocal enabledelayedexpansion

set BLAST_PATH=..\..

set /a count=0
set /a fail=0
for /F %%d IN (directories.txt) do (
  call :check %BLAST_PATH%\%%d
)

echo %fail% of %count% files reformatted.
exit /b %fail%

:check
for /R "%1" %%f in (*.h, *.cpp, *.cu, *.hlsl) do (
  p4 edit %%f
  clang-format.exe -i %%f
  if errorlevel 1 (
    echo %%f
    set /a fail += 1
  )
  set /a count += 1
)
