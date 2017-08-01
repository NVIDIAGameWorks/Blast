SET SRC_ROOT=..\..\..\..\..
SET EXTERNAL_ROOT=%SRC_ROOT%\..\..\external
SET BUILDTOOL_ROOT=%SRC_ROOT%\..\..\external

@if "%PERL%"=="" set PERL=%BUILDTOOL_ROOT%\perl\5.8.8_822\bin\perl
@%PERL% %EXTERNAL_ROOT%\NvParameterized\1.1\trunk\build\scripts\GenParameterized.pl -force PlaylistParams.pl . .