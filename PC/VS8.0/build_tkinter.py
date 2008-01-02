"""Script to compile the dependencies of _tkinter

Copyright (c) 2007 by Christian Heimes <christian@cheimes.de>

Licensed to PSF under a Contributor Agreement.
"""

import os
import sys
import shutil

here = os.path.abspath(os.path.dirname(__file__))
par = os.path.pardir

if 1:
    TCL = "tcl8.4.16"
    TK = "tk8.4.16"
    TIX = "tix-8.4.0"
else:
    TCL = "tcl8.5b3"
    TK = "tcl8.5b3"
    TIX = "Tix8.4.2"

ROOT = os.path.abspath(os.path.join(here, par, par, par))
# Windows 2000 compatibility: WINVER 0x0500
# http://msdn2.microsoft.com/en-us/library/aa383745.aspx
NMAKE = "nmake /nologo /f %s COMPILERFLAGS=-DWINVER=0x0500 %s %s"

def nmake(makefile, command="", **kw):
    defines = ' '.join(k+'='+v for k, v in kw.items())
    cmd = NMAKE % (makefile, defines, command)
    print("\n\n"+cmd+"\n")
    if os.system(cmd) != 0:
        raise RuntimeError(cmd)

def build(platform, clean):
    if platform == "Win32":
        dest = os.path.join(ROOT, "tcltk")
        machine = "X86"
    elif platform == "x64":
        dest = os.path.join(ROOT, "tcltk64")
        machine = "X64"
    else:
        raise ValueError(platform)

    # TCL
    tcldir = os.path.join(ROOT, TCL)
    if 1:
        os.chdir(os.path.join(tcldir, "win"))
        if clean:
            nmake("makefile.vc", "clean")
        nmake("makefile.vc")
        nmake("makefile.vc", "install", INSTALLDIR=dest)

    # TK
    if 1:
        os.chdir(os.path.join(ROOT, TK, "win"))
        if clean:
            nmake("makefile.vc", "clean", TCLDIR=tcldir)
        nmake("makefile.vc", TCLDIR=tcldir)
        nmake("makefile.vc", "install", TCLDIR=tcldir, INSTALLDIR=dest)

    # TIX
    if 1:
        # python9.mak is available at http://svn.python.org
        os.chdir(os.path.join(ROOT, TIX, "win"))
        if clean:
            nmake("python9.mak", "clean")
        nmake("python9.mak", MACHINE=machine)
        nmake("python9.mak", "install")

def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("Win32", "x64"):
        print("%s Win32|x64" % sys.argv[0])
        sys.exit(1)

    if "-c" in sys.argv:
        clean = True
    else:
        clean = False

    build(sys.argv[1], clean)


if __name__ == '__main__':
    main()
