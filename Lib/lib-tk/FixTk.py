import sys, os, _tkinter
ver = str(_tkinter.TCL_VERSION)
v = os.path.join(sys.prefix, "tcl", "tcl"+ver)
if os.path.exists(os.path.join(v, "init.tcl")):
    os.environ["TCL_LIBRARY"] = v
