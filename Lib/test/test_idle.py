import unittest
from test.support import import_module

# Skip test if _thread or _tkinter wasn't built, if idlelib is missing,
# or if tcl/tk is not the 8.5+ needed for ttk widgets.
import_module('threading')  # imported by PyShell, imports _thread
tk = import_module('tkinter')  # imports _tkinter
if tk.TkVersion < 8.5:
    raise unittest.SkipTest("IDLE requires tk 8.5 or later.")
idlelib = import_module('idlelib')

# Before test imports, tell IDLE to avoid changing the environment.
idlelib.testing = True

# unittest.main and test.libregrtest.runtest.runtest_inner
# call load_tests, when present, to discover tests to run.
from idlelib.idle_test import load_tests

if __name__ == '__main__':
    tk.NoDefaultRoot()
    unittest.main(exit=False)
    tk._support_default_root = 1
    tk._default_root = None
