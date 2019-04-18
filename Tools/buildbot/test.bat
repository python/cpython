@echo off
rem Used by the buildbot "test" step.
setlocal

set here=%~dp0
set rt_opts=-q -d
set regrtest_args=-j1
set arm32_ssh=

:CheckOpts
if "%1"=="-x64" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-arm32" (set arm32_ssh=true) & shift & goto CheckOpts
if "%1"=="-d" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-O" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-q" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="+d" (set rt_opts=%rt_opts:-d=%) & shift & goto CheckOpts
if "%1"=="+q" (set rt_opts=%rt_opts:-q=%) & shift & goto CheckOpts
if NOT "%1"=="" (set regrtest_args=%regrtest_args% %1) & shift & goto CheckOpts

echo on
set rt_args=%rt_opts% -uall -rwW --slowest --timeout=1200 --fail-env-changed %regrtest_args%
if "%arm32_ssh%"=="true" goto :Arm32Ssh

call "%here%..\..\PCbuild\rt.bat" %rt_args%
exit /b 0

:Arm32Ssh
if "%SSH_SERVER%"=="" (echo SSH_SERVER environment variable must be set & exit 127)
if "%REMOTE_PYTHON_DIR%"=="" (set REMOTE_PYTHON_DIR=c:\python)
if "%PYTHON_SOURCE%"=="" (set PYTHON_SOURCE=%here..\..\)
ssh %SSH_SERVER% "if EXIST %REMOTE_PYTHON_DIR% (rd %REMOTE_PYTHON_DIR% /s/q)"
scp "%PYTHON_SOURCE%" "%SSH_SERVER%:%REMOTE_PYTHON_DIR%"
ssh %SSH_SERVER% call "%REMOTE_PYTHON_DIR%\PCbuild\rt.bat" %rt_args%
exit /b 0
