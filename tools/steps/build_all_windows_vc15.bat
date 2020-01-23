:: Setup VS2017 build environment
set COMMAND="C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
FOR /F "tokens=*" %%i IN ('%COMMAND%') do (SET "COMNTOOLS=%%i\Common7\Tools\")
@call "%COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsx86_amd64.bat"

:: Note that we use /build rather than /rebuild because cmake cleans the directories when we
:: generate the projects (making /rebuild redundant since it's basically /clean + /build). 
:: By using /build these bat files can be useful during regular development (to verify changes)
:: Will rename them from rebuild to build at a future point in time.

@set ROOT_PATH=%~dp0..\..\compiler

:: Windows 'all'
@set SOLUTION_PATH=vc15win64-cmake\BlastAll.sln
@call :BUILD
@if %ERRORLEVEL% neq 0 goto ERROR

:: Success
@exit /B 0

:ERROR
@echo Failure while building *Windows vc15* targets!
@exit /B 1

:BUILD
@echo | set /p dummyName=** Building %SOLUTION_PATH% debug ... **
@devenv "%ROOT_PATH%\%SOLUTION_PATH%" /build "debug"
@echo ** End of %SOLUTION_PATH% debug **
@echo.
@if %ERRORLEVEL% neq 0 exit /B

@echo | set /p dummyName=** Building %SOLUTION_PATH% profile ... **
@devenv "%ROOT_PATH%\%SOLUTION_PATH%" /build "profile"
@echo ** End of %SOLUTION_PATH% profile **
@echo.

@echo | set /p dummyName=** Building %SOLUTION_PATH% checked ... **
@devenv "%ROOT_PATH%\%SOLUTION_PATH%" /build "checked"
@echo ** End of %SOLUTION_PATH% checked **
@echo.

@echo | set /p dummyName=** Building %SOLUTION_PATH% release ... **
@devenv "%ROOT_PATH%\%SOLUTION_PATH%" /build "release"
@echo ** End of %SOLUTION_PATH% release **
@echo.
@exit /B