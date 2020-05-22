@set BIN_PATH=%~dp0..\..\bin\%1-cmake\debug
@pushd "%BIN_PATH%"
@call BlastUnitTests.exe --gtest_output=xml:BlastUnitTestsDEBUG_x64.xml
@echo ##teamcity[importData type='gtest' parseOutOfDate='true' file='%BIN_PATH%\BlastUnitTestsDEBUG_x64.xml']
@popd
@if %errorlevel% NEQ 0 exit /b 1