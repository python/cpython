import sys, os
v = os.path.join(sys.prefix, "tcl", "tcl8.3")
if os.path.exists(os.path.join(v, "init.tcl")):
    os.environ["TCL_LIBRARY"] = v
