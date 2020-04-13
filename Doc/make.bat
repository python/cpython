@echo off
setlocal

pushd %~dp0

set this=%~n0

call ..\PCbuild\find_python.bat %PYTHON%

if not defined PYTHON set PYTHON=py

if not defined SPHINXBUILD (
    %PYTHON% -c "import sphinx" > nul 2> nul
    if errorlevel 1 (
        echo Installing sphinx with %PYTHON%
        %PYTHON% -m pip install sphinx
        if errorlevel 1 exit /B
    )
    set SPHINXBUILD=%PYTHON% -c "import sphinx.cmd.build, sys; sys.exit(sphinx.cmd.build.main())"
)

%PYTHON% -c "import python_docs_theme" > nul 2> nul
if errorlevel 1 (
    echo Installing python-docs-theme with %PYTHON%
    %PYTHON% -m pip install python-docs-theme
    if errorlevel 1 exit /B
)

if not defined BLURB (
    %PYTHON% -c "import blurb" > nul 2> nul
    if errorlevel 1 (
        echo Installing blurb with %PYTHON%
        %PYTHON% -m pip install blurb
        if errorlevel 1 exit /B
    )
    set BLURB=%PYTHON% -m blurb
)

if "%1" NEQ "htmlhelp" goto :skiphhcsearch
if exist "%HTMLHELP%" goto :skiphhcsearch

rem Search for HHC in likely places
set HTMLHELP=
where hhc /q && set "HTMLHELP=hhc" && goto :skiphhcsearch
where /R ..\externals hhc > "%TEMP%\hhc.loc" 2> nul && set /P HTMLHELP= < "%TEMP%\hhc.loc" & del "%TEMP%\hhc.loc"
if not exist "%HTMLHELP%" where /R "%ProgramFiles(x86)%" hhc > "%TEMP%\hhc.loc" 2> nul && set /P HTMLHELP= < "%TEMP%\hhc.loc" & del "%TEMP%\hhc.loc"
if not exist "%HTMLHELP%" where /R "%ProgramFiles%" hhc > "%TEMP%\hhc.loc" 2> nul && set /P HTMLHELP= < "%TEMP%\hhc.loc" & del "%TEMP%\hhc.loc"
if not exist "%HTMLHELP%" (
    echo.
    echo.The HTML Help Workshop was not found.  Set the HTMLHELP variable
    echo.to the path to hhc.exe or download and install it from
    echo.http://msdn.microsoft.com/en-us/library/ms669985
    exit /B 1
)
:skiphhcsearch

if not defined DISTVERSION for /f "usebackq" %%v in (`%PYTHON% tools/extensions/patchlevel.py`) do set DISTVERSION=%%v

if not defined BUILDDIR set BUILDDIR=build

rem Targets that don't require sphinx-build
if "%1" EQU "" goto help
if "%1" EQU "help" goto help
if "%1" EQU "check" goto check
if "%1" EQU "serve" goto serve
if "%1" == "clean" (
    rmdir /q /s "%BUILDDIR%"
    goto end
)

%SPHINXBUILD% >nul 2> nul
if errorlevel 9009 (
    echo.
    echo.The 'sphinx-build' command was not found. Make sure you have Sphinx
    echo.installed, then set the SPHINXBUILD environment variable to point
    echo.to the full path of the 'sphinx-build' executable. Alternatively you
    echo.may add the Sphinx directory to PATH.
    echo.
    echo.If you don't have Sphinx installed, grab it from
    echo.http://sphinx-doc.org/
    popd
    exit /B 1
)

rem Targets that do require sphinx-build and have their own label
if "%1" EQU "htmlview" goto htmlview

rem Everything else
goto build

:help
echo.usage: %this% BUILDER [filename ...]
echo.
echo.Call %this% with the desired Sphinx builder as the first argument, e.g.
echo.``%this% html`` or ``%this% doctest``.  Interesting targets that are
echo.always available include:
echo.
echo.   Provided by Sphinx:
echo.      html, htmlhelp, latex, text
echo.      suspicious, linkcheck, changes, doctest
echo.   Provided by this script:
echo.      clean, check, serve, htmlview
echo.
echo.All arguments past the first one are passed through to sphinx-build as
echo.filenames to build or are ignored.  See README.rst in this directory or
echo.the documentation for your version of Sphinx for more exhaustive lists
echo.of available targets and descriptions of each.
echo.
echo.This script assumes that the SPHINXBUILD environment variable contains
echo.a legitimate command for calling sphinx-build, or that sphinx-build is
echo.on your PATH if SPHINXBUILD is not set.  Options for sphinx-build can
echo.be passed by setting the SPHINXOPTS environment variable.
goto end

:build
if not exist "%BUILDDIR%" mkdir "%BUILDDIR%"

rem PY_MISC_NEWS_DIR is also used by our Sphinx extension in tools/extensions/pyspecific.py
if not defined PY_MISC_NEWS_DIR set PY_MISC_NEWS_DIR=%BUILDDIR%\%1
if not exist "%PY_MISC_NEWS_DIR%" mkdir "%PY_MISC_NEWS_DIR%"
if exist ..\Misc\NEWS (
    echo.Copying Misc\NEWS to %PY_MISC_NEWS_DIR%\NEWS
    copy ..\Misc\NEWS "%PY_MISC_NEWS_DIR%\NEWS" > nul
) else if exist ..\Misc\NEWS.D (
    if defined BLURB (
        echo.Merging Misc/NEWS with %BLURB%
        %BLURB% merge -f "%PY_MISC_NEWS_DIR%\NEWS"
    ) else (
        echo.No Misc/NEWS file and Blurb is not available.
        exit /B 1
    )
)

if defined PAPER (
    set SPHINXOPTS=-D latex_elements.papersize=%PAPER% %SPHINXOPTS%
)
if "%1" EQU "htmlhelp" (
    set SPHINXOPTS=-D html_theme_options.body_max_width=none %SPHINXOPTS%
)
cmd /S /C "%SPHINXBUILD% %SPHINXOPTS% -b%1 -dbuild\doctrees . "%BUILDDIR%\%1" %2 %3 %4 %5 %6 %7 %8 %9"

if "%1" EQU "htmlhelp" (
    "%HTMLHELP%" "%BUILDDIR%\htmlhelp\python%DISTVERSION:.=%.hhp"
    rem hhc.exe seems to always exit with code 1, reset to 0 for less than 2
    if not errorlevel 2 cmd /C exit /b 0
)

echo.
if errorlevel 1 (
    echo.Build failed (exit code %ERRORLEVEL%^), check for error messages
    echo.above.  Any output will be found in %BUILDDIR%\%1
) else (
    echo.Build succeeded. All output should be in %BUILDDIR%\%1
)
goto end

:htmlview
if NOT "%2" EQU "" (
    echo.Can't specify filenames to build with htmlview target, ignoring.
)
cmd /C %this% html

if EXIST "%BUILDDIR%\html\index.html" (
    echo.Opening "%BUILDDIR%\html\index.html" in the default web browser...
    start "" "%BUILDDIR%\html\index.html"
)

goto end

:check
cmd /S /C "%PYTHON% tools\rstlint.py -i tools"
goto end

:serve
cmd /S /C "%PYTHON% ..\Tools\scripts\serve.py "%BUILDDIR%\html""
goto end

:end
popd
