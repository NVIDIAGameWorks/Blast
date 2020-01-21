@set ROOT_PATH=%~dp0..\..

::Remove old builds to keep things clean (important on build agents so we don't fill them up)
@del /q /f "%ROOT_PATH%\blast_*.zip"

@pushd "%ROOT_PATH%\docs\_compile"
@call build_all.bat
@popd
@if %errorlevel% NEQ 0 goto :ERROR

@set VERSION=%1
@set OPTIONS=
@if ["%VERSION%"] NEQ [""] set OPTIONS=-v %VERSION%

@call "%ROOT_PATH%\buildtools\packager\create_packages.bat" %OPTIONS% ps4
@if %ERRORLEVEL% EQU 0 goto :eof

:ERROR
@echo Failure during packaging for PS4!!!
@exit /b 1