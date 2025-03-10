import os.path
import unittest

from test.support import (
    check_sanitizer,
    import_helper,
    load_package_tests,
    requires,
    check_tk_version,
    )


if check_sanitizer(address=True, memory=True):
    # See gh-90791 for details
    raise unittest.SkipTest("Tests involving libX11 can SEGFAULT on ASAN/MSAN builds")

# Skip test if _tkinter wasn't built.
import_helper.import_module('_tkinter')

# Skip test if tk cannot be initialized.
requires('gui')

# Skip test if tk version is too new.
check_tk_version()


import tkinter
tcl = tkinter.Tcl()
major, minor, micro = tcl.call("info", "patchlevel").split(".")
version = f"{int(major):02d}{int(minor):02d}{int(micro):02d}"
if version > "080699":
    raise unittest.SkipTest(f"Tk version {major}.{minor}.{micro} not supported")

def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
