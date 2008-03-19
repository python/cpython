@rem Fetches (and builds if necessary) external dependencies

@rem Assume we start inside the Python source directory
call "Tools\buildbot\external-common.bat"
call "%VS90COMNTOOLS%\vsvars32.bat"

if not exist tcltk\bin\tcl84g.dll (
    cd tcl-8.4.18.2\win
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 DEBUG=1 INSTALLDIR=..\..\tcltk clean all install
    cd ..\..
)

if not exist tcltk\bin\tk84g.dll (
    cd tk-8.4.18.1\win    
    nmake -f makefile.vc COMPILERFLAGS=-DWINVER=0x0500 DEBUG=1 INSTALLDIR=..\..\tcltk TCLDIR=..\..\tcl-8.4.18.2 clean all install
    cd ..\..
)
