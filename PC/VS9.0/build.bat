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
rem  -k  Attempt to kill any running Pythons before building

setlocal
set platf=Win32
set vs_platf=x86
set conf=Release
set target=
set dir=%~dp0
set kill=
set build_tkinter=

:CheckOpts
if '%1'=='-c' (set conf=%2) & shift & shift & goto CheckOpts
if '%1'=='-p' (set platf=%2) & shift & shift & goto CheckOpts
if '%1'=='-r' (set target=/rebuild) & shift & goto CheckOpts
if '%1'=='-t' (
    if '%2'=='Clean' (set target=/clean) & shift & shift & goto CheckOpts
    if '%2'=='Rebuild' (set target=/rebuild) & shift & shift & goto CheckOpts
    if '%2'=='Build' (set target=) & shift & shift & goto CheckOpts
    echo.Unknown target: %2 & goto :eof
)
if '%1'=='-d' (set conf=Debug) & shift & goto CheckOpts
if '%1'=='-e' call "%dir%..\..\PCbuild\get_externals.bat" & (set build_tkinter=true) & shift & goto CheckOpts
if '%1'=='-k' (set kill=true) & shift & goto CheckOpts

if '%conf%'=='Debug' (set dbg_ext=_d) else (set dbg_ext=)
if '%platf%'=='x64' (
    set vs_platf=x86_amd64
    set builddir=%dir%amd64\
) else (
    set builddir=%dir%
)
rem Can't use builddir until we're in a new command...
if '%platf%'=='x64' (
    rem Needed for buliding OpenSSL
    set HOST_PYTHON=%builddir%python%dbg_ext%.exe
)

rem Setup the environment
call "%dir%env.bat" %vs_platf%

if '%kill%'=='true' (
    vcbuild "%dir%kill_python.vcproj" "%conf%|%platf%" && "%builddir%kill_python%dbg_ext%.exe"
)

set externals_dir=%dir%..\..\externals
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
    ) else (
        set tcl_dbg_ext=
        set debug_flag=0
    )
    set tcldir=%externals_dir%\tcl-8.5.19.0
    set tkdir=%externals_dir%\tk-8.5.19.0
    set tixdir=%externals_dir%\tix-8.4.3.5
)
if '%build_tkinter%'=='true' (
    if not exist "%tcltkdir%\bin\tcl85%tcl_dbg_ext%.dll" (
        @rem all and install need to be separate invocations, otherwise nmakehlp is not found on install
        pushd "%tcldir%\win"
        nmake -f makefile.vc MACHINE=%machine% DEBUG=%debug_flag% INSTALLDIR="%tcltkdir%" clean all
        nmake -f makefile.vc MACHINE=%machine% DEBUG=%debug_flag% INSTALLDIR="%tcltkdir%" install
        popd
    )

    if not exist "%tcltkdir%\bin\tk85%tcl_dbg_ext%.dll" (
        pushd "%tkdir%\win"
        nmake -f makefile.vc MACHINE=%machine% DEBUG=%debug_flag% INSTALLDIR="%tcltkdir%" TCLDIR="%tcldir%" clean
        nmake -f makefile.vc MACHINE=%machine% DEBUG=%debug_flag% INSTALLDIR="%tcltkdir%" TCLDIR="%tcldir%" all
        nmake -f makefile.vc MACHINE=%machine% DEBUG=%debug_flag% INSTALLDIR="%tcltkdir%" TCLDIR="%tcldir%" install
        popd
    )

    if not exist "%tcltkdir%\lib\tix8.4.3\tix84%tcl_dbg_ext%.dll" (
        pushd "%tixdir%\win"
        nmake -f python.mak DEBUG=%debug_flag% MACHINE=%machine% TCL_DIR="%tcldir%" TK_DIR="%tkdir%" INSTALL_DIR="%tcltkdir%" clean
        nmake -f python.mak DEBUG=%debug_flag% MACHINE=%machine% TCL_DIR="%tcldir%" TK_DIR="%tkdir%" INSTALL_DIR="%tcltkdir%" all
        nmake -f python.mak DEBUG=%debug_flag% MACHINE=%machine% TCL_DIR="%tcldir%" TK_DIR="%tkdir%" INSTALL_DIR="%tcltkdir%" install
        popd
    )
)

rem Call on VCBuild to do the work, echo the command.
rem Passing %1-9 is not the preferred option, but argument parsing in
rem batch is, shall we say, "lackluster"
echo on
vcbuild "%dir%pcbuild.sln" %target% "%conf%|%platf%" %1 %2 %3 %4 %5 %6 %7 %8 %9
