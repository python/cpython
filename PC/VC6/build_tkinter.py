import os
import sys
import subprocess

TCL_MAJOR = 8
TCL_MINOR = 5
TCL_PATCH = 2

TIX_MAJOR = 8
TIX_MINOR = 4
TIX_PATCH = 3

def abspath(name):
    par = os.path.pardir
    return os.path.abspath(os.path.join(__file__, par, par, par, par, name))

TCL_DIR = abspath("tcl%d.%d.%d" % (TCL_MAJOR, TCL_MINOR, TCL_PATCH))
TK_DIR  = abspath("tk%d.%d.%d"  % (TCL_MAJOR, TCL_MINOR, TCL_PATCH))
TIX_DIR = abspath("tix%d.%d.%d" % (TIX_MAJOR, TIX_MINOR, TIX_PATCH))
OUT_DIR = abspath("tcltk")

def have_args(*a):
    return any(s in sys.argv[1:] for s in a)

def enter(dir):
    os.chdir(os.path.join(dir, "win"))

def main():
    debug = have_args("-d", "--debug")
    clean = have_args("clean")
    install = have_args("install")
    tcl = have_args("tcl")
    tk = have_args("tk")
    tix = have_args("tix")
    if not(tcl) and not(tk) and not(tix):
        tcl = tk = tix = True

    def nmake(makefile, *a):
        args = ["nmake", "/nologo", "/f", makefile, "DEBUG=%d" % debug]
        args.extend(a)
        subprocess.check_call(args)

    if tcl:
        enter(TCL_DIR)
        def nmake_tcl(*a):
            nmake("makefile.vc", *a)
        if clean:
            nmake_tcl("clean")
        elif install:
            nmake_tcl("install", "INSTALLDIR=" + OUT_DIR)
        else:
            nmake_tcl()

    if tk:
        enter(TK_DIR)
        def nmake_tk(*a):
            nmake("makefile.vc", "TCLDIR=" + TCL_DIR, *a)
        if clean:
            nmake_tk("clean")
        elif install:
            nmake_tk("install", "INSTALLDIR=" + OUT_DIR)
        else:
            nmake_tk()

    if tix:
        enter(TIX_DIR)
        def nmake_tix(*a):
            nmake("python.mak",
                  "TCL_MAJOR=%d" % TCL_MAJOR,
                  "TCL_MINOR=%d" % TCL_MINOR,
                  "TCL_PATCH=%d" % TCL_PATCH,
                  "MACHINE=IX86", *a)
        if clean:
            nmake_tix("clean")
        elif install:
            nmake_tix("install", "INSTALL_DIR=" + OUT_DIR)
        else:
            nmake_tix()

if __name__ == '__main__':
    main()
