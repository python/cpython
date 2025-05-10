@echo off
rem Run Tests.  Run the regression test suite.
rem Usage:  rt [-d] [-O] [-q] [-x64] regrtest_args
rem -d   Run Debug build (python_d.exe).  Else release build.
rem -O   Run python.exe or python_d.exe (see -d) with -O.
rem -q   "quick" -- normally the tests are run twice, the first time
rem      after deleting all the .pyc files reachable from Lib/.
rem      -q runs the tests just once, and without deleting .pyc files.
rem -p <Win32|x64|ARM|ARM64> or -win32, -x64, -arm32, -arm64
rem      Run the specified architecture of python (or python_d if -d
rem      was specified). If omitted, uses %PREFIX% if set or 64-bit.
rem --disable-gil Run free-threaded build.
rem All leading instances of these switches are shifted off, and
rem whatever remains (up to 9 arguments) is passed to regrtest.py.
rem For example,
rem     rt -O -d -x test_thread
rem runs
rem     python_d -O ../lib/test/regrtest.py -x test_thread
rem twice, and
rem     rt -q -g test_binascii
rem runs
rem     python_d ../lib/test/regrtest.py -g test_binascii
rem to generate the expected-output file for binascii quickly.
rem
rem Confusing:  if you want to pass a comma-separated list, like
rem     -u network,largefile
rem then you have to quote it on the rt line, like
rem     rt -u "network,largefile"

setlocal

set pcbuild=%~dp0
set pyname=python
set suffix=
set qmode=
set dashO=
set regrtestargs=--fast-ci
set exe=

:CheckOpts
if "%~1"=="-O" (set dashO=-O)     & shift & goto CheckOpts
if "%~1"=="-q" (set qmode=yes)    & shift & goto CheckOpts
if "%~1"=="-d" (set suffix=_d)    & shift & goto CheckOpts
rem HACK: Need some way to infer the version number in this script
if "%~1"=="--disable-gil" (set pyname=python3.15t) & shift & goto CheckOpts
if "%~1"=="-win32" (set prefix=%pcbuild%win32) & shift & goto CheckOpts
if "%~1"=="-x64" (set prefix=%pcbuild%amd64) & shift & goto CheckOpts
if "%~1"=="-amd64" (set prefix=%pcbuild%amd64) & shift & goto CheckOpts
if "%~1"=="-arm64" (set prefix=%pcbuild%arm64) & shift & goto CheckOpts
if "%~1"=="-arm32" (set prefix=%pcbuild%arm32) & shift & goto CheckOpts
if "%~1"=="-p" (call :SetPlatform %~2) & shift & shift & goto CheckOpts
if NOT "%~1"=="" (set regrtestargs=%regrtestargs% %~1) & shift & goto CheckOpts

if not defined prefix set prefix=%pcbuild%amd64
set exe=%prefix%\%pyname%%suffix%.exe
set cmd="%exe%" %dashO% -m test %regrtestargs%
if defined qmode goto Qmode

echo Deleting .pyc files ...
"%exe%" "%pcbuild%rmpyc.py"

echo Cleaning _pth files ...
if exist %prefix%\*._pth del %prefix%\*._pth

echo on
%cmd%
@echo off

echo About to run again without deleting .pyc first:
pause
goto Qmode

:SetPlatform
if /I %1 EQU Win32 (set prefix=%pcbuild%win32) & exit /B 0
if /I %1 EQU x64 (set prefix=%pcbuild%amd64) & exit /B 0
if /I %1 EQU ARM64 (set prefix=%pcbuild%arm64) & exit /B 0
if /I %1 EQU ARM (set prefix=%pcbuild%arm32) & exit /B 0
echo Invalid platform "%1"
exit /B 1

:Qmode
echo on
%cmd%
