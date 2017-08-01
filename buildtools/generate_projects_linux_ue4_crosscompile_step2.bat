@echo off
:: Reset errorlevel status so we are not inheriting this state from the calling process:
@call :CLEAN_EXIT

:: Run packman to ensure dependencies are present and run cmake generation script afterwards
@echo Running packman in preparation for cmake ...
@echo.

:: We need this for NMake
@call "%VS140COMNTOOLS%VsdevCmd.bat"

::Save this for when it's overwritten with the Linux one
set PM_cmake_PATH_Win=%PM_cmake_PATH%

@call "%~dp0packman\packman.cmd" pull "%BLAST_ROOT_DIR%\dependencies.xml" --platform linux-UE4-cross --postscript "%~dp0cmake_projects_linux_ue4_crosscompile.bat"
@if %ERRORLEVEL% neq 0 (
    @exit /b %errorlevel%
)

:CLEAN_EXIT
@exit /b 0