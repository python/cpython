"""Utility which tries to locate the Tcl/Tk 8.0 DLLs on Windows.

This is a no-op on other platforms.
"""

# Error messages we may spit out

NO_TCL_MESSAGE = """\
WHOOPS!  I can't find a Tcl/Tk 8.0 installation anywhere.
Please make sure that Tcl.Tk 8.0 is installed and that the PATH
environment variable is set to include the Tcl/bin directory
(or wherever TK80.DLL and TCL80.DLL are installed).
If you don't know how to fix this, consider searching the Python FAQ
for the error you get; post to the comp.lang.python if all else fails.
Read the source file FixTk.py for details.
"""

NO_TKINTER_MESSAGE = """\
WHOOPS!  Even though I think I have found a Tcl/Tk 8.0 installation,
I can't seem to import the _tkinter extension module.
I get the following exception:
    ImportError: %s
If you don't know how to fix this, consider searching the Python FAQ
for the error you get; post to the comp.lang.python if all else fails.
Read the source file FixTk.py for details.
"""

import sys
if sys.platform == "win32":
    try:
        import _tkinter
    except ImportError:
        import os
        try:
            path = os.environ['PATH']
        except KeyError:
            path = ""
        python_exe = sys.executable
        python_dir = os.path.dirname(python_exe)
        program_files = os.path.dirname(python_dir)
        def tclcheck(dir):
            for dll in "tcl80.dll", "tk80.dll", "tclpip80.dll":
                if not os.path.isfile(os.path.join(dir, dll)):
                    return 0
            return 1
        for tcldir in [program_files, "\\Program files", "\\",
                       "C:\\Program Files", "D:\\Program Files"]:
            tcldir = os.path.join(tcldir, "Tcl", "bin")
            if tclcheck(tcldir):
                break
        else:
            tcldir = None
        if not tcldir:
            sys.stderr.write(NO_TCL_MESSAGE)
        else:
            if path and path[-1] != os.pathsep:
                path = path + os.pathsep
            path = path + tcldir
            os.environ["PATH"] = path
            os.putenv("PATH", path)
            try:
                import _tkinter
            except ImportError, message:
                sys.stderr.write(NO_TKINTER_MESSAGE % str(message))
