@@echo off
setlocal

set SVNROOT=http://svn.python.org/projects
if "%PYTHON%" EQU "" set PYTHON=..\pcbuild\python
if "%HTMLHELP%" EQU "" set HTMLHELP=%ProgramFiles%\HTML Help Workshop\hhc.exe

if "%1" EQU "" goto help
if "%1" EQU "html" goto build
if "%1" EQU "htmlhelp" goto build
if "%1" EQU "web" goto build
if "%1" EQU "webrun" goto webrun
if "%1" EQU "checkout" goto checkout
if "%1" EQU "update" goto update

:help
echo HELP
echo.
echo builddoc checkout
echo builddoc update
echo builddoc html
echo builddoc htmlhelp
echo builddoc web
echo builddoc webrun
echo.
goto end

:checkout
svn co %SVNROOT%/doctools/trunk/sphinx tools/sphinx
svn co %SVNROOT%/external/docutils-0.4/docutils tools/docutils
svn co %SVNROOT%/external/Jinja-1.1/jinja tools/jinja
svn co %SVNROOT%/external/Pygments-0.9/pygments tools/pygments
goto end

:update
svn update tools/sphinx
svn update tools/docutils
svn update tools/jinja
svn update tools/pygments
goto end

:build
if not exist build mkdir build
if not exist build\%1 mkdir build\%1
if not exist build\doctrees mkdir build\doctrees
cmd /C %PYTHON% tools\sphinx-build.py -b%1 -dbuild\doctrees . build\%1
if "%1" EQU "htmlhelp" "%HTMLHELP%" build\htmlhelp\pydoc.hhp
goto end

:webrun
set PYTHONPATH=tools
%PYTHON% -m sphinx.web build\web
goto end

:end
