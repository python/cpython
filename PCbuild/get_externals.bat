@echo off
setlocal
rem Simple script to fetch source for external libraries

if not exist "%~dp0..\externals" mkdir "%~dp0..\externals"
pushd "%~dp0..\externals"

if "%SVNROOT%"=="" set SVNROOT=http://svn.python.org/projects/external/

rem Optionally clean up first.  Be warned that this can be very destructive!
if not "%1"=="" (
    for %%c in (-c --clean --clean-only) do (
        if "%1"=="%%c" goto clean
    )
    goto usage
)
goto fetch

:clean
echo.Cleaning up external libraries.
for /D %%d in (
               bzip2-*
               db-*
               nasm-*
               openssl-*
               tcl-*
               tcltk*
               tk-*
               tix-*
               sqlite-*
               xz-*
               ) do (
    echo.Removing %%d
    rmdir /s /q %%d
)
if "%1"=="--clean-only" (
    goto end
)

:fetch
rem Fetch current versions

svn --version > nul 2>&1
if ERRORLEVEL 9009 (
    echo.svn.exe must be on your PATH.
    echo.Try TortoiseSVN (http://tortoisesvn.net/^) and be sure to check the
    echo.command line tools option.
    popd
    exit /b 1
)

echo.Fetching external libraries...

rem When updating these versions, remember to update the relevant property
rem files in both this dir and PC\VS9.0

set libraries=
set libraries=%libraries%                                    bzip2-1.0.6
if NOT "%IncludeBsddb%"=="false" set libraries=%libraries%   db-4.7.25.0
if NOT "%IncludeSSL%"=="false" set libraries=%libraries%     nasm-2.11.06
if NOT "%IncludeSSL%"=="false" set libraries=%libraries%     openssl-1.0.2g
set libraries=%libraries%                                    sqlite-3.6.21
if NOT "%IncludeTkinter%"=="false" set libraries=%libraries% tcl-8.5.15.0
if NOT "%IncludeTkinter%"=="false" set libraries=%libraries% tk-8.5.15.0
if NOT "%IncludeTkinter%"=="false" set libraries=%libraries% tix-8.4.3.5

for %%e in (%libraries%) do (
    if exist %%e (
        echo.%%e already exists, skipping.
    ) else (
        echo.Fetching %%e...
        svn export %SVNROOT%%%e
    )
)

goto end

:usage
echo.invalid argument: %1
echo.usage: %~n0 [[ -c ^| --clean ] ^| --clean-only ]
echo.
echo.Pull all sources necessary for compiling optional extension modules
echo.that rely on external libraries.  Requires svn.exe to be on your PATH
echo.and pulls sources from %SVNROOT%.
echo.
echo.Use the -c or --clean option to clean up all external library sources
echo.before pulling in the current versions.
echo.
echo.Use the --clean-only option to do the same cleaning, without pulling in
echo.anything new.
echo.
echo.Only the first argument is checked, all others are ignored.
echo.
echo.**WARNING**: the cleaning options unconditionally remove any directory
echo.that is a child of
echo.   %CD%
echo.and matches wildcard patterns beginning with bzip2-, db-, nasm-, openssl-,
echo.tcl-, tcltk, tk-, tix-, sqlite-, or xz-, and as such has the potential
echo.to be very destructive if you are not aware of what it is doing.  Use with
echo.caution!
popd
exit /b -1


:end
echo Finished.
popd
