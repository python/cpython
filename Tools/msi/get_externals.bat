@echo off
setlocal
rem Simple script to fetch source for external tools

where /Q svn
if ERRORLEVEL 1 (
    echo.svn.exe must be on your PATH to get external tools.
    echo.Try TortoiseSVN (http://tortoisesvn.net/^) and be sure to check the
    echo.command line tools option.
    popd
    exit /b 1
)

if not exist "%~dp0..\..\externals" mkdir "%~dp0..\..\externals"
pushd "%~dp0..\..\externals"

if "%SVNROOT%"=="" set SVNROOT=http://svn.python.org/projects/external/

if not exist "windows-installer\.svn" (
    echo.Checking out installer dependencies to %CD%\windows-installer
    svn co %SVNROOT%windows-installer
) else (
    echo.Updating installer dependencies in %CD%\windows-installer
    svn up windows-installer
)

popd
