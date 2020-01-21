@echo off

echo #############################################################################################
echo Starting %~n0 %date% %time%
echo #############################################################################################

echo PM_CMakeModules_VERSION %PM_CMakeModules_VERSION%

if NOT DEFINED PM_CMakeModules_VERSION GOTO DONT_RUN_STEP_2

IF NOT DEFINED PM_PACKAGES_ROOT GOTO PM_PACKAGES_ROOT_UNDEFINED

REM Now set up the CMake command from PM_PACKAGES_ROOT

SET CMAKECMD=%PM_cmake_PATH%\bin\cmake.exe

SET CMAKE_MODULE_PATH=%PM_CMakeModules_PATH%

echo "Cmake: %CMAKECMD%"

REM Generate projects here
echo.
echo #############################################################################################
ECHO "Creating VS2015 PS4"

SET CMAKE_OUTPUT_DIR=compiler\vc14ps4-cmake\
IF EXIST %CMAKE_OUTPUT_DIR% rmdir /S /Q %CMAKE_OUTPUT_DIR%
mkdir %CMAKE_OUTPUT_DIR%
pushd %CMAKE_OUTPUT_DIR%
%CMAKECMD% %BLAST_ROOT_DIR% -G "Visual Studio 14 2015" -Wno-dev -DTARGET_BUILD_PLATFORM=PS4 -DCMAKE_TOOLCHAIN_FILE="%CMAKE_MODULE_PATH%\ps4\PS4Toolchain.txt" -DCMAKE_GENERATOR_PLATFORM=ORBIS -DBL_LIB_OUTPUT_DIR=%BLAST_ROOT_DIR%\lib\vc14ps4-cmake\ -DBL_DLL_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc14ps4-cmake\ -DBL_EXE_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc14ps4-cmake\
popd
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

GOTO :End

:PM_PACKAGES_ROOT_UNDEFINED
ECHO PM_PACKAGES_ROOT has to be defined, pointing to the root of the dependency tree.
PAUSE
GOTO END

:DONT_RUN_STEP_2
ECHO Don't run this batch file directly. Run generate_projects_(platform).bat instead
PAUSE
GOTO END

:End
