@echo off
rem This script adds the latest available tools to the path for the current
rem command window. However, most builds of Python will ignore the version
rem of the tools on PATH and use PlatformToolset instead. Ideally, both sets of
rem tools should be the same version to avoid potential conflicts.
rem
rem To build Python with an earlier toolset, pass "/p:PlatformToolset=v100" (or
rem 'v110', 'v120' or 'v140') to the build script.

echo Build environments: x86, amd64, x86_amd64
echo.
set _ARGS=%*
if NOT DEFINED _ARGS set _ARGS=amd64

if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto :skip_vswhere
set VSTOOLS=
for /F "tokens=*" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -property installationPath -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64') DO @(set VSTOOLS=%%i\VC\Auxiliary\Build\vcvarsall.bat)
if not defined VSTOOLS goto :skip_vswhere
call "%VSTOOLS%" %_ARGS%
exit /B 0

:skip_vswhere
if not defined VSTOOLS set VSTOOLS=%VS140COMNTOOLS%
if not defined VSTOOLS set VSTOOLS=%VS120COMNTOOLS%
if not defined VSTOOLS set VSTOOLS=%VS110COMNTOOLS%
if not defined VSTOOLS set VSTOOLS=%VS100COMNTOOLS%
call "%VSTOOLS%..\..\VC\vcvarsall.bat" %_ARGS%
