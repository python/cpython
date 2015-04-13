@echo off
setlocal

set D=%~dp0
set PCBUILD=%~dp0..\..\PCBuild\


echo Building Lib\distutils\command\wininst-xx.0.exe

call "%PCBUILD%env.bat" x86
if errorlevel 1 goto :eof

msbuild "%D%bdist_wininst.vcxproj" "/p:SolutionDir=%PCBUILD%\" /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 goto :eof


echo Building Lib\distutils\command\wininst-xx.0-amd64.exe

call "%PCBUILD%env.bat" x86_amd64
if errorlevel 1 goto :eof

msbuild "%D%bdist_wininst.vcxproj" "/p:SolutionDir=%PCBUILD%\" /p:Configuration=Release /p:Platform=x64
