:: Reset errorlevel status so we are not inheriting this state from the calling process:
@call :RESET_ERROR
:: You can remove the call below if you do your own manual configuration of the dev machines
@call "%~dp0\configure\configure.bat"
@if errorlevel 1 exit /b 1
:: Everything below is mandatory
@if not defined PM_PYTHON goto :PYTHON_ENV_ERROR
@if not defined PM_MODULE goto :MODULE_ENV_ERROR

@"%PM_PYTHON%" "%PM_MODULE%" %*
@goto :eof

:: Subroutines below
:PYTHON_ENV_ERROR
@echo User environment variable PM_PYTHON is not set! Please configure machine for packman or call configure.bat.
@exit /b 1

:MODULE_ENV_ERROR
@echo User environment variable PM_MODULE is not set! Please configure machine for packman or call configure.bat.
@exit /b 1

:RESET_ERROR
@exit /b 0