@echo off

@echo ---start Moc UI---

setlocal enabledelayedexpansion 
set PROJECT_ROOT=..\..
SET SRC_ROOT=..\..\..\..
SET EXTERNAL_ROOT=%SRC_ROOT%\..\..\external
set QTPath=%EXTERNAL_ROOT%\Qt5.6.1\vc2015\x86\bin

set GenPath="%PROJECT_ROOT%\project\Generated\CurveEditor"
set GenMocPath=%GenPath%\moc
set GenUIPath=%GenPath%\ui
set GenQRCPath=%GenPath%\qrc

echo commands to run
echo rd /s /Q %GenPath%
echo mkdir %GenPath%
echo mkdir %GenMocPath%
echo mkdir %GenUIPath%
echo mkdir %GenQRCPath%

rd /s /Q %GenPath%
mkdir %GenPath%
mkdir %GenMocPath%
mkdir %GenUIPath%
mkdir %GenQRCPath%

:domocCurveEditor

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\Internal\Window\*.h"') do ( 
	findstr /i "Q_OBJECT" "%%a">nul&&( 
		echo %QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
		%QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
	)
)

echo %QTPath%\moc.exe "%PROJECT_ROOT%\source\CurveEditorMainWindow.h" -o %GenMocPath%\moc_CurveEditorMainWindow.cpp  
%QTPath%\moc.exe "%PROJECT_ROOT%\source\CurveEditorMainWindow.h" -o %GenMocPath%\moc_CurveEditorMainWindow.cpp 

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\Internal\UI\*.ui"') do ( 
	echo %QTPath%\uic.exe %%a -o %GenUIPath%\ui_%%~na.h
	%QTPath%\uic.exe %%a -o %GenUIPath%\ui_%%~na.h
)

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\Internal\UI\Resources\*.qrc"') do ( 	
	echo %QTPath%\rcc.exe -name "%%~na" -no-compress "%%a" -o %GenQRCPath%\qrc_%%~na.cpp
	%QTPath%\rcc.exe -name "%%~na" -no-compress "%%a" -o %GenQRCPath%\qrc_%%~na.cpp
)

@echo ---end Moc UI for CurveEditor---

@echo ---start Moc UI for CurveEditorTestApp---
set GenPath="%PROJECT_ROOT%\project\Generated\CurveEditorTestApp"
set GenMocPath=%GenPath%\moc
set GenUIPath=%GenPath%\ui
set GenQRCPath=%GenPath%\qrc

echo commands to run
echo rd /s /Q %GenPath%
echo mkdir %GenPath%
echo mkdir %GenMocPath%
echo mkdir %GenUIPath%
echo mkdir %GenQRCPath%

rd /s /Q %GenPath%
mkdir %GenPath%
mkdir %GenMocPath%
mkdir %GenUIPath%
mkdir %GenQRCPath%

:domocCurveEditorTestApp
for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\GraphEditorTestApp\*.h"') do ( 
	findstr /i "Q_OBJECT" "%%a">nul&&( 
		echo %QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
		%QTPath%\moc.exe %%a -o %GenMocPath%\moc_%%~na.cpp
	)
)

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\GraphEditorTestApp\*.ui"') do ( 
	echo %QTPath%\uic.exe %%a -o %GenUIPath%\ui_%%~na.h
	%QTPath%\uic.exe %%a -o %GenUIPath%\ui_%%~na.h
)

for /f "delims=" %%a in ('dir /a-d/b/s "%PROJECT_ROOT%\source\GraphEditorTestApp\*.qrc"') do ( 
	echo %QTPath%\rcc.exe -name "%%~na" -no-compress "%%a" -o %GenQRCPath%\qrc_%%~na.cpp
	%QTPath%\rcc.exe -name "%%~na" -no-compress "%%a" -o %GenQRCPath%\qrc_%%~na.cpp
)

@echo ---end Moc UI for CurveEditorTestApp---

goto end

:end