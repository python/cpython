@echo off
setlocal
rem Downloads and build sources for libraries we depend upon

if "%PCBUILD%"=="" (set PCBUILD=%~dp0)
if "%EXTERNALS_DIR%"=="" (set EXTERNALS_DIR=%PCBUILD%\..\externals)

set CERT_SETTING=
set ORG_SETTING=

:CheckOpts
if "%~1"=="--certificate" (set CERT_SETTING=/p:SigningCertificate="%~2") && shift && shift & goto CheckOpts
if "%~1"=="-c" (set CERT_SETTING=/p:SigningCertificate="%~2") && shift && shift & goto CheckOpts
if "%~1"=="--organization" (set ORG_SETTING=--organization "%~2") && shift && shift && goto CheckOpts

if "%~1"=="" goto Build
echo Unrecognized option: %1
exit /B 1

:Build
call "%PCBUILD%find_msbuild.bat" %MSBUILD%
if ERRORLEVEL 1 (echo Cannot locate MSBuild.exe on PATH or as MSBUILD variable & exit /b 2)

rem call "%PCBUILD%find_python.bat" "%PYTHON%"
rem if ERRORLEVEL 1 (echo Cannot locate python.exe on PATH or as PYTHON variable & exit /b 3)

call "%PCBUILD%get_externals.bat" --tkinter %ORG_SETTING%

%MSBUILD% "%PCBUILD%tcl.vcxproj" /p:Configuration=Release /p:Platform=Win32
%MSBUILD% "%PCBUILD%tk.vcxproj" /p:Configuration=Release /p:Platform=Win32
%MSBUILD% "%PCBUILD%tix.vcxproj" /p:Configuration=Release /p:Platform=Win32

%MSBUILD% "%PCBUILD%tcl.vcxproj" /p:Configuration=Release /p:Platform=x64
%MSBUILD% "%PCBUILD%tk.vcxproj" /p:Configuration=Release /p:Platform=x64
%MSBUILD% "%PCBUILD%tix.vcxproj" /p:Configuration=Release /p:Platform=x64
