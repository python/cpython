@rem Fetches (and builds if necessary) external dependencies

@rem Assume we start inside the Python source directory
call "Tools\buildbot\external-common.bat"
call "%VS90COMNTOOLS%\vsvars32.bat"

if not exist tcltk\bin\tcl85g.dll (
    @rem all and install need to be separate invocations, otherwise nmakehlp is not found on install
    cd tcl-8.5.2.1\win
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 DEBUG=1 INSTALLDIR=..\..\tcltk clean all 
    nmake -f makefile.vc DEBUG=1 INSTALLDIR=..\..\tcltk install
    cd ..\..
)

if not exist tcltk\bin\tk85g.dll (
    cd tk-8.5.2.0\win    
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.5.2.1 clean
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.5.2.1 all
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 OPTS=noxp DEBUG=1 INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.5.2.1 install
    cd ..\..
)
