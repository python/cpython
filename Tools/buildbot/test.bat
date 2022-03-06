@echo off
rem Used by the buildbot "test" step.
setlocal

set PATH=%PATH%;%SystemRoot%\SysNative\OpenSSH;%SystemRoot%\System32\OpenSSH
set here=%~dp0
set rt_opts=-q -d
set regrtest_args=-j1
set arm32_ssh=

:CheckOpts
if "%1"=="-x64" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-arm64" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-arm32" (set rt_opts=%rt_opts% %1) & (set arm32_ssh=true) & shift & goto CheckOpts
if "%1"=="-d" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-O" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="-q" (set rt_opts=%rt_opts% %1) & shift & goto CheckOpts
if "%1"=="+d" (set rt_opts=%rt_opts:-d=%) & shift & goto CheckOpts
if "%1"=="+q" (set rt_opts=%rt_opts:-q=%) & shift & goto CheckOpts
if NOT "%1"=="" (set regrtest_args=%regrtest_args% %1) & shift & goto CheckOpts

if "%PROCESSOR_ARCHITECTURE%"=="ARM" if "%arm32_ssh%"=="true" goto NativeExecution
if "%arm32_ssh%"=="true" goto :Arm32Ssh

:NativeExecution
call "%here%..\..\PCbuild\rt.bat" %rt_opts% -uall -rwW --slowest --timeout=1200 --fail-env-changed %regrtest_args%
exit /b %ERRORLEVEL%

:Arm32Ssh
set dashU=-unetwork -udecimal -usubprocess -uurlfetch -utzdata
if "%SSH_SERVER%"=="" goto :Arm32SshHelp
if "%PYTHON_SOURCE%"=="" (set PYTHON_SOURCE=%here%..\..\)
if "%REMOTE_PYTHON_DIR%"=="" (set REMOTE_PYTHON_DIR=C:\python\)
if NOT "%REMOTE_PYTHON_DIR:~-1,1%"=="\" (set REMOTE_PYTHON_DIR=%REMOTE_PYTHON_DIR%\)

set TEMP_ARGS=--temp %REMOTE_PYTHON_DIR%temp

set rt_args=%rt_opts% %dashU% -rwW --slowest --timeout=1200 --fail-env-changed %regrtest_args% %TEMP_ARGS%
ssh %SSH_SERVER% "set TEMP=%REMOTE_PYTHON_DIR%temp& %REMOTE_PYTHON_DIR%PCbuild\rt.bat" %rt_args%
exit /b %ERRORLEVEL%

:Arm32SshHelp
echo SSH_SERVER environment variable must be set to administrator@[ip address]
echo where [ip address] is the address of a Windows IoT Core ARM32 device.
echo.
echo The test worker should have the SSH agent running.
echo Also a key must be created with ssh-keygen and added to both the buildbot worker machine
echo and the ARM32 worker device: see https://docs.microsoft.com/en-us/windows/iot-core/connect-your-device/ssh
exit /b 127
