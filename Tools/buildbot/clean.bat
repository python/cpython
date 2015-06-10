@echo off
rem Used by the buildbot "clean" step.

setlocal
set root=%~dp0..\..
set pcbuild=%root%\PCbuild

echo Deleting build
call "%pcbuild%\build.bat" -t Clean -k %*
call "%pcbuild%\build.bat" -t Clean -k -d %*

echo Deleting .pyc/.pyo files ...
del /s "%root%\Lib\*.pyc" "%root%\Lib\*.pyo"

echo Deleting test leftovers ...
rmdir /s /q "%root%\build"
