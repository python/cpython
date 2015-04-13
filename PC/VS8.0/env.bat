@echo off
set VS8=%ProgramFiles%\Microsoft Visual Studio 8
echo Build environments: x86, ia64, amd64, x86_amd64, x86_ia64
echo.
call "%VS8%\VC\vcvarsall.bat" %1
