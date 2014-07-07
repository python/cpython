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

echo Purging all non-tracked files with `hg purge`
echo on
hg -R "%root%" --config extensions.purge= purge --all -X "%root%\Lib\test\data"
