@echo off
rem A batch program to build or rebuild a particular configuration.
rem just for convenience.

setlocal
set platf=Win32
set conf=Release
set build=/build

:CheckOpts
if "%1"=="-c" (set conf=%2)     & shift & shift & goto CheckOpts
if "%1"=="-p" (set platf=%2) & shift & shift & goto CheckOpts
if "%1"=="-r" (set build=/rebuild)    & shift & goto CheckOpts

set cmd=devenv pcbuild.sln %build% "%conf%|%platf%"
echo %cmd%
%cmd%

rem Copy whatever was built to the canonical 'PCBuild' directory.
rem This helps extensions which use distutils etc.
rem (Don't check if the build was successful - we expect a few failures
rem due to missing libs)
echo Copying built files to ..\PCBuild
if not exist %platf%%conf%\. (echo %platf%%conf% does not exist - nothing copied & goto xit)
if not exist ..\PCBuild\. (echo ..\PCBuild does not exist - nothing copied & goto xit)
xcopy /q/y %platf%%conf%\* ..\PCBuild\.

:xit
