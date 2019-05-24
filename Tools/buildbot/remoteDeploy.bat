@echo off
rem Used by the buildbot "remotedeploy" step.
setlocal

set here=%~dp0
set arm32_ssh=

:CheckOpts
if "%1"=="-arm32" (set arm32_ssh=true) & shift & goto CheckOpts
if NOT "%1"=="" (echo unrecognized option %1) & goto Arm32SshHelp

if "%arm32_ssh%"=="true" goto :Arm32Ssh

:Arm32Ssh
if "%SSH_SERVER%"=="" goto :Arm32SshHelp
if "%SSH%"=="" if EXIST %WINDIR%\System32\OpenSSH\ssh.exe (set SSH=%WINDIR%\System32\OpenSSH\ssh.exe)
if "%SCP%"=="" if EXIST %WINDIR%\System32\OpenSSH\scp.exe (set SCP=%WINDIR%\System32\OpenSSH\scp.exe)
echo SSH = %SSH%
echo SCP = %SCP%
if "%PYTHON_SOURCE%"=="" (set PYTHON_SOURCE=%here%..\..\)
if "%REMOTE_PYTHON_DIR%"=="" (set REMOTE_PYTHON_DIR=C:\python\)
if NOT "%REMOTE_PYTHON_DIR:~-1,1%"=="\" (set REMOTE_PYTHON_DIR=%REMOTE_PYTHON_DIR%\)
%SSH% %SSH_SERVER% "if EXIST %REMOTE_PYTHON_DIR% (rd %REMOTE_PYTHON_DIR% /s/q)"
%SSH% %SSH_SERVER% "md %REMOTE_PYTHON_DIR%PCBuild\arm32"
%SSH% %SSH_SERVER% "md %REMOTE_PYTHON_DIR%temp"
%SSH% %SSH_SERVER% "md %REMOTE_PYTHON_DIR%Modules"
%SSH% %SSH_SERVER% "md %REMOTE_PYTHON_DIR%PC"
for /f "USEBACKQ" %%i in (`dir PCbuild\*.bat /b`) do @%SCP% PCBuild\%%i "%SSH_SERVER%:%REMOTE_PYTHON_DIR%PCBuild"
for /f "USEBACKQ" %%i in (`dir PCbuild\*.py /b`) do @%SCP% PCBuild\%%i "%SSH_SERVER%:%REMOTE_PYTHON_DIR%PCBuild"
for /f "USEBACKQ" %%i in (`dir PCbuild\arm32\*.exe /b`) do @%SCP% PCBuild\arm32\%%i "%SSH_SERVER%:%REMOTE_PYTHON_DIR%PCBuild\arm32"
for /f "USEBACKQ" %%i in (`dir PCbuild\arm32\*.pyd /b`) do @%SCP% PCBuild\arm32\%%i "%SSH_SERVER%:%REMOTE_PYTHON_DIR%PCBuild\arm32"
for /f "USEBACKQ" %%i in (`dir PCbuild\arm32\*.dll /b`) do @%SCP% PCBuild\arm32\%%i "%SSH_SERVER%:%REMOTE_PYTHON_DIR%PCBuild\arm32"
%SCP% -r "%PYTHON_SOURCE%Include" "%SSH_SERVER%:%REMOTE_PYTHON_DIR%Include"
%SCP% -r "%PYTHON_SOURCE%Lib" "%SSH_SERVER%:%REMOTE_PYTHON_DIR%Lib"
%SCP% -r "%PYTHON_SOURCE%Tools" "%SSH_SERVER%:%REMOTE_PYTHON_DIR%Tools"
%SCP% "%PYTHON_SOURCE%Modules\Setup" "%SSH_SERVER%:%REMOTE_PYTHON_DIR%Modules"
%SCP% "%PYTHON_SOURCE%PC\pyconfig.h" "%SSH_SERVER%:%REMOTE_PYTHON_DIR%PC"

exit /b %ERRORLEVEL%

:Arm32SshHelp
echo SSH_SERVER environment variable must be set to administrator@[ip address]
echo where [ip address] is the address of a Windows IoT Core ARM32 device.
echo.
echo The test worker should have the SSH agent running.
echo Also a key must be created with ssh-keygen and added to both the buildbot worker machine
echo and the ARM32 worker device: see https://docs.microsoft.com/en-us/windows/iot-core/connect-your-device/ssh
exit /b 127
