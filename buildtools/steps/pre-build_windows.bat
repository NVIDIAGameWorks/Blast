@pushd "%~dp0..\.."
@call generate_projects_vc14win64.bat
@call generate_projects_vc15win64.bat
@popd
@if %errorlevel% NEQ 0 exit /b 1

