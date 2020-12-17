@echo off
@set OPENSSL=%~dp0apps

title OpenSSL Running for Windows 10 x64
echo OpenSSL Running for Windows 10 x64
echo.

%SystemDrive%
cd %OPENSSL%

openssl version -a
echo.

cmd.exe /K
