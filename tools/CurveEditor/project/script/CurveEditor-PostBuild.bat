Rem @ECHO OFF

SETLOCAL

ECHO Output Directory: %1
rem CD %1
SET CURDIR=%CD%
ECHO CURDIR: %CURDIR%

SET OUTPUTDIR=%1
SET PLATFORM=%2
SET CONFIGURATION=%3
SET QTDIR=%4

ECHO OUTPUTDIR: %OUTPUTDIR%
ECHO PLATFORM: %PLATFORM%
ECHO CONFIGURATION: %CONFIGURATION%
ECHO QTDIR: %QTDIR%
ECHO CURDIR: %CURDIR%

ECHO ---------------------------------------------------------------------------------
ECHO FurViwer Post-Build Events...
ECHO ---------------------------------------------------------------------------------

IF /i "%PLATFORM%"=="Win32" (
	SET sourcePath=%QTDIR%\x86\bin
) ELSE IF /i "%PLATFORM%"=="x64" (
	SET sourcePath=%QTDIR%\x64\bin
)
ECHO sourcePath: %sourcePath%

IF /i "%CONFIGURATION%"=="Debug" (
	CALL :UpdateTarget Qt5Cored.dll     %OUTPUTDIR%
	CALL :UpdateTarget Qt5Guid.dll      %OUTPUTDIR%
	CALL :UpdateTarget Qt5Widgetsd.dll  %OUTPUTDIR%
	CALL :UpdateTarget Qt5Xmld.dll  %OUTPUTDIR%
) ELSE (
	CALL :UpdateTarget Qt5Core.dll      %OUTPUTDIR%
	CALL :UpdateTarget Qt5Gui.dll       %OUTPUTDIR%
	CALL :UpdateTarget Qt5Widgets.dll   %OUTPUTDIR%
	CALL :UpdateTarget Qt5Xml.dll   %OUTPUTDIR%
)

SET PLATFORMSFOLDER=%OUTPUTDIR%platforms\
ECHO PLATFORMSFOLDER: %PLATFORMSFOLDER%

IF NOT EXIST %PLATFORMSFOLDER%   MKDIR %PLATFORMSFOLDER%

rem CD %PLATFORMSFOLDER%
IF /i "%PLATFORM%"=="Win32" (
	SET sourcePath=%QTDIR%\x86\plugins\platforms
) ELSE IF /i "%PLATFORM%"=="x64" (
	SET sourcePath=%QTDIR%\x64\plugins\platforms
)
ECHO sourcePath: %sourcePath%
ECHO CURDIR: %CURDIR%

IF /i "%CONFIGURATION%"=="Debug" (
	CALL :UpdateTarget qwindowsd.dll    %PLATFORMSFOLDER%
) ELSE (
	CALL :UpdateTarget qwindows.dll     %PLATFORMSFOLDER%
)

GOTO END_POST_BUILD
	
:UpdateTarget
SET str=%2
SET toPath=%str:/=\%
SET str=%sourcePath%\%1
SET fromPath=%str:/=\%
IF NOT EXIST %2%1 (
	rem ECHO CURDIR: %CURDIR%
	echo XCOPY %fromPath% %toPath% /R /Y
	XCOPY %fromPath% %toPath%  /R /Y
) ELSE (
	rem ECHO CURDIR: %CURDIR%
	echo XCOPY %fromPath% %toPath%  /R /Y
	XCOPY %fromPath% %toPath%  /R /Y
)

GOTO END

:END_POST_BUILD
ENDLOCAL

:END
