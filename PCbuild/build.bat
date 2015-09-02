@echo off
goto Run
:Usage
echo.%~nx0 [flags and arguments] [quoted MSBuild options]
echo.
echo.Build CPython from the command line.  Requires the appropriate
echo.version(s) of Microsoft Visual Studio to be installed (see readme.txt).
echo.Also requires Subversion (svn.exe) to be on PATH if the '-e' flag is
echo.given.
echo.
echo.After the flags recognized by this script, up to 9 arguments to be passed
echo.directly to MSBuild may be passed.  If the argument contains an '=', the
echo.entire argument must be quoted (e.g. `%~nx0 "/p:PlatformToolset=v100"`)
echo.
echo.Available flags:
echo.  -h  Display this help message
echo.  -r  Target Rebuild instead of Build
echo.  -d  Set the configuration to Debug
echo.  -e  Build external libraries fetched by get_externals.bat
echo.  -m  Enable parallel build
echo.  -M  Disable parallel build (disabled by default)
echo.  -v  Increased output messages
echo.  -k  Attempt to kill any running Pythons before building (usually done
echo.      automatically by the pythoncore project)
echo.
echo.Available arguments:
echo.  -c Release ^| Debug ^| PGInstrument ^| PGUpdate
echo.     Set the configuration (default: Release)
echo.  -p x64 ^| Win32
echo.     Set the platform (default: Win32)
echo.  -t Build ^| Rebuild ^| Clean ^| CleanAll
echo.     Set the target manually
exit /b 127

:Run
setlocal
set platf=Win32
set vs_platf=x86
set conf=Release
set target=Build
set dir=%~dp0
set parallel=
set verbose=/nologo /v:m
set kill=

:CheckOpts
if "%~1"=="-h" goto Usage
if "%~1"=="-c" (set conf=%2) & shift & shift & goto CheckOpts
if "%~1"=="-p" (set platf=%2) & shift & shift & goto CheckOpts
if "%~1"=="-r" (set target=Rebuild) & shift & goto CheckOpts
if "%~1"=="-t" (set target=%2) & shift & shift & goto CheckOpts
if "%~1"=="-d" (set conf=Debug) & shift & goto CheckOpts
if "%~1"=="-e" call "%dir%get_externals.bat" & shift & goto CheckOpts
if "%~1"=="-m" (set parallel=/m) & shift & goto CheckOpts
if "%~1"=="-M" (set parallel=) & shift & goto CheckOpts
if "%~1"=="-v" (set verbose=/v:n) & shift & goto CheckOpts
if "%~1"=="-k" (set kill=true) & shift & goto CheckOpts

if "%platf%"=="x64" (set vs_platf=x86_amd64)

rem Setup the environment
call "%dir%env.bat" %vs_platf% >nul

if "%kill%"=="true" (
    msbuild /v:m /nologo /target:KillPython "%dir%\pythoncore.vcxproj" /p:Configuration=%conf% /p:Platform=%platf% /p:KillPython=true
)

rem Call on MSBuild to do the work, echo the command.
rem Passing %1-9 is not the preferred option, but argument parsing in
rem batch is, shall we say, "lackluster"
echo on
msbuild "%dir%pcbuild.proj" /t:%target% %parallel% %verbose% /p:Configuration=%conf% /p:Platform=%platf% %1 %2 %3 %4 %5 %6 %7 %8 %9
