import sys, os

# Delay import _tkinter until we have set TCL_LIBRARY,
# so that Tcl_FindExecutable has a chance to locate its
# encoding directory.

# Unfortunately, we cannot know the TCL_LIBRARY directory
# if we don't know the tcl version, which we cannot find out
# without import Tcl. Fortunately, Tcl will itself look in
# <TCL_LIBRARY>\..\tcl<TCL_VERSION>, so anything close to
# the real Tcl library will do.

prefix = os.path.join(sys.prefix,"tcl")
# if this does not exist, no further search is needed
if os.path.exists(prefix):
    if not os.environ.has_key("TCL_LIBRARY"):
        for name in os.listdir(prefix):
            if name.startswith("tcl"):
                tcldir = os.path.join(prefix,name)
                if os.path.isdir(tcldir):
                    os.environ["TCL_LIBRARY"] = tcldir
    # Now set the other variables accordingly
    import _tkinter
    ver = str(_tkinter.TCL_VERSION)
    for t in "tk", "tix":
        key = t.upper() + "_LIBRARY"
        try:
            v = os.environ[key]
        except KeyError:
            v = os.path.join(sys.prefix, "tcl", t+ver)
            if os.path.exists(os.path.join(v, "tclIndex")):
                os.environ[key] = v
