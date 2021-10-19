@call "%~dp0..\..\..\..\_build\windows-x86_64\debug\blast-sdk\bin\UnitTests.exe" --gtest_output=xml:"%~dp0..\..\..\..\_build\windows-x86_64\debug\blast-sdk\bin\UnitTests.xml"
@echo ##teamcity[importData type='gtest' parseOutOfDate='true' file='%~dp0..\..\..\..\_build\windows-x86_64\debug\blast-sdk\bin\UnitTests.xml']
@if %errorlevel% NEQ 0 exit /b 1