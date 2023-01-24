@echo off

if "%1"=="" (
echo Missing argument: must give public repo root path as the argument.
exit /b
)

set dst=%1\blast

RD /S /Q %dst%

robocopy %~dp0.. %dst% /s /ns /nc /ndl /np ^
/xd "_*" ".*" "docs" "tools" "exporter" "physx" "RT" "apps" "samples" ^
/xf "NvBlastExtPx*" "NvBlastExtKJPx*" "ExtPx*" "ext_px*" "ext_physx*" "ext_exporter*" "tinyobjloader-LICENSE.md" "*.rst" "CODEOWNERS"
robocopy %~dp0..\docs %dst%\docs CHANGELOG.md /ns /nc /ndl /np
robocopy %~dp0..\tools\packman %dst%\tools\packman /s /ns /nc /ndl /np
robocopy %~dp0..\tools\repoman %dst%\tools\repoman /s /ns /nc /ndl /np /xd "_*"

set edit_file_list=premake5.lua repo.toml deps\repo-deps.packman.xml deps\target-deps.packman.xml

for %%f in (%edit_file_list%) do (
    call "%~dp0..\tools\packman\python" %~dp0edit_public_repo.py %dst%\%%f
)
