@echo off
setlocal
set D=%~dp0
set PCBUILD=%D%..\..\PCBuild\

set BUILDX86=
set BUILDX64=
set REBUILD=
set OUTPUT=
set PACKAGES=

:CheckOpts
if "%~1" EQU "-h" goto Help
if "%~1" EQU "-x86" (set BUILDX86=1) && shift && goto CheckOpts
if "%~1" EQU "-x64" (set BUILDX64=1) && shift && goto CheckOpts
if "%~1" EQU "-r" (set REBUILD=-r) && shift && goto CheckOpts
if "%~1" EQU "-o" (set OUTPUT="/p:OutputPath=%~2") && shift && shift && goto CheckOpts
if "%~1" EQU "--out" (set OUTPUT="/p:OutputPath=%~2") && shift && shift && goto CheckOpts
if "%~1" EQU "-p" (set PACKAGES=%PACKAGES% %~2) && shift && shift && goto CheckOpts

if not defined BUILDX86 if not defined BUILDX64 (set BUILDX86=1) && (set BUILDX64=1)

if not defined NUGET where nuget -q || echo Cannot find nuget.exe on PATH and NUGET is not set. && exit /B 1
call "%PCBUILD%find_msbuild.bat" %MSBUILD%
if ERRORLEVEL 1 (echo Cannot locate MSBuild.exe on PATH or as MSBUILD variable & exit /b 2)
if not defined PYTHON set PYTHON=py -3

@%PYTHON% -c "" >nul 2>nul
@if errorlevel 1 (
    %NUGET% install python -OutputDirectory "%D%obj" -ExcludeVersion -NonInteractive
    set PYTHON="%D%obj\python\tools\python.exe"
)


if defined PACKAGES set PACKAGES="/p:Packages=%PACKAGES%"

if defined BUILDX86 (
    if defined REBUILD ( call "%PCBUILD%build.bat" -e -r
    ) else if not exist "%PCBUILD%python.exe" call "%PCBUILD%build.bat" -e
    if errorlevel 1 goto :eof

    %MSBUILD% "%D%make_pkg.proj" /p:Configuration=Release /p:Platform=x86 %OUTPUT% %PACKAGES%
    if errorlevel 1 goto :eof
)

if defined BUILDX64 (
    if defined REBUILD ( call "%PCBUILD%build.bat" -p x64 -e -r
    ) else if not exist "%PCBUILD%amd64\python.exe" call "%PCBUILD%build.bat" -p x64 -e
    if errorlevel 1 goto :eof

    %MSBUILD% "%D%make_pkg.proj" /p:Configuration=Release /p:Platform=x64 %OUTPUT% %PACKAGES%
    if errorlevel 1 goto :eof
)

exit /B 0

:Help
echo build.bat [-x86] [-x64] [--out DIR] [-r] [-h]
echo.
echo    -x86                Build x86 installers
echo    -x64                Build x64 installers
echo    -r                  Rebuild rather than incremental build
echo    --out [DIR]         Override output directory
echo    -h                  Show usage
