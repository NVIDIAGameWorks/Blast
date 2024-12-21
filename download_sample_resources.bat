:: Reset errorlevel status so we are not inheriting this state from the calling process:
@call :CLEAN_EXIT

:: Run packman to pull resources
@echo Running packman to pull resources...
@call "%~dp0buildtools\packman\packman.cmd" pull "%~dp0\samples\resources\configs\resources.xml"
@if %ERRORLEVEL% neq 0 (
    @exit /b %errorlevel%
) else (
    @echo Success!
)

:CLEAN_EXIT
@exit /b 0