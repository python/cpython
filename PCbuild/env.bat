@echo off
rem This script adds the latest available tools to the path for the current
rem command window. However, all builds of Python should ignore the version
rem of the tools on PATH and use PlatformToolset instead, which should
rem always be 'v90'.
rem
rem To build Python with a newer toolset, pass "/p:PlatformToolset=v100" (or
rem 'v110', 'v120' or 'v140') to the build script.  Note that no toolset
rem other than 'v90' is supported!

echo Build environments: x86, amd64, x86_amd64
echo.
set VSTOOLS=%VS140COMNTOOLS%
if "%VSTOOLS%"=="" set VSTOOLS=%VS120COMNTOOLS%
if "%VSTOOLS%"=="" set VSTOOLS=%VS110COMNTOOLS%
if "%VSTOOLS%"=="" set VSTOOLS=%VS100COMNTOOLS%
call "%VSTOOLS%..\..\VC\vcvarsall.bat" %*
