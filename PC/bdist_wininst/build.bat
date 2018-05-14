@echo off
setlocal

set D=%~dp0
set PCBUILD=%~dp0..\..\PCBuild\


echo Building Lib\distutils\command\wininst-xx.0.exe

call "%PCBUILD%find_msbuild.bat" %MSBUILD%
if ERRORLEVEL 1 (echo Cannot locate MSBuild.exe on PATH or as MSBUILD variable & exit /b 2)

%MSBUILD% "%D%bdist_wininst.vcxproj" "/p:SolutionDir=%PCBUILD%\" /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 goto :eof


echo Building Lib\distutils\command\wininst-xx.0-amd64.exe

%MSBUILD% "%D%bdist_wininst.vcxproj" "/p:SolutionDir=%PCBUILD%\" /p:Configuration=Release /p:Platform=x64
