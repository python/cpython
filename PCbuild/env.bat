@echo off
set VS9=%ProgramFiles%\Microsoft Visual Studio 9.0
echo Build environments: x86, ia64, amd64, x86_amd64, x86_ia64
echo.
call "%VS9%\VC\vcvarsall.bat" %1
