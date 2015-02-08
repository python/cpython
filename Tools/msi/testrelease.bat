@setlocal
@echo off

set D=%~dp0
set PCBUILD=%D%..\..\PCBuild\

set TARGETDIR=%TEMP%
set TESTX86=
set TESTX64=
set TESTALLUSER=
set TESTPERUSER=

:CheckOpts
if '%1' EQU '-x86' (set TESTX86=1) && shift && goto CheckOpts
if '%1' EQU '-x64' (set TESTX64=1) && shift && goto CheckOpts
if '%1' EQU '-t' (set TARGETDIR=%~2) && shift && shift && goto CheckOpts
if '%1' EQU '-a' (set TESTALLUSER=1) && shift && goto CheckOpts
if '%1' EQU '-p' (set TESTPERUSER=1) && shift && goto CheckOpts

if not defined TESTX86 if not defined TESTX64 (set TESTX86=1) && (set TESTX64=1)
if not defined TESTALLUSER if not defined TESTPERUSER (set TESTALLUSER=1) && (set TESTPERUSER=1)


if defined TESTX86 (
    for %%f in ("%PCBUILD%win32\en-us\*.exe") do (
        if defined TESTALLUSER call :test "%%~ff" "%TARGETDIR%\%%~nf-alluser" InstallAllUsers=1
        if defined TESTPERUSER call :test "%%~ff" "%TARGETDIR%\%%~nf-peruser" InstallAllUsers=0
        if errorlevel 1 exit /B
    )
)

if defined TESTX64 (
    for %%f in ("%PCBUILD%amd64\en-us\*.exe") do (
        if defined TESTALLUSER call :test "%%~ff" "%TARGETDIR%\%%~nf-alluser" InstallAllUsers=1
        if defined TESTPERUSER call :test "%%~ff" "%TARGETDIR%\%%~nf-peruser" InstallAllUsers=0
        if errorlevel 1 exit /B
    )
)

exit /B 0

:test
@setlocal
@echo on

@if not exist "%~1" exit /B 1

@set EXITCODE=0
@echo Installing %1 into %2
"%~1" /passive /log "%~2\install\log.txt" %~3 TargetDir="%~2\Python" Include_debug=1 Include_symbols=1 CompileAll=1

@if not errorlevel 1 (
    @echo Printing version
    "%~2\Python\python.exe" -c "import sys; print(sys.version)" > "%~2\version.txt" 2>&1
)
@if not errorlevel 1 (
    @echo Installing package
    "%~2\Python\python.exe" -m pip install azure > "%~2\pip.txt" 2>&1
    @if not errorlevel 1 (
        "%~2\Python\python.exe" -m pip uninstall -y azure python-dateutil six > "%~2\pip.txt" 2>&1
    )
)
@if not errorlevel 1 (
    @echo Testing Tcl/tk
    @set TCL_LIBRARY=%~2\Python\tcl\tcl8.6
    "%~2\Python\python.exe" -m test -uall -v test_ttk_guionly test_tk test_idle > "%~2\tcltk.txt" 2>&1
    @set TCL_LIBRARY=
)

@set EXITCODE=%ERRORLEVEL%

@for /d %%f in ("%PROGRAMDATA%\Microsoft\Windows\Start Menu\Programs\Python*") do @dir "%%~ff\*.lnk" /s/b > "%~2\startmenu.txt" 2>&1
@for /d %%f in ("%APPDATA%\Microsoft\Windows\Start Menu\Programs\Python*") do @dir "%%~ff\*.lnk" /s/b >> "%~2\startmenu.txt" 2>&1

@echo Result was %EXITCODE%
@echo Removing %1
"%~1" /passive /uninstall /log "%~2\uninstall\log.txt"

@echo off
exit /B %EXITCODE%
