@echo off
SetLocal EnableDelayedExpansion
echo #############################################################################################
echo Starting %~n0 %date% %time%
echo #############################################################################################

echo PM_CMakeModules_VERSION %PM_CMakeModules_VERSION%

if NOT DEFINED PM_CMakeModules_VERSION GOTO DONT_RUN_STEP_2

IF NOT DEFINED PM_PACKAGES_ROOT GOTO PM_PACKAGES_ROOT_UNDEFINED

IF DEFINED PM_UE4LinuxToolchainV8_PATH SET LINUX_MULTIARCH_ROOT=%PM_UE4LinuxToolchainV8_PATH%

IF NOT DEFINED LINUX_MULTIARCH_ROOT GOTO LINUX_MULTIARCH_ROOT_UNDEFINED

::Also set legacy path since the LinuxCrossToolchain.x86_64-unknown-linux-gnu.cmake uses it
pushd "%LINUX_MULTIARCH_ROOT%\x86_64-unknown-linux-gnu"
SET LINUX_ROOT=%cd%
popd

REM Now set up the CMake command from PM_PACKAGES_ROOT
::Ugly hack, this will be the linux one, due to packman downloading Linux binaries, so swap the path
SET CMAKECMD=%PM_cmake_PATH_Win%\bin\cmake.exe

echo Cmake: %CMAKECMD%


REM Generate projects here

echo.
echo #############################################################################################
ECHO "Creating Linux UE4 Cross-compile NMake files"

echo -DUE4_LINUX_CROSSCOMPILE=TRUE -DCMAKE_TOOLCHAIN_FILE=%PM_CMakeModules_PATH%/Linux/LinuxCrossToolchain.x86_64-unknown-linux-gnu.cmake -DTARGET_BUILD_PLATFORM=linux -DBL_LIB_OUTPUT_DIR=%BLAST_ROOT_DIR%/lib/linux64-UE4 -DBL_DLL_OUTPUT_DIR=%BLAST_ROOT_DIR%/bin/linux64-UE4 -DBL_EXE_OUTPUT_DIR=%BLAST_ROOT_DIR%/bin/linux64-UE4

SET CMAKE_CMD_LINE_PARAMS=-DUE4_LINUX_CROSSCOMPILE=TRUE -DCMAKE_TOOLCHAIN_FILE=%PM_CMakeModules_PATH%/Linux/LinuxCrossToolchain.x86_64-unknown-linux-gnu.cmake -DTARGET_BUILD_PLATFORM=linux -DBL_LIB_OUTPUT_DIR=%BLAST_ROOT_DIR%/lib/linux64-UE4 -DBL_DLL_OUTPUT_DIR=%BLAST_ROOT_DIR%/bin/linux64-UE4 -DBL_EXE_OUTPUT_DIR=%BLAST_ROOT_DIR%/bin/linux64-UE4

echo CMAKE_CMD_LINE_PARAMS
echo %CMAKE_CMD_LINE_PARAMS%

if not exist %BLAST_ROOT_DIR%/bin/linux64-UE4 mkdir %BLAST_ROOT_DIR%/bin/linux64-UE4

::Seems like there are no checked or profile Linux PhysX Libs
FOR %%Z IN (debug, release, checked, profile) DO (
::FOR %%Z IN (debug, release) DO (
    SET CMAKE_OUTPUT_DIR=%BLAST_ROOT_DIR%\compiler\linux64-%%Z-UE4\
    IF EXIST !CMAKE_OUTPUT_DIR! rmdir /S /Q !CMAKE_OUTPUT_DIR!
    mkdir !CMAKE_OUTPUT_DIR!
    pushd !CMAKE_OUTPUT_DIR!
    "%CMAKECMD%" %BLAST_ROOT_DIR% -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=%%Z %CMAKE_CMD_LINE_PARAMS%
    popd
    if !ERRORLEVEL! NEQ 0 exit /b !ERRORLEVEL!
)


GOTO :End

:LINUX_MULTIARCH_ROOT_UNDEFINED
ECHO LINUX_MULTIARCH_ROOT has to be defined, pointing to UE4 Linux toolchain
PAUSE
GOTO END

:PM_PACKAGES_ROOT_UNDEFINED
ECHO PM_PACKAGES_ROOT has to be defined, pointing to the root of the dependency tree.
PAUSE
GOTO END

:DONT_RUN_STEP_2
ECHO Don't run this batch file directly. Run generate_projects_(platform).bat instead
PAUSE
GOTO END

:End
