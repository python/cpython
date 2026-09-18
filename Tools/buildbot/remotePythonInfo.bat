@echo off
rem Used by the buildbot "remotedeploy" step.
setlocal

set PATH=%PATH%;%SystemRoot%\SysNative\OpenSSH;%SystemRoot%\System32\OpenSSH
set here=%~dp0
set arm32_ssh=
set suffix=_d
if "%REMOTE_PYTHON_DIR%"=="" (set REMOTE_PYTHON_DIR=C:\python\)
if NOT "%REMOTE_PYTHON_DIR:~-1,1%"=="\" (set REMOTE_PYTHON_DIR=%REMOTE_PYTHON_DIR%\)

:CheckOpts
if "%1"=="-arm32" (set arm32_ssh=true) & (set prefix=%REMOTE_PYTHON_DIR%pcbuild\arm32) & shift & goto CheckOpts
if "%1"=="-d" (set suffix=_d) & shift & goto CheckOpts
if "%1"=="+d" (set suffix=) & shift & goto CheckOpts
if NOT "%1"=="" (echo unrecognized option %1) & goto Arm32SshHelp

if "%arm32_ssh%"=="true" goto :Arm32Ssh

:Arm32Ssh
if "%SSH_SERVER%"=="" goto :Arm32SshHelp

set PYTHON_EXE=%prefix%\python%suffix%.exe
echo on
ssh %SSH_SERVER% %PYTHON_EXE% -m test.pythoninfo
exit /b %ERRORLEVEL%

:Arm32SshHelp
echo SSH_SERVER environment variable must be set to administrator@[ip address]
echo where [ip address] is the address of a Windows IoT Core ARM32 device.
echo.
echo The test worker should have the SSH agent running.
echo Also a key must be created with ssh-keygen and added to both the buildbot worker machine
echo and the ARM32 worker device: see https://docs.microsoft.com/en-us/windows/iot-core/connect-your-device/ssh
exit /b 127
