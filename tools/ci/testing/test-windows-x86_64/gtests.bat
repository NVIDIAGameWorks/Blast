@set BIN_PATH=%~dp0..\..\..\..\_build\windows-x86_64\debug\blast-sdk\bin
@pushd "%BIN_PATH%"
@call UnitTests.exe --gtest_output=xml:UnitTests.xml
@echo ##teamcity[importData type='gtest' parseOutOfDate='true' file='%BIN_PATH%\UnitTests.xml']
@popd
@if %errorlevel% NEQ 0 exit /b 1