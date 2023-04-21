@echo off
rem start idle
rem Usage:  idle [-d]
rem -d   Run Debug build (python_d.exe).  Else release build.

setlocal
set PCBUILD=%~dp0
set exedir=%PCBUILD%\win32
set exe=python
PATH %PATH%;..\externals\tcltk\bin

:CheckOpts
if "%1"=="-d" (set exe=%exe%_d) & shift & goto :CheckOpts
if "%1"=="-p" (call :SetExeDir %2) & shift & shift & goto :CheckOpts

set cmd=%exedir%\%exe% %PCBUILD%\..\Lib\idlelib\idle.py %1 %2 %3 %4 %5 %6 %7 %8 %9

echo on
%cmd%
exit /B %LASTERRORCODE%

:SetExeDir
if /I %1 EQU Win32 (set exedir=%PCBUILD%\win32)
if /I %1 EQU x64 (set exedir=%PCBUILD%\amd64)
if /I %1 EQU ARM (set exedir=%PCBUILD%\arm32)
if /I %1 EQU ARM64 (set exedir=%PCBUILD%\arm64)
exit /B 0
