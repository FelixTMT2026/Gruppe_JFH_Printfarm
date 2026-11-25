@echo off
setlocal EnableExtensions
rem ================================================================
rem PrintFarm_Gruppe_03 - portables Build-Skript (Windows)
rem Findet alle Werkzeuge automatisch, keine festen Pfade:
rem   Qt     : hoechste 6.x-Version mit mingw_64-Kit unter QT_HOME
rem            bzw. C:\Qt, D:\Qt, %SystemDrive%\Qt, %USERPROFILE%\Qt
rem   MinGW  : Qt-Tools\mingw* bzw. g++ aus dem PATH
rem            (ueberschreibbar: MINGW_DIR = ...\bin)
rem   CMake  : PATH bzw. Qt-Tools\CMake_64
rem   Ninja  : PATH bzw. Qt-Tools\Ninja; sonst MinGW-Makefiles
rem Baut danach in "build\PrintFarm_Gruppe_03.exe".
rem ================================================================
set "PROJEKT_DIR=%~dp0"
cd /d "%PROJEKT_DIR%"

rem ---------- Qt-Basisordner bestimmen ----------
set "QT_BASE="
if defined QT_HOME set "QT_BASE=%QT_HOME%"
if not defined QT_BASE if exist "C:\Qt" set "QT_BASE=C:\Qt"
if not defined QT_BASE if exist "D:\Qt" set "QT_BASE=D:\Qt"
if not defined QT_BASE if exist "%SystemDrive%\Qt" set "QT_BASE=%SystemDrive%\Qt"
if not defined QT_BASE if exist "%USERPROFILE%\Qt" set "QT_BASE=%USERPROFILE%\Qt"

rem ---------- Qt 6.x mit MinGW-Kit suchen (hoechste Version gewinnt) ----------
set "QT_PREFIX="
if defined QT_BASE (
    for /f "delims=" %%V in ('dir /b /ad /on "%QT_BASE%\6.*" 2^>nul') do (
        if exist "%QT_BASE%\%%V\mingw_64\bin\Qt6Core.dll" set "QT_PREFIX=%QT_BASE%\%%V\mingw_64"
    )
)

rem ---------- MinGW-Compiler suchen ----------
set "MINGW_BIN="
if defined MINGW_DIR set "MINGW_BIN=%MINGW_DIR%"
if not defined MINGW_BIN if defined QT_BASE (
    for /f "delims=" %%M in ('dir /b /ad /on "%QT_BASE%\Tools\mingw*" 2^>nul') do (
        if exist "%QT_BASE%\Tools\%%M\bin\g++.exe" set "MINGW_BIN=%QT_BASE%\Tools\%%M\bin"
    )
)
if not defined MINGW_BIN (
    for /f "delims=" %%I in ('where g++.exe 2^>nul') do if not defined MINGW_BIN set "MINGW_BIN=%%~dpI"
)

rem ---------- CMake suchen (Qt-Tools bevorzugt, Store-Stubs im PATH vermeiden) ----------
set "CMAKE_EXE="
if defined QT_BASE if exist "%QT_BASE%\Tools\CMake_64\bin\cmake.exe" set "CMAKE_EXE=%QT_BASE%\Tools\CMake_64\bin\cmake.exe"
if not defined CMAKE_EXE (
    for /f "delims=" %%I in ('where cmake.exe 2^>nul') do if not defined CMAKE_EXE set "CMAKE_EXE=%%~fI"
)

rem ---------- Ninja suchen (Qt-Tools bevorzugt; sonst mingw32-make als Ersatz) ----------
set "NINJA_EXE="
if defined QT_BASE if exist "%QT_BASE%\Tools\Ninja\ninja.exe" set "NINJA_EXE=%QT_BASE%\Tools\Ninja\ninja.exe"
if not defined NINJA_EXE (
    for /f "delims=" %%I in ('where ninja.exe 2^>nul') do if not defined NINJA_EXE set "NINJA_EXE=%%~fI"
)

set "GEN=Ninja"
set "MAKE_PROG="
if defined NINJA_EXE set "MAKE_PROG=%NINJA_EXE%"
if not defined MAKE_PROG if defined MINGW_BIN (
    if exist "%MINGW_BIN%\mingw32-make.exe" (
        set "GEN=MinGW Makefiles"
        set "MAKE_PROG=%MINGW_BIN%\mingw32-make.exe"
    )
)

rem ---------- Gefundene Werkzeuge ausgeben (Transparenz) ----------
echo Qt     : %QT_PREFIX%
echo MinGW  : %MINGW_BIN%
echo CMake  : %CMAKE_EXE%
echo Build  : %GEN% [%MAKE_PROG%]
echo.

