"""Script to compile the dependencies of _tkinter

Copyright (c) 2007 by Christian Heimes <christian@cheimes.de>

Licensed to PSF under a Contributor Agreement.
"""

import os
import sys
import shutil

here = os.path.abspath(os.path.dirname(__file__))
par = os.path.pardir

#TCL = "tcl8.4.16"
#TIX = "Tix8.4.2"
#TK = "tk8.4.16"
TCL = "tcl8.4.12"
TIX = "Tix8.4.0"
TK = "tk8.4.12"
ROOT = os.path.abspath(os.path.join(here, par, par))
NMAKE = "nmake /nologo "

def system(cmd):
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
    if True:
        os.chdir(os.path.join(tcldir, "win"))
        if clean:
            system(NMAKE + "/f makefile.vc clean")
        system(NMAKE + "/f makefile.vc")
        system(NMAKE + "/f makefile.vc INSTALLDIR=%s install" % dest)

    # TK
    if True:
        os.chdir(os.path.join(ROOT, TK, "win"))
        if clean:
            system(NMAKE + "/f makefile.vc clean")
        system(NMAKE + "/f makefile.vc TCLDIR=%s" % tcldir)
        system(NMAKE + "/f makefile.vc TCLDIR=%s INSTALLDIR=%s install" %
            (tcldir, dest))

    # TIX
    if True:
        os.chdir(os.path.join(ROOT, TIX, "win"))
        if clean:
            system(NMAKE + "/f makefile.vc clean")
        system(NMAKE + "/f makefile.vc MACHINE=%s" % machine)
        system(NMAKE + "/f makefile.vc INSTALL_DIR=%s install" % dest)


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
