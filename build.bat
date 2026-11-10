@echo off
rem Baut das Projekt per Doppelklick: legt bei Bedarf den build-Ordner an
rem (Ziel der Verknuepfung Gruppe_JFH_Printfarm.lnk) und kompiliert danach.
rem Qt 6.11.x MinGW 64-bit, wie im Projekt konfiguriert.
set "PROJEKT_DIR=%~dp0"
set "QT_PREFIX=C:/Qt/6.11.1/mingw_64"
set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
set "NINJA_EXE=C:\Qt\Tools\Ninja\ninja.exe"
set "GXX_EXE=C:\Qt\Tools\mingw1310_64\bin\g++.exe"

cd /d "%PROJEKT_DIR%"

if not exist "build\CMakeCache.txt" (
    echo Konfiguriere build-Ordner ...
    "%CMAKE_EXE%" -B build -G Ninja ^
        -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%" ^
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

echo Fertig: build\Gruppe_JFH_Printfarm.exe
pause
