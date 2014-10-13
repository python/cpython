@echo off
rem Used by the buildbot "clean" step.

setlocal
set root=%~dp0..\..
set pcbuild=%root%\PCbuild

echo.Attempting to kill Pythons...
for %%k in (kill_python.exe
            kill_python_d.exe
            amd64\kill_python.exe
            amd64\kill_python_d.exe
            ) do (
    if exist "%pcbuild%\%%k" (
        echo.Calling %pcbuild%\%%k...
        "%pcbuild%\%%k"
    )
)
if "%1" == "x64" (
    set vcvars_target=x86_amd64
    set platform_target=x64
) else (
    set vcvars_target=x86
    set platform_target=x86
)
call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" %vcvars_target%
echo Deleting .pyc/.pyo files ...
del /s "%root%\Lib\*.pyc" "%root%\Lib\*.pyo"
echo Deleting test leftovers ...
rmdir /s /q "%root%\build"

msbuild /target:clean "%pcbuild%\pcbuild.sln" /p:Configuration=Release /p:PlatformTarget=%platform_target%
msbuild /target:clean "%pcbuild%\pcbuild.sln" /p:Configuration=Debug /p:PlatformTarget=%platform_target%
