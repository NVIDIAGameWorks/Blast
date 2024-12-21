@echo off
:: Reset errorlevel status so we are not inheriting this state from the calling process:
@call :CLEAN_EXIT

:: Set the blast root to the current directory so that included solutions that aren't Blast know where the root is without having to
:: guess or hardcode a relative path.
:: Use the "short" path so that we don't have to quote paths in that calls below. If we don't do that spaces can break us.
@SET BLAST_ROOT_DIR=%~sdp0

:: Run packman to ensure dependencies are present and run cmake generation script afterwards
@call "%~dp0buildtools\get_build_deps.cmd" win.linux-UE4-cross
@if %ERRORLEVEL% neq 0 (
    @exit /b %errorlevel%
)

::Need linux libs, but windows tools, run us again to get the path
@call "%~dp0buildtools\packman\packman.cmd" pull "%~dp0target_platform_deps.xml" --platform linux --postscript "%~dp0buildtools\cmake_projects_linux_ue4_crosscompile.bat"
@if %ERRORLEVEL% neq 0 (
    @exit /b %errorlevel%
) else (
    @echo Success!
)

:CLEAN_EXIT
@exit /b 0