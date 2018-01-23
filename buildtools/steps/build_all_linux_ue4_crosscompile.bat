:: Setup VS2015 build environment
@call "%VS140COMNTOOLS%VsdevCmd.bat"

@set ROOT_PATH=%~dp0..\..\compiler

@call :BUILD
@if %ERRORLEVEL% neq 0 goto ERROR

:: Success
@exit /B 0

:ERROR
@echo Failure while building *Linux UE4 cross-compile* targets!
@exit /B 1

:BUILD
@echo | set /p dummyName=** Building debug ... **
@pushd
@cd "%ROOT_PATH%\linux64-debug-UE4"
@nmake
@popd
@echo ** End of debug **
@echo.
@if %ERRORLEVEL% neq 0 exit /B

@echo | set /p dummyName=** Building release ... **
@pushd
@cd "%ROOT_PATH%\linux64-release-UE4"
@nmake
@popd
@echo ** End of release **
@echo.
@if %ERRORLEVEL% neq 0 exit /B

@echo | set /p dummyName=** Building profile ... **
@pushd
@cd "%ROOT_PATH%\linux64-profile-UE4"
@nmake
@popd
@echo ** End of profile **
@echo.
@if %ERRORLEVEL% neq 0 exit /B

@echo | set /p dummyName=** Building checked ... **
@pushd
@cd "%ROOT_PATH%\linux64-checked-UE4"
@nmake
@popd
@echo ** End of checked **
@echo.
@exit /B