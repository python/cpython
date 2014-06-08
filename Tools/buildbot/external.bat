@rem Fetches (and builds if necessary) external dependencies

@rem Assume we start inside the Python source directory
call "Tools\buildbot\external-common.bat"
call "%VS100COMNTOOLS%\vsvars32.bat"

if not exist tcltk\bin\tcl86tg.dll (
    @rem all and install need to be separate invocations, otherwise nmakehlp is not found on install
    cd tcl-8.6.1.0\win
    nmake -f makefile.vc OPTS=symbols INSTALLDIR=..\..\tcltk clean core shell dlls
    nmake -f makefile.vc OPTS=symbols INSTALLDIR=..\..\tcltk install-binaries install-libraries
    cd ..\..
)

if not exist tcltk\bin\tk86tg.dll (
    cd tk-8.6.1.0\win
    nmake -f makefile.vc OPTS=symbols INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.6.1.0 clean
    nmake -f makefile.vc OPTS=symbols INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.6.1.0 all
    nmake -f makefile.vc OPTS=symbols INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.6.1.0 install-binaries install-libraries
    cd ..\..
)

if not exist tcltk\lib\tix8.4.3\tix84g.dll (
    cd tix-8.4.3.4\win
    nmake -f python.mak DEBUG=1 MACHINE=IX86 TCL_DIR=..\..\tcl-8.6.1.0 TK_DIR=..\..\tk-8.6.1.0 INSTALL_DIR=..\..\tcltk clean
    nmake -f python.mak DEBUG=1 MACHINE=IX86 TCL_DIR=..\..\tcl-8.6.1.0 TK_DIR=..\..\tk-8.6.1.0 INSTALL_DIR=..\..\tcltk all
    nmake -f python.mak DEBUG=1 MACHINE=IX86 TCL_DIR=..\..\tcl-8.6.1.0 TK_DIR=..\..\tk-8.6.1.0 INSTALL_DIR=..\..\tcltk install
    cd ..\..
)
