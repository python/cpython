@echo off
rem
rem Runs the blurb tool. If necessary, will install Python and/or blurb.
rem
rem Pass "--update"/"-U" as the first argument to update blurb.
rem

call "%~dp0find_python.bat" %PYTHON%
if ERRORLEVEL 1 (echo Cannot locate python.exe on PATH or as PYTHON variable & exit /b 3)

if "%1" EQU "--update" (%PYTHON% -m pip install -U blurb && shift)
if "%1" EQU "-U" (%PYTHON% -m pip install -U blurb && shift)

%PYTHON% -m blurb %1 %2 %3 %4 %5 %6 %7 %8 %9
if ERRORLEVEL 1 goto :install_and_retry
exit /B 0

:install_and_retry
rem Before reporting the error, make sure that blurb is actually installed.
rem If not, install it first and try again.
set _ERR=%ERRORLEVEL%
%PYTHON% -c "import blurb"
if NOT ERRORLEVEL 1 exit /B %_ERR%
echo Installing blurb...
%PYTHON% -m pip install blurb
if ERRORLEVEL 1 exit /B %ERRORLEVEL%
%PYTHON% -m blurb %*
exit /B
