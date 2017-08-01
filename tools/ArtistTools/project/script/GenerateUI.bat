@echo off

@echo ---start Moc UI---

setlocal enabledelayedexpansion 
set PROJECT_ROOT=..\..

SET SRC_ROOT=..\..\..\..
SET EXTERNAL_ROOT=%SRC_ROOT%\..\..\external

set QTPath=%EXTERNAL_ROOT%\Qt5.6.1\vc2015\x86\bin

set GenPath="%PROJECT_ROOT%\project\Generated\CoreLib"
set GenMocPath=%GenPath%\moc
set GenUIPath=%GenPath%\ui
set GenQRCPath=%GenPath%\qrc

rem goto mocBlastPluginCPP
rem goto domocHair

echo commands to run
echo mkdir %GenMocPath%
echo mkdir %GenUIPath%
echo mkdir %GenQRCPath%
rd /s /Q  %GenMocPath%
rd /s /Q  %GenUIPath%
rd /s /Q  %GenQRCPath%
mkdir %GenMocPath%
mkdir %GenUIPath%
mkdir %GenQRCPath%

:domoc

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\CoreLib\*.h"') do ( 
	findstr /i "Q_OBJECT" "%%a">nul&&( 
		echo %QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
		%QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
	)
)

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\CoreLib\Window\*.h"') do ( 
	findstr /i "Q_OBJECT" "%%a">nul&&( 
		echo %QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
		%QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
	)
)

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\CoreLib\UI\*.ui"') do ( 
	echo %QTPath%\uic.exe %%a -o %GenUIPath%\ui_%%~na.h
	%QTPath%\uic.exe %%a -o %GenUIPath%\ui_%%~na.h
)

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\CoreLib\UI\*.qrc"') do ( 
	echo %QTPath%\rcc.exe %%a -name %%a  -no-compress -o %GenQRCPath%\qrc_%%~na.cpp
	%QTPath%\rcc.exe %%a -name %%a  -no-compress -o %GenQRCPath%\qrc_%%~na.cpp
)

:mocAppMainWindowCPP

echo copy %GenMocPath%\moc_AppMainWindow.cpp %GenMocPath%\..\moc_AppMainWindow.cpp
copy %GenMocPath%\moc_AppMainWindow.cpp %GenMocPath%\..\moc_AppMainWindow.cpp

echo %QTPath%\moc.exe "%PROJECT_ROOT%\source\CoreLib\Window\AppMainWindow.h" -o %GenMocPath%\moc_AppMainWindow.cpp -DNV_ARTISTTOOLS
%QTPath%\moc.exe "%PROJECT_ROOT%\source\CoreLib\Window\AppMainWindow.h" -o %GenMocPath%\moc_AppMainWindow.cpp -DNV_ARTISTTOOLS

@echo ---end Moc UI for CoreLib---
:domocHair

@echo ---start Moc UI for BlastPlugin---
set GenPath="%PROJECT_ROOT%\project\Generated\BlastPlugin"
set GenMocPath=%GenPath%\moc
set GenUIPath=%GenPath%\ui
set GenQRCPath=%GenPath%\qrc

echo commands to run
echo mkdir %GenMocPath%
echo mkdir %GenUIPath%
echo mkdir %GenQRCPath%
rd /s /Q  %GenMocPath%
rd /s /Q  %GenUIPath%
rd /s /Q  %GenQRCPath%
mkdir %GenMocPath%
mkdir %GenUIPath%
mkdir %GenQRCPath%

:domocHairStart
for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\BlastPlugin\*.h"') do ( 
	findstr /i "Q_OBJECT" "%%a">nul&&( 
		echo %QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp "-I./../../source/CoreLib"
		%QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp "-I./../../source/CoreLib"
	)
)

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\BlastPlugin\Window\*.h"') do ( 
	findstr /i "Q_OBJECT" "%%a">nul&&( 
		echo %QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
		%QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
	)
)

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\BlastPlugin\UI\*.ui"') do ( 
	echo %QTPath%\uic.exe %%a -o %GenUIPath%\ui_%%~na.h
	%QTPath%\uic.exe %%a -o %GenUIPath%\ui_%%~na.h
)

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\BlastPlugin\UI\*.qrc"') do ( 
	echo %QTPath%\rcc.exe %%a -name %%a  -no-compress -o %GenQRCPath%\qrc_%%~na.cpp
	%QTPath%\rcc.exe %%a -name %%a  -no-compress -o %GenQRCPath%\qrc_%%~na.cpp
)

:mocBlastPluginCPP
echo %QTPath%\moc.exe "%PROJECT_ROOT%\source\BlastPlugin\BlastPlugin.h" -o %GenMocPath%\moc_BlastPlugin.cpp  "-I./../../source/CoreLib"
%QTPath%\moc.exe "%PROJECT_ROOT%\source\BlastPlugin\BlastPlugin.h" -o %GenMocPath%\moc_BlastPlugin.cpp  "-I./../../source/CoreLib" 
goto end

:end