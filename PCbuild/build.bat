@echo off
rem A batch program to build or rebuild a particular configuration,
rem just for convenience.

rem Arguments:
rem  -c  Set the configuration (default: Release)
rem  -p  Set the platform (x64 or Win32, default: Win32)
rem  -r  Target Rebuild instead of Build
rem  -d  Set the configuration to Debug
rem  -e  Pull in external libraries using get_externals.bat

setlocal
set platf=Win32
set conf=Release
set target=Build
set dir=%~dp0

:CheckOpts
if "%1"=="-c" (set conf=%2) & shift & shift & goto CheckOpts
if "%1"=="-p" (set platf=%2) & shift & shift & goto CheckOpts
if "%1"=="-r" (set target=Rebuild) & shift & goto CheckOpts
if "%1"=="-d" (set conf=Debug) & shift & goto CheckOpts
if "%1"=="-e" call "%dir%get_externals.bat" & shift & goto CheckOpts

if "%platf%"=="x64" (set vs_platf=x86_amd64)

rem Setup the environment
call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" %vs_platf%

rem Call on MSBuild to do the work, echo the command.
rem Passing %1-9 is not the preferred option, but argument parsing in
rem batch is, shall we say, "lackluster"
echo on
msbuild "%dir%pcbuild.sln" /t:%target% /p:Configuration=%conf% /p:Platform=%platf% %1 %2 %3 %4 %5 %6 %7 %8 %9
