@echo off
rem Used by the buildbot "test" step.
setlocal

set here=%~dp0
set rt_opts=-q -d
set regrtest_args=-j1

:CheckOpts
if "%1"=="-x64" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-d" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-O" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-q" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="+d" (set rt_opts=%rt_opts:-d=%) & shift & goto CheckOpts
if "%1"=="+q" (set rt_opts=%rt_opts:-q=%) & shift & goto CheckOpts
if NOT "%1"=="" (set regrtest_args=%regrtest_args% %1) & shift & goto CheckOpts

echo on
call "%here%..\..\PCbuild\rt.bat" %rt_opts% -uall -rwW --slowest --timeout=1200 %regrtest_args%
