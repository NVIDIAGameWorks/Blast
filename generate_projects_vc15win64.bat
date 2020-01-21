:: Reset errorlevel status so we are not inheriting this state from the calling process:
@call :CLEAN_EXIT

:: Set the blast root to the current directory so that included solutions that aren't Blast know where the root is without having to
:: guess or hardcode a relative path.
:: Use the "short" path so that we don't have to quote paths in that calls below. If we don't do that spaces can break us.
@SET BLAST_ROOT_DIR=%~sdp0

:: Run packman to ensure dependencies are present and run cmake generation script afterwards
@call "%~dp0buildtools\get_build_deps.cmd" win.msvc
@if %ERRORLEVEL% neq 0 exit /b %errorlevel%

@echo Getting target platform dependencies for win.x86_64.vc150 ...
@call "%~dp0buildtools\packman5\packman.cmd" pull "%~dp0target_platform_deps.xml" --platform win.x86_64.vc150 --postscript "%~dp0buildtools\cmake_projects_vc15win64.bat"
@if %ERRORLEVEL% neq 0 (
    @exit /b %errorlevel%
) else (
    @echo Success!
)

:CLEAN_EXIT
@exit /b 0