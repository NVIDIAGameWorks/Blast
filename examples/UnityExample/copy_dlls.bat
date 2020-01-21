@echo off

CALL :COPY_DLL NvBlast_x64.dll ..\..\bin\vc14win64-cmake Assets\Plugins\Blast\x64
CALL :COPY_DLL NvBlastDEBUG_x64.dll ..\..\bin\vc14win64-cmake Assets\Plugins\Blast\x64
CALL :COPY_DLL NvBlastExtCommonDEBUG_x64.dll ..\..\bin\vc14win64-cmake Assets\Plugins\Blast\x64
CALL :COPY_DLL NvBlastExtCommon_x64.dll ..\..\bin\vc14win64-cmake Assets\Plugins\Blast\x64
CALL :COPY_DLL NvBlastExtShadersDEBUG_x64.dll ..\..\bin\vc14win64-cmake Assets\Plugins\Blast\x64
CALL :COPY_DLL NvBlastExtShaders_x64.dll ..\..\bin\vc14win64-cmake Assets\Plugins\Blast\x64
CALL :COPY_DLL NvBlastExtUtilsDEBUG_x64.dll ..\..\bin\vc14win64-cmake Assets\Plugins\Blast\x64
CALL :COPY_DLL NvBlastExtUtils_x64.dll ..\..\bin\vc14win64-cmake Assets\Plugins\Blast\x64

GOTO END

:COPY_DLL
if NOT EXIST %3 MKDIR %3
IF NOT EXIST %2\%1 (
	REM ECHO File doesn't exist %1
) ELSE (
	ECHO COPY "%2\%1" "%3\%1"
	COPY "%2\%1" "%3\%1" 
)
GOTO END

:END
