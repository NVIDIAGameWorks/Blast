@echo off
:: Run packman to ensure required Python is present:
set PYTHON_VERSION=2.7.14-windows-x86_32
set BLAST_ROOT=%~dp0..\..
call "%BLAST_ROOT%\tools\packman5\packman.cmd" install python %PYTHON_VERSION%
IF %ERRORLEVEL% NEQ 0 EXIT /B 1

"%PM_python_PATH%\python.exe" "%~dp0\create_packages.py" %*
IF %ERRORLEVEL% NEQ 0 EXIT /B 1