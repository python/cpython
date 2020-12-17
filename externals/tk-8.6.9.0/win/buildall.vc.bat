@echo off

::  This is an example batchfile for building everything. Please
::  edit this (or make your own) for your needs and wants using
::  the instructions for calling makefile.vc found in makefile.vc

set SYMBOLS=

:OPTIONS
if "%1" == "/?" goto help
if /i "%1" == "/help" goto help
if %1.==symbols. goto SYMBOLS
if %1.==debug. goto SYMBOLS
goto OPTIONS_DONE

:SYMBOLS
   set SYMBOLS=symbols
   shift
   goto OPTIONS

:OPTIONS_DONE

:: reset errorlevel
cd > nul

:: You might have installed your developer studio to add itself to the
:: path or have already run vcvars32.bat.  Testing these envars proves
:: cl.exe and friends are in your path.
::
if defined VCINSTALLDIR  (goto :startBuilding)
if defined MSDEVDIR      (goto :startBuilding)
if defined MSVCDIR       (goto :startBuilding)
if defined MSSDK         (goto :startBuilding)
if defined WINDOWSSDKDIR (goto :startBuilding)

:: We need to run the development environment batch script that comes
:: with developer studio (v4,5,6,7,etc...)  All have it.  This path
:: might not be correct.  You should call it yourself prior to running
:: this batchfile.
::
call "C:\Program Files\Microsoft Developer Studio\vc98\bin\vcvars32.bat"
if errorlevel 1 (goto no_vcvars)

:startBuilding

echo.
echo Sit back and have a cup of coffee while this grinds through ;)
echo You asked for *everything*, remember?
echo.
title Building Tk, please wait...


:: makefile.vc uses this for its default anyways, but show its use here
:: just to be explicit and convey understanding to the user.  Setting
:: the INSTALLDIR envar prior to running this batchfile affects all builds.
::
if "%INSTALLDIR%" == "" set INSTALLDIR=C:\Program Files\Tcl


:: Where is the Tcl source directory?
:: You can set the TCLDIR environment variable to your Tcl HEAD checkout
if "%TCLDIR%" == "" set TCLDIR=..\..\tcl

:: Build the normal stuff along with the help file.
::
set OPTS=none
if not %SYMBOLS%.==. set OPTS=symbols
nmake -nologo -f makefile.vc release htmlhelp OPTS=%OPTS% %1
if errorlevel 1 goto error

:: Build the static core and shell.
::
set OPTS=static,msvcrt
if not %SYMBOLS%.==. set OPTS=symbols,static,msvcrt
nmake -nologo -f makefile.vc shell OPTS=%OPTS% %1
if errorlevel 1 goto error

set OPTS=
set SYMBOLS=
goto end

:error
echo *** BOOM! ***
goto end

:no_vcvars
echo vcvars32.bat was not run prior to this batchfile, nor are the MS tools in your path.
goto out

:help
title buildall.vc.bat help message
echo usage:
echo   %0                 : builds Tk for all build types (do this first)
echo   %0 install         : installs all the release builds (do this second)
echo   %0 symbols         : builds Tk for all debugging build types
echo   %0 symbols install : install all the debug builds.
echo.
goto out

:end
title Building Tk, please wait... DONE!
echo DONE!
goto out

:out
pause
title Command Prompt
