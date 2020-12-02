@echo off
rem Downloads and build sources for libraries we depend upon

goto Run
:Usage
echo.%~nx0 [flags and arguments]
echo.
echo.Download and build OpenSSL. This should only be performed in order to
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

call "%PCBUILD%\find_python.bat" "%PYTHON%"
if ERRORLEVEL 1 (echo Cannot locate python.exe on PATH or as PYTHON variable & exit /b 3)

call "%PCBUILD%\get_externals.bat" --openssl-src --no-openssl %ORG_SETTING%

if "%PERL%" == "" where perl > "%TEMP%\perl.loc" 2> nul && set /P PERL= <"%TEMP%\perl.loc" & del "%TEMP%\perl.loc"
if "%PERL%" == "" (echo Cannot locate perl.exe on PATH or as PERL variable & exit /b 4)

%MSBUILD% "%PCBUILD%\openssl.vcxproj" /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 exit /b
%MSBUILD% "%PCBUILD%\openssl.vcxproj" /p:Configuration=Release /p:Platform=x64
if errorlevel 1 exit /b
%MSBUILD% "%PCBUILD%\openssl.vcxproj" /p:Configuration=Release /p:Platform=ARM
if errorlevel 1 exit /b
%MSBUILD% "%PCBUILD%\openssl.vcxproj" /p:Configuration=Release /p:Platform=ARM64
if errorlevel 1 exit /b

