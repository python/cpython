@setlocal enableextensions
@echo off

set D=%~dp0
set PCBUILD=%D%..\..\PCbuild\

set TARGETDIR=%TEMP%
set TESTX86=
set TESTX64=
set TESTALLUSER=
set TESTPERUSER=

:CheckOpts
if "%1" EQU "-h" goto Help
if "%1" EQU "-x86" (set TESTX86=1) && shift && goto CheckOpts
if "%1" EQU "-x64" (set TESTX64=1) && shift && goto CheckOpts
if "%1" EQU "-t" (set TARGETDIR=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "--target" (set TARGETDIR=%~2) && shift && shift && goto CheckOpts
if "%1" EQU "-a" (set TESTALLUSER=1) && shift && goto CheckOpts
if "%1" EQU "--alluser" (set TESTALLUSER=1) && shift && goto CheckOpts
if "%1" EQU "-p" (set TESTPERUSER=1) && shift && goto CheckOpts
if "%1" EQU "--peruser" (set TESTPERUSER=1) && shift && goto CheckOpts

if not defined TESTX86 if not defined TESTX64 (set TESTX86=1) && (set TESTX64=1)
if not defined TESTALLUSER if not defined TESTPERUSER (set TESTALLUSER=1) && (set TESTPERUSER=1)


if defined TESTX86 (
    for %%f in ("%PCBUILD%win32\en-us\*.exe") do (
        if defined TESTALLUSER call :test "%%~ff" "%TARGETDIR%\%%~nf-alluser" "InstallAllUsers=1 CompileAll=1"
        if errorlevel 1 exit /B
        if defined TESTPERUSER call :test "%%~ff" "%TARGETDIR%\%%~nf-peruser" "InstallAllUsers=0 CompileAll=0"
        if errorlevel 1 exit /B
    )
)

if defined TESTX64 (
    for %%f in ("%PCBUILD%amd64\en-us\*.exe") do (
        if defined TESTALLUSER call :test "%%~ff" "%TARGETDIR%\%%~nf-alluser" "InstallAllUsers=1 CompileAll=1"
        if errorlevel 1 exit /B
        if defined TESTPERUSER call :test "%%~ff" "%TARGETDIR%\%%~nf-peruser" "InstallAllUsers=0 CompileAll=0"
        if errorlevel 1 exit /B
    )
)

exit /B 0

:test
@setlocal
@echo on

@if not exist "%~1" exit /B 1

@set EXE=%~1
@if not "%EXE:embed=%"=="%EXE%" exit /B 0

@set EXITCODE=0
@echo Installing %1 into %2
"%~1" /passive /log "%~2\install\log.txt" TargetDir="%~2\Python" Include_debug=1 Include_symbols=1 %~3

@if not errorlevel 1 (
    @echo Printing version
    "%~2\Python\python.exe" -c "import sys; print(sys.version)" > "%~2\version.txt" 2>&1
)

@if not errorlevel 1 (
    @echo Capturing Start Menu
    @dir /s/b "%PROGRAMDATA%\Microsoft\Windows\Start Menu\Programs" | findstr /ic:"python" > "%~2\startmenu.txt" 2>&1
    @dir /s/b "%APPDATA%\Microsoft\Windows\Start Menu\Programs" | findstr /ic:"python"  >> "%~2\startmenu.txt" 2>&1

    @echo Capturing registry
    @for /F "usebackq" %%f in (`reg query HKCR /s /f python /k`) do @(
        echo %%f >> "%~2\hkcr.txt"
        reg query "%%f" /s >> "%~2\hkcr.txt" 2>&1
    )
    @reg query HKCU\Software\Python /s > "%~2\hkcu.txt" 2>&1
    @reg query HKLM\Software\Python /reg:32 /s > "%~2\hklm.txt" 2>&1
    @reg query HKLM\Software\Python /reg:64 /s >> "%~2\hklm.txt" 2>&1
    cmd /k exit 0
)

@if not errorlevel 1 (
    @echo Installing package
    "%~2\Python\python.exe" -m pip install "azure<0.10" > "%~2\pip.txt" 2>&1
    @if not errorlevel 1 (
        "%~2\Python\python.exe" -m pip uninstall -y azure python-dateutil six >> "%~2\pip.txt" 2>&1
    )
)
@if not errorlevel 1 (
    @echo Testing Tcl/tk
    @set TCL_LIBRARY=%~2\Python\tcl\tcl8.6
    "%~2\Python\python.exe" -m test -uall -v test_ttk_guionly test_tk test_idle > "%~2\tcltk.txt" 2>&1
    @set TCL_LIBRARY=
)

@set EXITCODE=%ERRORLEVEL%

@echo Result was %EXITCODE%
@echo Removing %1
"%~1" /passive /uninstall /log "%~2\uninstall\log.txt"

@echo off
exit /B %EXITCODE%

:Help
echo testrelease.bat [--target TARGET] [-x86] [-x64] [--alluser] [--peruser] [-h]
echo.
echo    --target (-t)   Specify the target directory for installs and logs
echo    -x86            Run tests for x86 installers
echo    -x64            Run tests for x64 installers
echo    --alluser (-a)  Run tests for all-user installs (requires Administrator)
echo    --peruser (-p)  Run tests for per-user installs
echo    -h              Display this help information
echo.
echo If no test architecture is specified, all architectures will be tested.
echo If no install type is selected, all install types will be tested.
echo.
