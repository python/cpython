import sys, os, _tkinter

ver = str(_tkinter.TCL_VERSION)
for t in "tcl", "tk", "tix":
    key = t.upper() + "_LIBRARY"
    try:
        v = os.environ[key]
    except KeyError:
        v = os.path.join(sys.prefix, "tcl", t+ver)
        if os.path.exists(os.path.join(v, "tclIndex")):
            os.environ[key] = v
