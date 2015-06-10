@echo off
echo Build environments: x86, ia64, amd64, x86_amd64, x86_ia64
echo.
call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" %*
