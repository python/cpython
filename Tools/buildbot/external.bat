@rem Fetches (and builds if necessary) external dependencies

@rem Assume we start inside the Python source directory
call "Tools\buildbot\external-common.bat"
call "%VS90COMNTOOLS%\vsvars32.bat"

if not exist tcltk\bin\tcl85g.dll (
    @rem all and install need to be separate invocations, otherwise nmakehlp is not found on install
    cd tcl-8.5.15.0\win
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 DEBUG=1 INSTALLDIR=..\..\tcltk clean all
    nmake -f makefile.vc DEBUG=1 INSTALLDIR=..\..\tcltk install
    cd ..\..
)

if not exist tcltk\bin\tk85g.dll (
    cd tk-8.5.15.0\win
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.5.15.0 clean
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.5.15.0 all
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.5.15.0 install
    cd ..\..
)

if not exist tcltk\lib\tix8.4.3\tix84g.dll (
    cd tix-8.4.3.5\win
    nmake -f python.mak DEBUG=1 MACHINE=IX86 COMPILERFLAGS=-DWINVER=0x0500 TCL_DIR=..\..\tcl-8.5.15.0 TK_DIR=..\..\tk-8.5.15.0 INSTALL_DIR=..\..\tcltk clean
    nmake -f python.mak DEBUG=1 MACHINE=IX86 COMPILERFLAGS=-DWINVER=0x0500 TCL_DIR=..\..\tcl-8.5.15.0 TK_DIR=..\..\tk-8.5.15.0 INSTALL_DIR=..\..\tcltk all
    nmake -f python.mak DEBUG=1 MACHINE=IX86 COMPILERFLAGS=-DWINVER=0x0500 TCL_DIR=..\..\tcl-8.5.15.0 TK_DIR=..\..\tk-8.5.15.0 INSTALL_DIR=..\..\tcltk install
    cd ..\..
)
