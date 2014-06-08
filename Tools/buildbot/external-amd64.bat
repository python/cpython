@rem Fetches (and builds if necessary) external dependencies

@rem Assume we start inside the Python source directory
call "Tools\buildbot\external-common.bat"
call "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64

if not exist tcltk64\bin\tcl85g.dll (
    cd tcl-8.5.15.0\win
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 DEBUG=1 MACHINE=AMD64 INSTALLDIR=..\..\tcltk64 clean all
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 DEBUG=1 MACHINE=AMD64 INSTALLDIR=..\..\tcltk64 install
    cd ..\..
)

if not exist tcltk64\bin\tk85g.dll (
    cd tk-8.5.15.0\win
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 MACHINE=AMD64 INSTALLDIR=..\..\tcltk64 TCLDIR=..\..\tcl-8.5.15.0 clean
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 MACHINE=AMD64 INSTALLDIR=..\..\tcltk64 TCLDIR=..\..\tcl-8.5.15.0 all
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 MACHINE=AMD64 INSTALLDIR=..\..\tcltk64 TCLDIR=..\..\tcl-8.5.15.0 install
    cd ..\..
)

if not exist tcltk64\lib\tix8.4.3\tix84g.dll (
    cd tix-8.4.3.5\win
    nmake -f python.mak DEBUG=1 MACHINE=AMD64 COMPILERFLAGS=-DWINVER=0x0500 TCL_DIR=..\..\tcl-8.5.15.0 TK_DIR=..\..\tk-8.5.15.0 INSTALL_DIR=..\..\tcltk64 clean
    nmake -f python.mak DEBUG=1 MACHINE=AMD64 COMPILERFLAGS=-DWINVER=0x0500 TCL_DIR=..\..\tcl-8.5.15.0 TK_DIR=..\..\tk-8.5.15.0 INSTALL_DIR=..\..\tcltk64 all
    nmake -f python.mak DEBUG=1 MACHINE=AMD64 COMPILERFLAGS=-DWINVER=0x0500 TCL_DIR=..\..\tcl-8.5.15.0 TK_DIR=..\..\tk-8.5.15.0 INSTALL_DIR=..\..\tcltk64 install
    cd ..\..
)