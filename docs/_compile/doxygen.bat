@ECHO OFF

REM Command file for Doxygen build

rem %1 must be the name of the doxyfile (w/o extension) and folder to create (e.g. blast_api)

set NAME=%1

if "%NAME%" == "" (
  goto missing_parameter_1
)

set BLAST_ROOT=%~dp0..\..

:create_folder
if exist ..\"%NAME%"_docs (
  rmdir /s /q ..\"%NAME%"_docs
)

:: Run packman to ensure doxygen is there:
set DOXYGEN_VERSION=1.5.8-gameworks-win

call "%BLAST_ROOT%\buildtools\packman5\packman.cmd" install doxygen %DOXYGEN_VERSION%
if errorlevel 1 (
  echo ***SCRIPTERROR: packman failed to get dependencies
  goto doxygen_failed
)

:build_doxygen
set DOXYGEN_EXE="%PM_doxygen_PATH%\bin\doxygen.exe"
if not exist %DOXYGEN_EXE% (
  echo ***SCRIPTERROR: %DOXYGEN_EXE% not found. Please fix.
  goto doxygen_failed
)
%DOXYGEN_EXE% %NAME%.doxyfile
if errorlevel 1 (
  echo ***SCRIPTERROR: doxygen build error
  goto doxygen_failed
)

:copy_logo
robocopy .\ ..\\%NAME%_docs\files\ blast_logo.png
if not errorlevel 1 (
  echo ***SCRIPTERROR: copying logo image failed
  goto doxygen_failed
)

:copy_index_html
copy index.html ..\%NAME%_docs\ /Y
if errorlevel 1 (
  echo ***SCRIPTERROR: copying index.html failed
  goto doxygen_failed
)
goto doxygen_succeeded

:doxygen_failed
echo ***SCRIPTERROR: doxygen.bat failed
@exit /b 1

:missing_parameter_1
echo The first argument must be the name of the doxyfile (without extension)
goto doxygen_failed

:doxygen_succeeded
echo ***doxygen.bat succeeded
