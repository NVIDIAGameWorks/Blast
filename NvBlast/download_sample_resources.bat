:: Reset errorlevel status so we are not inheriting this state from the calling process:
@call :CLEAN_EXIT

:: Run packman to pull resources
@echo Running packman to pull resources...
@call "%~dp0buildtools\packman\windows\packman.cmd" %PM_OPTIONS_EXT% pull "%~dp0\samples\resources\configs\resources.xml" --platform win
@if %ERRORLEVEL% neq 0 (
    @exit /b %errorlevel%
) else (
    @echo Success!
)

:CLEAN_EXIT
@exit /b 0