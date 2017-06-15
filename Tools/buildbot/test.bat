@echo off
rem Script to run the Python test suite used by the "test" step
rem of Windows buildbot slaves.
rem
rem See PCbuild/rt.bat for options, extra options:
rem  -t TIMEOUT: set a timeout in seconds

setlocal

set here=%~dp0
set rt_opts=-q -d
set regrtest_args=-j1

rem Use a timeout of 20 minutes per test file by default
set timeout=1200

:CheckOpts
if "%1"=="-x64" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-d" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-O" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-q" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="+d" (set rt_opts=%rt_opts:-d=%) & shift & goto CheckOpts
if "%1"=="+q" (set rt_opts=%rt_opts:-q=%) & shift & goto CheckOpts
if "%1"=="-t" (set timeout=%2) & shift & shift & goto CheckOpts
if NOT "%1"=="" (set regrtest_args=%regrtest_args% %1) & shift & goto CheckOpts

echo on
call "%here%..\..\PCbuild\rt.bat" %rt_opts% -uall -rwW --slowest --timeout=%timeout% %regrtest_args%
