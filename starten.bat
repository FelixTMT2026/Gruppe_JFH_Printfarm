@echo off
rem Startet die PrintFarm-Anwendung mit korrektem Qt-PATH (Desktop Qt 6.11.x MinGW 64-bit).
set "QT_DIR=C:\Qt\6.11.1\mingw_64"
set "PATH=%QT_DIR%\bin;%PATH%"
cd /d "%~dp0"
start "" "build\Gruppe_JFH_Printfarm.exe"
