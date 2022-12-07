@echo off

set dst=%1\blast

robocopy %~dp0.. %dst% /s /ns /nc /ndl /np /xd "_*" ".*" "tools" "exporter" "physx" "RT" "apps" "samples" /xf "NvBlastExtPx*" "NvBlastExtKJPx*" "ExtPx*" "ext_px*" "ext_physx*" "ext_exporter*"
robocopy %~dp0..\include\extensions\physx %dst%\include\extensions\physx NvBlastExtPxTask.h /ns /nc /ndl /np
robocopy %~dp0..\source\sdk\extensions\physx %dst%\source\sdk\extensions\physx NvBlastExtPxTaskImpl.* /ns /nc /ndl /np
robocopy %~dp0..\tools\packman %dst%\tools\packman /s /ns /nc /ndl /np
robocopy %~dp0..\tools\repoman %dst%\tools\repoman /s /ns /nc /ndl /np

call "%~dp0..\tools\packman\python" %~dp0edit_public_repo.py %dst%\premake5.lua
