@if "%PERL%"=="" set PERL=..\..\..\..\..\..\..\external\perl\5.8.8_822\bin\perl.exe
@%PERL% ..\..\..\..\..\..\..\external\NvParameterized\1.1\trunk\build\scripts\GenParameterized.pl -force BlastProjectParams.pl . .