rem ---------- Fehlerursachen klar melden ----------
if not defined QT_PREFIX (
    echo FEHLER: Qt 6.x mit MinGW-Kit nicht gefunden.
    echo Qt 6.11.x inkl. "MinGW 64-bit"-Kit installieren oder
    echo Umgebungsvariable QT_HOME auf den Qt-Basisordner setzen.
    pause
    exit /b 1
)
if not defined MINGW_BIN (
    echo FEHLER: Kein MinGW-Compiler ^(g++^) gefunden.
    echo Im Qt-Installer unter "Developer und Designer Tools" MinGW
    echo aktivieren oder MINGW_DIR auf das bin-Verzeichnis setzen.
    pause
    exit /b 1
)
if not defined CMAKE_EXE (
    echo FEHLER: CMake nicht gefunden ^(PATH oder Qt-Tools^).
    pause
    exit /b 1
)
if not defined MAKE_PROG (
    echo FEHLER: Weder Ninja noch mingw32-make gefunden.
    pause
    exit /b 1
)

rem ---------- MinGW/Qt ganz vorne in PATH (verhindert DLL-Konflikte) ----------
set "PATH=%MINGW_BIN%;%QT_PREFIX%\bin;%PATH%"
set "GXX_EXE=%MINGW_BIN%\g++.exe"

rem ---------- Konfigurieren (nur solange kein CMake-Cache existiert) ----------
if not exist "build\CMakeCache.txt" (
    echo Konfiguriere build-Ordner ...
    "%CMAKE_EXE%" -B build -G "%GEN%" ^
        -DCMAKE_MAKE_PROGRAM="%MAKE_PROG%" ^
        -DCMAKE_CXX_COMPILER="%GXX_EXE%" ^
        -DCMAKE_PREFIX_PATH="%QT_PREFIX%"
    if errorlevel 1 (
        echo FEHLER: CMake-Konfiguration fehlgeschlagen.
        pause
        exit /b 1
    )
)

echo Baue Projekt ...
"%CMAKE_EXE%" --build build
if errorlevel 1 (
    echo FEHLER: Build fehlgeschlagen.
    pause
    exit /b 1
)

rem ---------- Laufzeitdateien neben die EXE kopieren (Deployment) ----------
rem Ohne diesen Schritt startet die EXE per Doppelklick nicht
rem (Fehlermeldung ueber fehlende Qt- bzw. MinGW-DLLs).
if exist "build\PrintFarm_Gruppe_03.exe" (
    if exist "%QT_PREFIX%\bin\windeployqt.exe" (
        echo Lege Laufzeitdateien neben die EXE ...
        "%QT_PREFIX%\bin\windeployqt.exe" --release "build\PrintFarm_Gruppe_03.exe"
        if errorlevel 1 (
            echo FEHLER: Deployment fehlgeschlagen.
            pause
            exit /b 1
        )
    ) else (
        echo WARNUNG: windeployqt.exe nicht gefunden - EXE benoetigt Qt im PATH.
    )
    rem MinGW-Laufzeit-DLLs sicherstellen (falls windeployqt sie nicht kopiert)
    for %%D in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do (
        if exist "%MINGW_BIN%\%%D" if not exist "build\%%D" copy /y "%MINGW_BIN%\%%D" "build\" >nul
    )
)

rem ---------- Verknuepfung zur EXE im Hauptordner anlegen/erneuern ----------
rem "Ausfuehren in" = Projektordner, damit die App den Datenordner findet.
if exist "build\PrintFarm_Gruppe_03.exe" (
    set "PF_LNK=%PROJEKT_DIR%PrintFarm_Gruppe_03.lnk"
    set "PF_EXE=%PROJEKT_DIR%build\PrintFarm_Gruppe_03.exe"
    set "PF_DIR=%PROJEKT_DIR%"
    powershell -NoProfile -Command "$w = New-Object -ComObject WScript.Shell; $s = $w.CreateShortcut($env:PF_LNK); $s.TargetPath = $env:PF_EXE; $s.WorkingDirectory = $env:PF_DIR; $s.Description = 'PrintFarm_Gruppe_03 starten'; $s.Save()"
    if errorlevel 1 (
        echo Hinweis: Verknuepfung konnte nicht erstellt werden - PowerShell fehlt.
        echo Bitte build\PrintFarm_Gruppe_03.exe direkt starten.
    ) else (
        echo Verknuepfung erstellt: PrintFarm_Gruppe_03.lnk
    )
)

echo.
echo Fertig: build\PrintFarm_Gruppe_03.exe
echo Tests : cd build ^&^& ctest --output-on-failure
pause
