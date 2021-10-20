@echo off
call "%~dp0tools\prebuild"
call "%~dp0repo" build %*
