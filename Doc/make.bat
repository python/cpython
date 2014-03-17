@@echo off
setlocal

if "%PYTHON%" EQU "" set PYTHON=py -2
if "%HTMLHELP%" EQU "" set HTMLHELP=%ProgramFiles%\HTML Help Workshop\hhc.exe
if "%DISTVERSION%" EQU "" for /f "usebackq" %%v in (`%PYTHON% tools/sphinxext/patchlevel.py`) do set DISTVERSION=%%v

if "%1" EQU "" goto help
if "%1" EQU "html" goto build
if "%1" EQU "htmlhelp" goto build
if "%1" EQU "latex" goto build
if "%1" EQU "text" goto build
if "%1" EQU "suspicious" goto build
if "%1" EQU "linkcheck" goto build
if "%1" EQU "changes" goto build

:help
set this=%~n0
echo HELP
echo.
echo %this% html
echo %this% htmlhelp
echo %this% latex
echo %this% text
echo %this% suspicious
echo %this% linkcheck
echo %this% changes
echo.
goto end

:build
if not exist build mkdir build
if not exist build\%1 mkdir build\%1
if not exist build\doctrees mkdir build\doctrees
cmd /C %PYTHON% --version
cmd /C %PYTHON% tools\sphinx-build.py -b%1 -dbuild\doctrees . build\%*
if "%1" EQU "htmlhelp" "%HTMLHELP%" build\htmlhelp\python%DISTVERSION:.=%.hhp
goto end

:end
