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
SET VC_VERSION=%5

SET PM_DRIVE=%CD:~0,2%
SET PM_PACKAGES_ROOT=%PM_DRIVE%\NVIDIA\packman-repo
echo PM_PACKAGES_ROOT is %PM_PACKAGES_ROOT%

SET PX_VERSION=1.0.21652946
SET PHYSX_VERSION=3.4.21652946
SET APEX_VERSION=1.4.21652946

SET BLAST_ROOT=%CURDIR%\..\..\..\..
SET NVTOOLSEXT=%PM_PACKAGES_ROOT%\nvToolsExt\1.0
SET WINSDK_PATH=%BLAST_ROOT%\..\..\external\D3D11\Bin

ECHO OUTPUTDIR: %OUTPUTDIR%
ECHO PLATFORM: %PLATFORM%
ECHO CONFIGURATION: %CONFIGURATION%
ECHO VC_VERSION: %VC_VERSION%
ECHO QTDIR: %QTDIR%

ECHO BLAST_ROOT=%BLAST_ROOT%
ECHO PHYSX_PATH=%PHYSX_PATH%
ECHO PX_SHARED=%PX_SHARED%
ECHO APEX_PATH=%APEX_PATH%
ECHO NVTOOLSEXT=%NVTOOLSEXT%
ECHO WINSDK_PATH=%WINSDK_PATH%

ECHO CURDIR: %CURDIR%

ECHO ---------------------------------------------------------------------------------
ECHO ArtistTools Post-Build Events...
ECHO ---------------------------------------------------------------------------------


IF /i "%PLATFORM%"=="Win32" (
	SET sourcePath=%QTDIR%\x86\bin
	
	SET PX_SHARED=%PM_PACKAGES_ROOT%\PxShared-%VC_VERSION%win32\%PX_VERSION%
	SET PHYSX_PATH=%PM_PACKAGES_ROOT%\PhysX-%VC_VERSION%win32\%PHYSX_VERSION%
	SET APEX_PATH=%PM_PACKAGES_ROOT%\Apex-%VC_VERSION%win32\%APEX_VERSION%
	
	ECHO %CURDIR%/../script/dllcopy.bat x86 %BLAST_ROOT%/bin/%VC_VERSION%win32-cmake %PHYSX_PATH%\bin\%VC_VERSION%win32-cmake %PX_SHARED%/bin/%VC_VERSION%win32-cmake %APEX_PATH%/bin/%VC_VERSION%win32-cmake %NVTOOLSEXT%\bin\Win32 %BLAST_ROOT%/shared/external/GraphicsLib %WINSDK_PATH% %BLAST_ROOT%/shared/external
	CALL %CURDIR%/../script/dllcopy.bat x86 %BLAST_ROOT%/bin/%VC_VERSION%win32-cmake %PHYSX_PATH%\bin\%VC_VERSION%win32-cmake %PX_SHARED%/bin/%VC_VERSION%win32-cmake %APEX_PATH%/bin/%VC_VERSION%win32-cmake %NVTOOLSEXT%\bin\Win32 %BLAST_ROOT%/shared/external/GraphicsLib %WINSDK_PATH% %BLAST_ROOT%/shared/external
			
) ELSE IF /i "%PLATFORM%"=="x64" (
	SET sourcePath=%QTDIR%\x64\bin
	
	SET PX_SHARED=%PM_PACKAGES_ROOT%\PxShared-%VC_VERSION%win64\%PX_VERSION%
	SET PHYSX_PATH=%PM_PACKAGES_ROOT%\PhysX-%VC_VERSION%win64\%PHYSX_VERSION%
	SET APEX_PATH=%PM_PACKAGES_ROOT%\Apex-%VC_VERSION%win64\%APEX_VERSION%
	
	ECHO %CURDIR%/../script/dllcopy.bat x64 %BLAST_ROOT%/bin/%VC_VERSION%win64-cmake %PHYSX_PATH%\bin\%VC_VERSION%win64-cmake %PX_SHARED%/bin/%VC_VERSION%win64-cmake %APEX_PATH%/bin/%VC_VERSION%win64-cmake %NVTOOLSEXT%\bin\x64 %BLAST_ROOT%/shared/external/GraphicsLib %WINSDK_PATH% %BLAST_ROOT%/shared/external
	CALL %CURDIR%/../script/dllcopy.bat x64 %BLAST_ROOT%/bin/%VC_VERSION%win64-cmake %PHYSX_PATH%\bin\%VC_VERSION%win64-cmake %PX_SHARED%/bin/%VC_VERSION%win64-cmake %APEX_PATH%/bin/%VC_VERSION%win64-cmake %NVTOOLSEXT%\bin\x64 %BLAST_ROOT%/shared/external/GraphicsLib %WINSDK_PATH% %BLAST_ROOT%/shared/external
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
	CALL :UpdateTarget Qt5Xml.dll  %OUTPUTDIR%
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
	
SET sourcePath=%OUTPUTDIR%..\..\tools\ArtistTools\project\Resource
CALL :UpdateTarget ArtistToolsTheme.qss    %OUTPUTDIR%

REM copy d3d and sdl dlls
IF /i "%PLATFORM%"=="Win32" (
	SET sourcePath=%OUTPUTDIR%..\..\..\..\external\D3D11\Bin\x86
	CALL :UpdateTarget d3dcompiler_47.dll %OUTPUTDIR%
	SET sourcePath=%OUTPUTDIR%..\..\..\..\external\SDL2-2.0.0\bin\x86
	CALL :UpdateTarget SDL2.dll %OUTPUTDIR%
) ELSE IF /i "%PLATFORM%"=="x64" (
	SET sourcePath=%OUTPUTDIR%..\..\..\..\external\D3D11\Bin\x64
	CALL :UpdateTarget d3dcompiler_47.dll %OUTPUTDIR%
	SET sourcePath=%OUTPUTDIR%..\..\..\..\external\SDL2-2.0.0\bin\x64
	CALL :UpdateTarget SDL2.dll %OUTPUTDIR%
)

REM copy resources. Only need shaders.
REM  /i creates target folder if it does not exist.
SET sourcePath=%OUTPUTDIR%
SET targetPath=%sourcePath:/=\%
echo XCOPY /y /r /d /s /i %targetPath%..\..\samples\resources\shaders %targetPath%..\resources\shaders
XCOPY /y /r /d /s /i %targetPath%..\..\samples\resources\shaders %targetPath%..\resources\shaders
echo XCOPY /y /r /d /s /i %targetPath%..\..\tools\ArtistTools\source\BlastPlugin\Shaders %targetPath%..\resources\shaders
XCOPY /y /r /d /s /i %targetPath%..\..\tools\ArtistTools\source\BlastPlugin\Shaders %targetPath%..\resources\shaders
REM we do not need models
rem SET MODELSFOLDER=%targetPath%..\resources\models
rem IF NOT EXIST %MODELSFOLDER% MKDIR %MODELSFOLDER%

REM copy UI
REM  /i creates target folder if it does not exist.
SET sourcePath=%OUTPUTDIR%
SET targetPath=%sourcePath:/=\%
echo XCOPY /y /r /d /s /i %targetPath%..\..\tools\ArtistTools\project\UI %targetPath%..\UI
XCOPY /y /r /d /s /i %targetPath%..\..\tools\ArtistTools\project\UI %targetPath%..\UI
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
