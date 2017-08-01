@ECHO OFF
SET SCRIPT_PATH=%CD%

cd %SCRIPT_PATH%
call MocUI.bat

echo set ENV variables ...
rem SETLOCAL

REM Get the absolute path for \trunk\
@pushd ..\..\..\..
SET SRC_ROOT=%CD%
@popd

rem SET TOREMOVE=%CD:*\trunk=%
rem CALL SET SRC_ROOT=%%CD:%TOREMOVE%=%%
echo SRC_ROOT is %SRC_ROOT%

IF %SRC_ROOT%Nothing==Nothing EXIT /B 1

SET EXTERNAL_ROOT=%SRC_ROOT%\..\..\external

set VC_VERSION=vc12
set VS_STRING=VS2013
SET QTDIR=%EXTERNAL_ROOT%\Qt5.6.1\vc2013
call xpj4.exe -v 4 -t %VC_VERSION% -p win32 -p win64 -x CurveEditor.xml

set VC_VERSION=vc14
set VS_STRING=VS2015
SET QTDIR=%EXTERNAL_ROOT%\Qt5.6.1\vc2015
call xpj4.exe -v 4 -t %VC_VERSION% -p win32 -p win64 -x CurveEditor.xml

echo ""
echo if the following root paths are not right. please correct them
echo SRC_ROOT is %SRC_ROOT%
echo QTDIR root is %QTDIR%