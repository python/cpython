@echo off
rem Downloads and build sources for libraries we depend upon

goto Run
:Usage
echo.%~nx0 [flags and arguments]
echo.
echo.Download and build Tcl/Tk. This should only be performed in order to
echo.update the binaries kept online - in most cases, the files downloaded
echo.by the get_externals.bat script are sufficient for building CPython.
echo.
echo.Available flags:
echo.  -h  Display this help message
echo.
echo.Available arguments:
echo.  --certificate (-c)   The signing certificate to use for binaries.
echo.  --organization       The github organization to obtain sources from.
echo.
exit /b 127

:Run
setlocal

if "%PCBUILD%"=="" (set PCBUILD=%~dp0)
if "%EXTERNALS_DIR%"=="" (set EXTERNALS_DIR=%PCBUILD%\..\externals)

set CERT_SETTING=
set ORG_SETTING=

:CheckOpts
if "%~1"=="-h" shift & goto Usage
if "%~1"=="--certificate" (set SigningCertificate=%~2) && shift && shift & goto CheckOpts
if "%~1"=="-c" (set SigningCertificate=%~2) && shift && shift & goto CheckOpts
if "%~1"=="--organization" (set ORG_SETTING=--organization "%~2") && shift && shift && goto CheckOpts

if "%~1"=="" goto Build
echo Unrecognized option: %1
goto Usage

:Build
call "%PCBUILD%\find_msbuild.bat" %MSBUILD%
if ERRORLEVEL 1 (echo Cannot locate MSBuild.exe on PATH or as MSBUILD variable & exit /b 2)

rem call "%PCBUILD%\find_python.bat" "%PYTHON%"
rem if ERRORLEVEL 1 (echo Cannot locate python.exe on PATH or as PYTHON variable & exit /b 3)

call "%PCBUILD%\get_externals.bat" --tkinter-src %ORG_SETTING%

%MSBUILD% "%PCBUILD%\tcl.vcxproj" /p:Configuration=Release /p:Platform=Win32
%MSBUILD% "%PCBUILD%\tk.vcxproj" /p:Configuration=Release /p:Platform=Win32

%MSBUILD% "%PCBUILD%\tcl.vcxproj" /p:Configuration=Release /p:Platform=x64
%MSBUILD% "%PCBUILD%\tk.vcxproj" /p:Configuration=Release /p:Platform=x64

%MSBUILD% "%PCBUILD%\tcl.vcxproj" /p:Configuration=Release /p:Platform=ARM64
%MSBUILD% "%PCBUILD%\tk.vcxproj" /p:Configuration=Release /p:Platform=ARM64
