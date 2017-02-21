@set PM_PACKMAN_VERSION=3.7

:: Read PM_PACKAGE_SOURCE from file
@set /p PM_PACKAGE_SOURCE=<"%~dp0\..\..\source.conf"
@echo PM_PACKAGE_SOURCE: %PM_PACKAGE_SOURCE%

:: The external root may already be configured and we should do minimal work in that case
@if defined PM_PACKAGES_ROOT goto ENSURE_DIR

:: If the folder isn't set we assume that the best place for it is on the drive that we are currently
:: running from
@set PM_DRIVE=%CD:~0,2%

@set PM_PACKAGES_ROOT=%PM_DRIVE%\NVIDIA\packman-repo

:: We use *setx* here so that the variable is persisted in the user environment
@echo Setting user environment variable PM_PACKAGES_ROOT to %PM_PACKAGES_ROOT%
@setx PM_PACKAGES_ROOT %PM_PACKAGES_ROOT%
@if errorlevel 1 goto ERROR

:: The above doesn't work properly from a build step in VisualStudio because a separate process is
:: spawned for it so it will be lost for subsequent compilation steps - VisualStudio must
:: be launched from a new process. We catch this odd-ball case here:
@if defined VSLANG goto ERROR_IN_VS_WITH_NO_ROOT_DEFINED

:: Check for the directory that we need. Note that mkdir will create any directories
:: that may be needed in the path 
:ENSURE_DIR
@if not exist "%PM_PACKAGES_ROOT%" (
	@echo Creating directory %PM_PACKAGES_ROOT%
	@mkdir "%PM_PACKAGES_ROOT%"
	@if errorlevel 1 goto ERROR_MKDIR_PACKAGES_ROOT
)

:: The Python interpreter may already be externally configured
@if defined PM_PYTHON_EXT (
	@set PM_PYTHON=%PM_PYTHON_EXT%
	@goto PACKMAN
)

@set PM_PYTHON_DIR=%PM_PACKAGES_ROOT%\python\2.7.6-windows-x86
@set PM_PYTHON=%PM_PYTHON_DIR%\python.exe

@if exist "%PM_PYTHON%" goto PACKMAN

@set PM_PYTHON_PACKAGE=python@2.7.6-windows-x86.exe
@for /f "delims=" %%a in ('powershell -ExecutionPolicy ByPass -NoLogo -NoProfile -File "%~dp0\generate_temp_file_name.ps1"') do @set TEMP_FILE_NAME=%%a
@set TARGET=%TEMP_FILE_NAME%.exe
@set PM_PYTHON_GUID=4AB33777-0B8F-418A-ACEC-E673D09F164C
@call "%~dp0fetch_file.cmd" %PM_PACKAGE_SOURCE% %PM_PYTHON_PACKAGE% %PM_PYTHON_GUID% %TARGET%
@if errorlevel 1 goto ERROR

@echo Unpacking ...
@%TARGET% -o"%PM_PYTHON_DIR%" -y
@if errorlevel 1 goto ERROR

@del %TARGET%

:PACKMAN
:: The packman module may already be externally configured
@if defined PM_MODULE_EXT (
	@set PM_MODULE=%PM_MODULE_EXT%
	@goto END
)

@set PM_MODULE_DIR=%PM_PACKAGES_ROOT%\packman\%PM_PACKMAN_VERSION%-common
@set PM_MODULE=%PM_MODULE_DIR%\packman.py

@if exist "%PM_MODULE%" goto END

@set PM_MODULE_PACKAGE=packman@%PM_PACKMAN_VERSION%-common.zip
@for /f "delims=" %%a in ('powershell -ExecutionPolicy ByPass -NoLogo -NoProfile -File "%~dp0\generate_temp_file_name.ps1"') do @set TEMP_FILE_NAME=%%a
@set TARGET=%TEMP_FILE_NAME%
@set PM_COMMON_GUID=24ED6205-CFE6-4A64-A4B3-4BD87B64279F
@call "%~dp0fetch_file.cmd" %PM_PACKAGE_SOURCE% %PM_MODULE_PACKAGE% %PM_COMMON_GUID% %TARGET%
@if errorlevel 1 goto ERROR

@echo Unpacking ...
@"%PM_PYTHON%" "%~dp0\install_package.py" %TARGET% "%PM_MODULE_DIR%"
@if errorlevel 1 goto ERROR

@del %TARGET%

@goto END

:ERROR_IN_VS_WITH_NO_ROOT_DEFINED
@echo The above is a once-per-computer operation. Unfortunately VisualStudio cannot pick up environment change
@echo unless *VisualStudio is RELAUNCHED*.
@echo NOTE: If you are launching VisualStudio from command line or command line utility make sure
@echo you have a fresh environment (relaunch the command line or utility).
@echo.
@exit /B 1

:ERROR_MKDIR_PACKAGES_ROOT
@echo Failed to automatically create packman packages repo at %PM_PACKAGES_ROOT%.
@echo Please set a location explicitly that packman has permission to write to, by issuing:
@echo.
@echo    setx PM_PACKAGES_ROOT {path-you-choose-for-storing-packman-packages-locally}
@echo.
@echo Then launch a new command console for the changes to take effect and run packman command again.
@exit /B 1

:ERROR
@echo !!! Failure while configuring local machine :( !!!
@exit /B 1

:END
