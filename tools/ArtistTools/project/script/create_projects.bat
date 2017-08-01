@ECHO OFF

call GenerateParamsClasses.bat

call GenerateUI.bat

call GenerateShade.bat

call create_projectsOnly.bat
