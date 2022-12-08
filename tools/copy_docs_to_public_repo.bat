@echo off

set src=%~dp0..\_build\docs\blast-sdk\latest
set dst=%1\blast

RD /S /Q %dst%

robocopy %src%\_build %dst%\_build /s /ns /nc /ndl /np
robocopy %src%\_images %dst%\_images /s /ns /nc /ndl /np
robocopy %src%\_sphinx_design_static %dst%\_sphinx_design_static /s /ns /nc /ndl /np
robocopy %src%\_static %dst%\_static /s /ns /nc /ndl /np
robocopy %src%\docs %dst%\docs /s /ns /nc /ndl /np
robocopy %src% %dst% *.html objects.inv project.json searchindex.js VERSION /s /ns /nc /ndl /np
