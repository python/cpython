@echo off
setlocal
rem Downloads and build sources for libraries we depend upon

if "%PCBUILD%"=="" (set PCBUILD=%~dp0)
if "%EXTERNALS_DIR%"=="" (set EXTERNALS_DIR=%PCBUILD%\..\externals)

set ORG_SETTING=

:CheckOpts
if "%~1"=="--certificate" (set SigningCertificate=%~2) && shift && shift & goto CheckOpts
if "%~1"=="-c" (set SigningCertificate=%~2) && shift && shift & goto CheckOpts
if "%~1"=="--organization" (set ORG_SETTING=--organization "%~2") && shift && shift && goto CheckOpts

if "%~1"=="" goto Build
echo Unrecognized option: %1
exit /B 1

:Build
call "%PCBUILD%find_msbuild.bat" %MSBUILD%
if ERRORLEVEL 1 (echo Cannot locate MSBuild.exe on PATH or as MSBUILD variable & exit /b 2)

call "%PCBUILD%find_python.bat" "%PYTHON%"
if ERRORLEVEL 1 (echo Cannot locate python.exe on PATH or as PYTHON variable & exit /b 3)

call "%PCBUILD%get_externals.bat" --openssl %ORG_SETTING%

if "%PERL%" == "" where perl > "%TEMP%\perl.loc" 2> nul && set /P PERL= <"%TEMP%\perl.loc" & del "%TEMP%\perl.loc"
if "%PERL%" == "" (echo Cannot locate perl.exe on PATH & exit /b 4)

%MSBUILD% "%PCBUILD%openssl.vcxproj" /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 exit /b
%MSBUILD% "%PCBUILD%openssl.vcxproj" /p:Configuration=Release /p:Platform=x64
if errorlevel 1 exit /b
