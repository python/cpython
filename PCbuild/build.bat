@echo off
rem A batch program to build or rebuild a particular configuration,
rem just for convenience.

rem Arguments:
rem  -c  Set the configuration (default: Release)
rem  -p  Set the platform (x64 or Win32, default: Win32)
rem  -r  Target Rebuild instead of Build
rem  -t  Set the target manually (Build, Rebuild, or Clean)
rem  -d  Set the configuration to Debug
rem  -e  Pull in external libraries using get_externals.bat
rem  -m  Enable parallel build
rem  -M  Disable parallel build (disabled by default)
rem  -v  Increased output messages
rem  -k  Attempt to kill any running Pythons before building

setlocal
set platf=Win32
set vs_platf=x86
set conf=Release
set target=Build
set dir=%~dp0
set parallel=
set verbose=/nologo /v:m
set kill=
set build_tkinter=

:CheckOpts
if '%1'=='-c' (set conf=%2) & shift & shift & goto CheckOpts
if '%1'=='-p' (set platf=%2) & shift & shift & goto CheckOpts
if '%1'=='-r' (set target=Rebuild) & shift & goto CheckOpts
if '%1'=='-t' (set target=%2) & shift & shift & goto CheckOpts
if '%1'=='-d' (set conf=Debug) & shift & goto CheckOpts
if '%1'=='-e' call "%dir%get_externals.bat" & (set build_tkinter=true) & shift & goto CheckOpts
if '%1'=='-m' (set parallel=/m) & shift & goto CheckOpts
if '%1'=='-M' (set parallel=) & shift & goto CheckOpts
if '%1'=='-v' (set verbose=/v:n) & shift & goto CheckOpts
if '%1'=='-k' (set kill=true) & shift & goto CheckOpts

if '%conf%'=='Debug' (set dbg_ext=_d) else (set dbg_ext=)
if '%platf%'=='x64' (
    set vs_platf=x86_amd64
    set builddir=%dir%amd64\
) else (
    set builddir=%dir%
)

rem Setup the environment
call "%dir%env.bat" %vs_platf%

if '%kill%'=='true' (
    msbuild "%dir%kill_python.vcxproj" %verbose% /p:Configuration=%conf% /p:Platform=%platf% && "%builddir%kill_python%dbg_ext%.exe"
)

set externals_dir=%dir%..\externals
if '%build_tkinter%'=='true' (
    if '%platf%'=='x64' (
        set tcltkdir=%externals_dir%\tcltk64
        set machine=AMD64
    ) else (
        set tcltkdir=%externals_dir%\tcltk
        set machine=IX86
    )
    if '%conf%'=='Debug' (
        set tcl_dbg_ext=g
        set debug_flag=1
        set options=symbols
    ) else (
        set tcl_dbg_ext=
        set debug_flag=0
        set options=
    )
    set tcldir=%externals_dir%\tcl-8.6.1.0
    set tkdir=%externals_dir%\tk-8.6.1.0
    set tixdir=%externals_dir%\tix-8.4.3.4
)
if '%build_tkinter%'=='true' (
    if not exist "%tcltkdir%\bin\tcl86t%tcl_dbg_ext%.dll" (
        @rem all and install need to be separate invocations, otherwise nmakehlp is not found on install
        pushd "%tcldir%\win"
        nmake -f makefile.vc MACHINE=%machine% OPTS=%options% INSTALLDIR="%tcltkdir%" clean all
        nmake -f makefile.vc MACHINE=%machine% OPTS=%options% INSTALLDIR="%tcltkdir%" install-binaries install-libraries
        popd
    )
    if not exist "%builddir%tcl86t%tcl_dbg_ext%.dll" (
        xcopy "%tcltkdir%\bin\tcl86t%tcl_dbg_ext%.dll" "%builddir%"
    )

    if not exist "%tcltkdir%\bin\tk86t%tcl_dbg_ext%.dll" (
        pushd "%tkdir%\win"
        nmake -f makefile.vc MACHINE=%machine% OPTS=%options% INSTALLDIR="%tcltkdir%" TCLDIR="%tcldir%" clean
        nmake -f makefile.vc MACHINE=%machine% OPTS=%options% INSTALLDIR="%tcltkdir%" TCLDIR="%tcldir%" all
        nmake -f makefile.vc MACHINE=%machine% OPTS=%options% INSTALLDIR="%tcltkdir%" TCLDIR="%tcldir%" install-binaries install-libraries
        popd
    )
    if not exist "%builddir%tk86t%tcl_dbg_ext%.dll" (
        xcopy "%tcltkdir%\bin\tk86t%tcl_dbg_ext%.dll" "%builddir%"
    )

    if not exist "%tcltkdir%\lib\tix8.4.3\tix84%tcl_dbg_ext%.dll" (
        pushd "%tixdir%\win"
        nmake -f python.mak DEBUG=%debug_flag% MACHINE=%machine% TCL_DIR="%tcldir%" TK_DIR="%tkdir%" INSTALL_DIR="%tcltkdir%" clean
        nmake -f python.mak DEBUG=%debug_flag% MACHINE=%machine% TCL_DIR="%tcldir%" TK_DIR="%tkdir%" INSTALL_DIR="%tcltkdir%" all
        nmake -f python.mak DEBUG=%debug_flag% MACHINE=%machine% TCL_DIR="%tcldir%" TK_DIR="%tkdir%" INSTALL_DIR="%tcltkdir%" install
        popd
    )
)

rem Call on MSBuild to do the work, echo the command.
rem Passing %1-9 is not the preferred option, but argument parsing in
rem batch is, shall we say, "lackluster"
echo on
msbuild "%dir%pcbuild.sln" /t:%target% %parallel% %verbose% /p:Configuration=%conf% /p:Platform=%platf% %1 %2 %3 %4 %5 %6 %7 %8 %9
