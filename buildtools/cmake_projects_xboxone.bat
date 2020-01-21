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
ECHO "Creating VS2015 XboxOne"

SET CMAKE_OUTPUT_DIR=compiler\vc14xboxone-cmake\
IF EXIST %CMAKE_OUTPUT_DIR% rmdir /S /Q %CMAKE_OUTPUT_DIR%
mkdir %CMAKE_OUTPUT_DIR%
pushd %CMAKE_OUTPUT_DIR%
%CMAKECMD% %BLAST_ROOT_DIR% -G "Visual Studio 14 2015" -DTARGET_BUILD_PLATFORM=XboxOne -DCMAKE_GENERATOR_PLATFORM=Durango -DXDK_VERSION=160803 -DCMAKE_TOOLCHAIN_FILE="%CMAKE_MODULE_PATH%\xboxone\XboxOneToolchain.txt" -DBL_LIB_OUTPUT_DIR=%BLAST_ROOT_DIR%\lib\vc14xboxone-cmake\ -DBL_DLL_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc14xboxone-cmake\ -DBL_EXE_OUTPUT_DIR=%BLAST_ROOT_DIR%\bin\vc14xboxone-cmake\
popd
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

:ADD_DEPLOYMENT
@echo Adding auto-deploy
@"%PM_PYTHON%" "%~dp0platform\xboxone\add_xboxone_autodeploy.py" "%CMAKE_OUTPUT_DIR%BlastAll.sln" "BlastUnitTests" "BlastPerfTests"
@"%PM_PYTHON%" "%~dp0platform\xboxone\add_xboxone_autodeploy.py" "%CMAKE_OUTPUT_DIR%test_bin\BlastTests.sln" "BlastUnitTests" "BlastPerfTests"

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
