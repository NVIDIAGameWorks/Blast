@cmd /c "%~dp0build_all_windows_vc15.bat"
@if %ERRORLEVEL% neq 0 goto ERROR

:: Success
@exit /B 0

:ERROR
@echo !!! Failure while building for Windows!!!
@exit /B 1
