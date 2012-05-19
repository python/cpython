@echo off
set VS10=%ProgramFiles(x86)%\Microsoft Visual Studio 10.0
IF EXIST "%VS10%" GOTO ok
set VS10=%ProgramFiles%\Microsoft Visual Studio 10.0
:ok

echo Build environments: x86, ia64, amd64, x86_amd64, x86_ia64
echo.
call "%VS10%\VC\vcvarsall.bat" %1
