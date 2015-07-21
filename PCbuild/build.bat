@echo off
rem A batch program to build or rebuild a particular configuration,
rem just for convenience.

rem Arguments:
rem  -c  Set the configuration (default: Release)
rem  -p  Set the platform (x64 or Win32, default: Win32)
rem  -r  Target Rebuild instead of Build
rem  -t  Set the target manually (Build, Rebuild, Clean, or CleanAll)
rem  -d  Set the configuration to Debug
rem  -e  Pull in external libraries using get_externals.bat
rem  -m  Enable parallel build (enabled by default)
rem  -M  Disable parallel build
rem  -v  Increased output messages
rem  -k  Attempt to kill any running Pythons before building (usually unnecessary)

setlocal
set platf=Win32
set vs_platf=x86
set conf=Release
set target=Build
set dir=%~dp0
set parallel=/m
set verbose=/nologo /v:m
set kill=

:CheckOpts
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
if "%~1"=="-V" shift & goto Version

if "%platf%"=="x64" (set vs_platf=x86_amd64)

rem Setup the environment
call "%dir%env.bat" %vs_platf% >nul

if "%kill%"=="true" (
    msbuild /v:m /nologo /target:KillPython "%pcbuild%\pythoncore.vcxproj" /p:Configuration=%conf% /p:Platform=%platf% /p:KillPython=true
)

rem Call on MSBuild to do the work, echo the command.
rem Passing %1-9 is not the preferred option, but argument parsing in
rem batch is, shall we say, "lackluster"
echo on
msbuild "%dir%pcbuild.proj" /t:%target% %parallel% %verbose% /p:Configuration=%conf% /p:Platform=%platf% %1 %2 %3 %4 %5 %6 %7 %8 %9

@goto :eof

:Version
rem Display the current build version information
msbuild "%dir%python.props" /t:ShowVersionInfo /v:m /nologo %1 %2 %3 %4 %5 %6 %7 %8 %9
