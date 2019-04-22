@echo off
rem Used by the buildbot "test" step.
setlocal

set here=%~dp0
set rt_opts=-q -d
set regrtest_args=-j1
set arm32_ssh=
set dparam=-d

:CheckOpts
if "%1"=="-x64" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-arm32" (set arm32_ssh=true) & shift & goto CheckOpts
if "%1"=="-d" (set rt_opts=%rt_opts% %1) & (set dparam=-d)& shift & goto CheckOpts
if "%1"=="-O" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-q" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="+d" (set rt_opts=%rt_opts:-d=%) & (dparam=) & shift & goto CheckOpts
if "%1"=="+q" (set rt_opts=%rt_opts:-q=%) & shift & goto CheckOpts
if NOT "%1"=="" (set regrtest_args=%regrtest_args% %1) & shift & goto CheckOpts

echo on
set rt_args=%rt_opts% -uall -rwW --slowest --timeout=1200 --fail-env-changed %regrtest_args%
if "%arm32_ssh%"=="true" goto :Arm32Ssh

call "%here%..\..\PCbuild\rt.bat" %rt_args%
exit /b 0

:Arm32Ssh
if "%SSH_SERVER%"=="" (echo SSH_SERVER environment variable must be set & exit /b 127)
if "%PYTHON_SOURCE%"=="" (set PYTHON_SOURCE=%here%..\..\)
if "%REMOTE_PYTHON_DIR%"=="" (set REMOTE_PYTHON_DIR=c:\python\)
if "%REMOTE_PYTHON_SHARE%"=="" (set REMOTE_PYTHON_SHARE=P:\python)
REM #ssh %SSH_SERVER% "if EXIST %REMOTE_PYTHON_DIR% (rd %REMOTE_PYTHON_DIR% /s/q)"
REM #scp "%PYTHON_SOURCE%python.bat" "%SSH_SERVER%:%REMOTE_PYTHON_DIR%python.bat"
REM #scp -r "%PYTHON_SOURCE%PCBuild" "%SSH_SERVER%:%REMOTE_PYTHON_DIR%PCBuild"
REM #scp -r "%PYTHON_SOURCE%Lib" "%SSH_SERVER%:%REMOTE_PYTHON_DIR%Lib"
if EXIST %REMOTE_PYTHON_SHARE% (rd %REMOTE_PYTHON_SHARE% /s/q)
python.exe PC/layout -vv %dparam% -s %PYTHON_SOURCE% -b "%PYTHON_SOURCE%\PCBuild\arm32" -t "%REMOTE_PYTHON_SHARE%\temp" --copy "%REMOTE_PYTHON_SHARE%" --preset-iot --include-tests --include-venv
if NOT EXIST %REMOTE_PYTHON_SHARE%\PCbuild (md %REMOTE_PYTHON_SHARE%\PCbuild)
copy %PYTHON_SOURCE%PCBuild\*.bat %REMOTE_PYTHON_SHARE%\PCbuild
copy %PYTHON_SOURCE%PCBuild\*.py %REMOTE_PYTHON_SHARE%\PCbuild
ssh %SSH_SERVER% call "%REMOTE_PYTHON_DIR%PCbuild\rt.bat" %rt_args%
exit /b 0
