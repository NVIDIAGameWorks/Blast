:: You need to specify <package-source> <package-name> <package-guid> <target-name> as input to this command

@set PACKAGE_NAME=%2
@set PACKAGE_GUID=%3
@set TARGET=%4

@echo Fetching %PACKAGE_NAME% from %1 ...

@if ["%1"] EQU ["s3"] set SOURCE_OPT=-sourceName %PACKAGE_NAME%
@if ["%1"] EQU ["gtl"] set SOURCE_OPT=-sourceGUID %PACKAGE_GUID%

@powershell -ExecutionPolicy ByPass -NoLogo -NoProfile -File "%~dp0fetch_file_from_%1.ps1" %SOURCE_OPT% -output %TARGET%
:: A bug in powershell prevents the errorlevel code from being set when using the -File execution option
:: We must therefore do our own failure analysis, basically make sure the file exists and is larger than 0 bytes:
@if not exist %TARGET% goto ERROR_DOWNLOAD_FAILED
@if %~z4==0 goto ERROR_DOWNLOAD_FAILED

@exit /b 0

:ERROR_DOWNLOAD_FAILED
@echo Failed to download file from %1
@echo Most likely because endpoint cannot be reached (VPN connection down?)
@exit /b 1