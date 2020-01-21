@echo off
setlocal

set BLAST_PATH=..\..

if "%*"=="" (
  echo Usage: format-change.bat ^<p4 changelist #^>
  exit /b
)

set change=%~1
for /F %%d IN (directories.txt) do (
  call :format %BLAST_PATH%\%%d
)

exit /b

:format
for /F "delims=#" %%a in ('p4 opened -c %change% %~f1\....h, %~f1\....cpp, %~f1\....cu, %~f1\....hlsl 2^> NUL') do (
  for /F "tokens=3" %%f IN ('p4 where %%a') do (
    echo %%f
    clang-format.exe -i %%f
  )
)
