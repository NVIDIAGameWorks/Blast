@echo off

rem NOTE : rem next line if shader code has changed
goto:eof

@echo ---start complie shaders---

setlocal enabledelayedexpansion 
set PROJECT_ROOT=..\..
set GenPath="%PROJECT_ROOT%\project\Generated\CoreLib"

echo commands to run
echo mkdir %GenPath%
echo rd /s /Q  %GenPath%\x64\Shaders
echo rd /s /Q  %GenPath%\win32\Shaders
echo mkdir %GenPath%\x64\Shaders
echo mkdir %GenPath%\win32\Shaders

mkdir %GenPath%
rd /s /Q  %GenPath%\x64\Shaders
rd /s /Q  %GenPath%\win32\Shaders
mkdir %GenPath%\x64\Shaders
mkdir %GenPath%\win32\Shaders

if "%DXSDK_DIR%"=="" set DXSDK_DIR=..\..\..\..\..\..\..\external\DXSDK\June_2010
echo DXSDK_DIR is %DXSDK_DIR%

:doprocess_hlsl_vs_ps
call:process_hlsl_vs_ps BodyShader
call:process_hlsl_vs_ps BodyShadow
call:process_hlsl_vs_ps color
call:process_hlsl_vs_ps Light
call:process_hlsl_vs_ps ScreenQuad
call:process_hlsl_vs_ps ScreenQuadColor
call:process_hlsl_vs_ps VisualizeShadow

:process_hlsl_vs_ps
 SET FILE_HLSL=%~1
 echo "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_5_0 /I..\..\..\..\src /Fh "%GenPath%\x64\Shaders\%FILE_HLSL%_VS.h" /Evs_main "%PROJECT_ROOT%\source\Shaders\%FILE_HLSL%.hlsl"
 "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_5_0 /I..\..\..\..\src /Fh "%GenPath%\x64\Shaders\%FILE_HLSL%_VS.h" /Evs_main "%PROJECT_ROOT%\source\Shaders\%FILE_HLSL%.hlsl"
 "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_5_0 /I..\..\..\..\src /Fh "%GenPath%\win32\Shaders\%FILE_HLSL%_VS.h" /Evs_main "%PROJECT_ROOT%\source\Shaders\%FILE_HLSL%.hlsl"
 echo "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_5_0 /I..\..\..\..\src /Fh "%GenPath%\x64\Shaders\%FILE_HLSL%_PS.h" /Eps_main "%PROJECT_ROOT%\source\Shaders\%FILE_HLSL%.hlsl"
 "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_5_0 /I..\..\..\..\src /Fh "%GenPath%\x64\Shaders\%FILE_HLSL%_PS.h" /Eps_main "%PROJECT_ROOT%\source\Shaders\%FILE_HLSL%.hlsl"
 "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_5_0 /I..\..\..\..\src /Fh "%GenPath%\win32\Shaders\%FILE_HLSL%_PS.h" /Eps_main "%PROJECT_ROOT%\source\Shaders\%FILE_HLSL%.hlsl"
goto:eof

:end
