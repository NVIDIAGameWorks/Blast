@ECHO OFF

call build_source

call build_api

@if errorlevel 0 goto :eof
echo Error while building documentation!!!!
exit /b 1
