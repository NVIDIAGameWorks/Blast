@echo off
if not exist "%~dp0_capnp" mkdir "%~dp0_capnp"
call "%~dp0repo" build %*
