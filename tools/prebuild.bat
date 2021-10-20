@echo off

set CAPNP_GEN=%~dp0..\_capnp
if not exist "%CAPNP_GEN%" mkdir "%CAPNP_GEN%"

set CAPNP_DIR=%~dp0..\_build\host-deps\CapnProto
set CAPNP_BIN=%CAPNP_DIR%\tools\win32
set CAPNP_SRC=%CAPNP_DIR%\src

set SRC_DIR=%~dp0..\source\sdk\extensions\serialization

%CAPNP_BIN%\capnp compile -o %CAPNP_BIN%\capnpc-c++:%CAPNP_GEN% -I %CAPNP_SRC% --src-prefix %SRC_DIR% %SRC_DIR%/NvBlastExtLlSerialization-capn
%CAPNP_BIN%\capnp compile -o %CAPNP_BIN%\capnpc-c++:%CAPNP_GEN% -I %CAPNP_SRC% --src-prefix %SRC_DIR% %SRC_DIR%/NvBlastExtTkSerialization-capn
