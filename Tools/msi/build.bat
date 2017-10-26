@echo off
setlocal
set D=%~dp0
set PCBUILD=%D%..\..\PCbuild\

set BUILDX86=
set BUILDX64=
set BUILDDOC=
set BUILDTEST=--test-marker
set BUILDPACK=
set REBUILD=

:CheckOpts
if "%~1" EQU "-h" goto Help
if "%~1" EQU "-x86" (set BUILDX86=1) && shift && goto CheckOpts
if "%~1" EQU "-x64" (set BUILDX64=1) && shift && goto CheckOpts
if "%~1" EQU "--doc" (set BUILDDOC=1) && shift && goto CheckOpts
if "%~1" EQU "--no-test-marker" (set BUILDTEST=) && shift && goto CheckOpts
if "%~1" EQU "--pack" (set BUILDPACK=1) && shift && goto CheckOpts
if "%~1" EQU "-r" (set REBUILD=-r) && shift && goto CheckOpts

if not defined BUILDX86 if not defined BUILDX64 (set BUILDX86=1) && (set BUILDX64=1)

call "%D%get_externals.bat"
call "%PCBUILD%find_msbuild.bat" %MSBUILD%
if ERRORLEVEL 1 (echo Cannot locate MSBuild.exe on PATH or as MSBUILD variable & exit /b 2)

if defined BUILDX86 (
    call "%PCBUILD%build.bat" -d -e %REBUILD% %BUILDTEST%
    if errorlevel 1 goto :eof
    call "%PCBUILD%build.bat" -e %REBUILD% %BUILDTEST%
    if errorlevel 1 goto :eof
)
if defined BUILDX64 (
    call "%PCBUILD%build.bat" -p x64 -d -e %REBUILD% %BUILDTEST%
    if errorlevel 1 goto :eof
    call "%PCBUILD%build.bat" -p x64 -e %REBUILD% %BUILDTEST%
    if errorlevel 1 goto :eof
)

if defined BUILDDOC (
    call "%PCBUILD%..\Doc\make.bat" htmlhelp
    if errorlevel 1 goto :eof
)

rem Build the launcher MSI separately
%MSBUILD% "%D%launcher\launcher.wixproj" /p:Platform=x86

set BUILD_CMD="%D%bundle\snapshot.wixproj"
if defined BUILDTEST (
    set BUILD_CMD=%BUILD_CMD% /p:UseTestMarker=true
)
if defined BUILDPACK (
    set BUILD_CMD=%BUILD_CMD% /p:Pack=true
)
if defined REBUILD (
    set BUILD_CMD=%BUILD_CMD% /t:Rebuild
)

if defined BUILDX86 (
    %MSBUILD% %BUILD_CMD%
    if errorlevel 1 goto :eof
)
if defined BUILDX64 (
    %MSBUILD% /p:Platform=x64 %BUILD_CMD%
    if errorlevel 1 goto :eof
)

exit /B 0

:Help
echo build.bat [-x86] [-x64] [--doc] [-h] [--no-test-marker] [--pack] [-r]
echo.
echo    -x86                Build x86 installers
echo    -x64                Build x64 installers
echo    --doc               Build CHM documentation
echo    --no-test-marker    Build without test markers
echo    --pack              Embed core MSIs into installer
echo    -r                  Rebuild rather than incremental build
