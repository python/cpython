@echo off
rem Used by the buildbot "clean" step.

setlocal
set root=%~dp0..\..
set pcbuild=%root%\PCbuild

if "%1" == "x64" (
    set vcvars_target=x86_amd64
    set platform=x64
) else (
    set vcvars_target=x86
    set platform=Win32
)

call "%pcbuild%\env.bat" %vcvars_target%

echo.Attempting to kill Pythons...
msbuild /v:m /nologo /target:KillPython "%pcbuild%\pythoncore.vcxproj" /p:Configuration=Release /p:Platform=%platform% /p:KillPython=true

echo Deleting .pyc/.pyo files ...
del /s "%root%\Lib\*.pyc" "%root%\Lib\*.pyo"

echo Deleting test leftovers ...
rmdir /s /q "%root%\build"

echo Deleting build
msbuild /v:m /nologo /target:clean "%pcbuild%\pcbuild.proj" /p:Configuration=Release /p:Platform=%platform%
msbuild /v:m /nologo /target:clean "%pcbuild%\pcbuild.proj" /p:Configuration=Debug /p:Platform=%platform%
