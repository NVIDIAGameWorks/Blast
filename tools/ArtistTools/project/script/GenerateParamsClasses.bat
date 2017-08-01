@ECHO OFF
SET SCRIPT_PATH=%CD%

cd %SCRIPT_PATH%
cd ..\..\source\CoreLib\Parameters
call go.bat

cd %SCRIPT_PATH%
cd ..\..\source\BlastPlugin\Parameters
call go.bat

cd %SCRIPT_PATH%

