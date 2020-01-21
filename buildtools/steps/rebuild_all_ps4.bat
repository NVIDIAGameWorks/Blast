:: Setup VS2015 build environment
@call "%VS140COMNTOOLS%VsdevCmd.bat"

@set ROOT_PATH=%~dp0..\..\compiler
:: PS4 targets
@devenv "%ROOT_PATH%\vc14ps4-cmake\BlastAll.sln" /rebuild "debug"
@if %ERRORLEVEL% neq 0 goto ERROR

@devenv "%ROOT_PATH%\vc14ps4-cmake\BlastAll.sln" /rebuild "profile"
@if %ERRORLEVEL% neq 0 goto ERROR

@devenv "%ROOT_PATH%\vc14ps4-cmake\BlastAll.sln" /rebuild "checked"
@if %ERRORLEVEL% neq 0 goto ERROR

@devenv "%ROOT_PATH%\vc14ps4-cmake\BlastAll.sln" /rebuild "release"
@if %ERRORLEVEL% neq 0 goto ERROR

:: Success
@exit /B 0

:ERROR
@echo Failure while building *PS4* targets!
@exit /B 1
