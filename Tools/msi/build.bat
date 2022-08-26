@echo off
setlocal
set D=%~dp0
set PCBUILD=%D%..\..\PCbuild\

set BUILDX86=
set BUILDX64=
set BUILDARM64=
set BUILDDOC=
set BUILDTEST=
set BUILDPACK=
set REBUILD=

:CheckOpts
if    "%~1" EQU "-h" goto Help
if /I "%~1" EQU "-x86" (set BUILDX86=1) && shift && goto CheckOpts
if /I "%~1" EQU "-Win32" (set BUILDX86=1) && shift && goto CheckOpts
if /I "%~1" EQU "-x64" (set BUILDX64=1) && shift && goto CheckOpts
if /I "%~1" EQU "-arm64" (set BUILDARM64=1) && shift && goto CheckOpts
if    "%~1" EQU "--doc" (set BUILDDOC=1) && shift && goto CheckOpts
if    "%~1" EQU "--no-test-marker" (set BUILDTEST=) && shift && goto CheckOpts
if    "%~1" EQU "--test-marker" (set BUILDTEST=--test-marker) && shift && goto CheckOpts
if    "%~1" EQU "--pack" (set BUILDPACK=1) && shift && goto CheckOpts
if    "%~1" EQU "-r" (set REBUILD=-r) && shift && goto CheckOpts

if not defined BUILDX86 if not defined BUILDX64 if not defined BUILDARM64 (set BUILDX86=1) && (set BUILDX64=1)

call "%D%get_externals.bat"
call "%PCBUILD%find_msbuild.bat" %MSBUILD%
if ERRORLEVEL 1 (echo Cannot locate MSBuild.exe on PATH or as MSBUILD variable & exit /b 2)

if defined BUILDX86 (
    call "%PCBUILD%build.bat" -p Win32 -d -e %REBUILD% %BUILDTEST%
    if errorlevel 1 exit /B %ERRORLEVEL%
    call "%PCBUILD%build.bat" -p Win32 -e %REBUILD% %BUILDTEST%
    if errorlevel 1 exit /B %ERRORLEVEL%
)
if defined BUILDX64 (
    call "%PCBUILD%build.bat" -p x64 -d -e %REBUILD% %BUILDTEST%
    if errorlevel 1 exit /B %ERRORLEVEL%
    call "%PCBUILD%build.bat" -p x64 -e %REBUILD% %BUILDTEST%
    if errorlevel 1 exit /B %ERRORLEVEL%
)
if defined BUILDARM64 (
    call "%PCBUILD%build.bat" -p ARM64 -d -e %REBUILD% %BUILDTEST%
    if errorlevel 1 exit /B %ERRORLEVEL%
    call "%PCBUILD%build.bat" -p ARM64 -e %REBUILD% %BUILDTEST%
    if errorlevel 1 exit /B %ERRORLEVEL%
)

if defined BUILDDOC (
    call "%PCBUILD%..\Doc\make.bat" html
    if errorlevel 1 exit /B %ERRORLEVEL%
)

rem Build the launcher MSI separately
%MSBUILD% "%D%launcher\launcher.wixproj" /p:Platform=x86
if errorlevel 1 exit /B %ERRORLEVEL%

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
    %MSBUILD% /p:Platform=x86 %BUILD_CMD%
    if errorlevel 1 exit /B %ERRORLEVEL%
)
if defined BUILDX64 (
    %MSBUILD% /p:Platform=x64 %BUILD_CMD%
    if errorlevel 1 exit /B %ERRORLEVEL%
)
if defined BUILDARM64 (
    %MSBUILD% /p:Platform=ARM64 %BUILD_CMD%
    if errorlevel 1 exit /B %ERRORLEVEL%
)

exit /B 0

:Help
echo build.bat [-x86] [-x64] [-arm64] [--doc] [-h] [--test-marker] [--pack] [-r]
echo.
echo    -x86                Build x86 installers
echo    -x64                Build x64 installers
echo    -ARM64              Build ARM64 installers
echo    --doc               Build documentation
echo    --test-marker       Build with test markers
echo    --no-test-marker    Build without test markers (default)
echo    --pack              Embed core MSIs into installer
echo    -r                  Rebuild rather than incremental build
