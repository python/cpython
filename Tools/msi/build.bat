@echo off
setlocal
set D=%~dp0
set PCBUILD=%D%..\..\PCBuild\

set BUILDX86=
set BUILDX64=
set BUILDDOC=
set BUILDPX=
set BUILDPACK=

:CheckOpts
if "%~1" EQU "-h" goto Help
if "%~1" EQU "-x86" (set BUILDX86=1) && shift && goto CheckOpts
if "%~1" EQU "-x64" (set BUILDX64=1) && shift && goto CheckOpts
if "%~1" EQU "--doc" (set BUILDDOC=1) && shift && goto CheckOpts
if "%~1" EQU "--test-marker" (set BUILDPX=1) && shift && goto CheckOpts
if "%~1" EQU "--pack" (set BUILDPACK=1) && shift && goto CheckOpts

if not defined BUILDX86 if not defined BUILDX64 (set BUILDX86=1) && (set BUILDX64=1)

call "%D%get_externals.bat"

call "%PCBUILD%env.bat" x86

if defined BUILDX86 (
    call "%PCBUILD%build.bat" -d -e
    if errorlevel 1 goto :eof
    call "%PCBUILD%build.bat" -e
    if errorlevel 1 goto :eof
)
if defined BUILDX64 (
    call "%PCBUILD%build.bat" -p x64 -d -e
    if errorlevel 1 goto :eof
    call "%PCBUILD%build.bat" -p x64 -e
    if errorlevel 1 goto :eof
)

if defined BUILDDOC (
    call "%PCBUILD%..\Doc\make.bat" htmlhelp
    if errorlevel 1 goto :eof
)

set BUILD_CMD="%D%bundle\snapshot.wixproj"
if defined BUILDPX (
    set BUILD_CMD=%BUILD_CMD% /p:UseTestMarker=true
)
if defined BUILDPACK (
    set BUILD_CMD=%BUILD_CMD% /p:Pack=true
)

if defined BUILDX86 (
    msbuild %BUILD_CMD%
    if errorlevel 1 goto :eof
)
if defined BUILDX64 (
    msbuild /p:Platform=x64 %BUILD_CMD%
    if errorlevel 1 goto :eof
)

exit /B 0

:Help
echo build.bat [-x86] [-x64] [--doc] [-h] [--test-marker] [--pack]
echo.
echo    -x86                Build x86 installers
echo    -x64                Build x64 installers
echo    --doc               Build CHM documentation
echo    --test-marker       Build installers with 'x' markers
echo    --pack              Embed core MSIs into installer
