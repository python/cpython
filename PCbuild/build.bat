@echo off
goto Run
:Usage
echo.%~nx0 [flags and arguments] [quoted MSBuild options]
echo.
echo.Build CPython from the command line.  Requires the appropriate
echo.version(s) of Microsoft Visual Studio to be installed (see readme.txt).
echo.Also requires Subversion (svn.exe) to be on PATH if the '-e' flag is
echo.given.
echo.
echo.After the flags recognized by this script, up to 9 arguments to be passed
echo.directly to MSBuild may be passed.  If the argument contains an '=', the
echo.entire argument must be quoted (e.g. `%~nx0 "/p:PlatformToolset=v100"`)
echo.
echo.Available flags:
echo.  -h  Display this help message
echo.  -V  Display version information for the current build
echo.  -r  Target Rebuild instead of Build
echo.  -d  Set the configuration to Debug
echo.  -e  Build external libraries fetched by get_externals.bat
echo.      Extension modules that depend on external libraries will not attempt
echo.      to build if this flag is not present
echo.  -m  Enable parallel build (enabled by default)
echo.  -M  Disable parallel build
echo.  -v  Increased output messages
echo.  -k  Attempt to kill any running Pythons before building (usually done
echo.      automatically by the pythoncore project)
echo.  --pgo          Build with Profile-Guided Optimization.  This flag
echo.                 overrides -c and -d
echo.  --test-marker  Enable the test marker within the build.
echo.
echo.Available flags to avoid building certain modules.
echo.These flags have no effect if '-e' is not given:
echo.  --no-ssl      Do not attempt to build _ssl
echo.  --no-tkinter  Do not attempt to build Tkinter
echo.
echo.Available arguments:
echo.  -c Release ^| Debug ^| PGInstrument ^| PGUpdate
echo.     Set the configuration (default: Release)
echo.  -p x64 ^| Win32
echo.     Set the platform (default: Win32)
echo.  -t Build ^| Rebuild ^| Clean ^| CleanAll
echo.     Set the target manually
echo.  --pgo-job  The job to use for PGO training; implies --pgo
echo.             (default: "-m test --pgo")
exit /b 127

:Run
setlocal
set platf=Win32
set vs_platf=x86
set conf=Release
set target=Build
set dir=%~dp0
set parallel=/m
set verbose=/nologo /v:m
set kill=
set do_pgo=
set pgo_job=-m test --pgo
set on_64_bit=true

rem This may not be 100% accurate, but close enough.
if "%ProgramFiles(x86)%"=="" (set on_64_bit=false)

:CheckOpts
if "%~1"=="-h" goto Usage
if "%~1"=="-c" (set conf=%2) & shift & shift & goto CheckOpts
if "%~1"=="-p" (set platf=%2) & shift & shift & goto CheckOpts
if "%~1"=="-r" (set target=Rebuild) & shift & goto CheckOpts
if "%~1"=="-t" (set target=%2) & shift & shift & goto CheckOpts
if "%~1"=="-d" (set conf=Debug) & shift & goto CheckOpts
if "%~1"=="-m" (set parallel=/m) & shift & goto CheckOpts
if "%~1"=="-M" (set parallel=) & shift & goto CheckOpts
if "%~1"=="-v" (set verbose=/v:n) & shift & goto CheckOpts
if "%~1"=="-k" (set kill=true) & shift & goto CheckOpts
if "%~1"=="--pgo" (set do_pgo=true) & shift & goto CheckOpts
if "%~1"=="--pgo-job" (set do_pgo=true) & (set pgo_job=%~2) & shift & shift & goto CheckOpts
if "%~1"=="--test-marker" (set UseTestMarker=true) & shift & goto CheckOpts
if "%~1"=="-V" shift & goto Version
rem These use the actual property names used by MSBuild.  We could just let
rem them in through the environment, but we specify them on the command line
rem anyway for visibility so set defaults after this
if "%~1"=="-e" (set IncludeExternals=true) & shift & goto CheckOpts
if "%~1"=="--no-ssl" (set IncludeSSL=false) & shift & goto CheckOpts
if "%~1"=="--no-tkinter" (set IncludeTkinter=false) & shift & goto CheckOpts

if "%IncludeExternals%"=="" set IncludeExternals=false
if "%IncludeSSL%"=="" set IncludeSSL=true
if "%IncludeTkinter%"=="" set IncludeTkinter=true

if "%IncludeExternals%"=="true" call "%dir%get_externals.bat"

if "%platf%"=="x64" (
    if "%on_64_bit%"=="true" (
        rem This ought to always be correct these days...
        set vs_platf=amd64
    ) else (
        if "%do_pgo%"=="true" (
            echo.ERROR: Cannot cross-compile with PGO
            echo.    32bit operating system detected, if this is incorrect,
            echo.    make sure the ProgramFiles(x86^) environment variable is set
            exit /b 1
        )
        set vs_platf=x86_amd64
    )
)

if not exist "%GIT%" where git > "%TEMP%\git.loc" 2> nul && set /P GIT= < "%TEMP%\git.loc" & del "%TEMP%\git.loc"
if exist "%GIT%" set GITProperty=/p:GIT="%GIT%"
if not exist "%GIT%" echo Cannot find Git on PATH & set GITProperty=

rem Setup the environment
call "%dir%env.bat" %vs_platf% >nul

if "%kill%"=="true" call :Kill

if "%do_pgo%"=="true" (
    set conf=PGInstrument
    call :Build
    del /s "%dir%\*.pgc"
    del /s "%dir%\..\Lib\*.pyc"
    echo on
    call "%dir%\..\python.bat" %pgo_job%
    @echo off
    call :Kill
    set conf=PGUpdate
    set target=Build
)
goto Build
:Kill
echo on
msbuild "%dir%\pythoncore.vcxproj" /t:KillPython %verbose%^
 /p:Configuration=%conf% /p:Platform=%platf%^
 /p:KillPython=true

@echo off
goto :eof

:Build
rem Call on MSBuild to do the work, echo the command.
rem Passing %1-9 is not the preferred option, but argument parsing in
rem batch is, shall we say, "lackluster"
echo on
msbuild "%dir%pcbuild.proj" /t:%target% %parallel% %verbose%^
 /p:Configuration=%conf% /p:Platform=%platf%^
 /p:IncludeExternals=%IncludeExternals%^
 /p:IncludeSSL=%IncludeSSL% /p:IncludeTkinter=%IncludeTkinter%^
 /p:UseTestMarker=%UseTestMarker% %GITProperty%^
 %1 %2 %3 %4 %5 %6 %7 %8 %9

@echo off
goto :eof

:Version
rem Display the current build version information
msbuild "%dir%python.props" /t:ShowVersionInfo /v:m /nologo %1 %2 %3 %4 %5 %6 %7 %8 %9
