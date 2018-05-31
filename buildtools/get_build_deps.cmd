@echo Getting build tool dependencies for %1 ...
@call "%~dp0packman5\packman.cmd" pull "%~dp0build_platform_deps.xml" --platform %1
@if %ERRORLEVEL% neq 0 (
    @exit /b %errorlevel%
) else (
    @echo Done!
)

