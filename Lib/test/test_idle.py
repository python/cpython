import unittest
from test.support import import_module
from test.support import check_sanitizer

if check_sanitizer(address=True, memory=True):
    raise unittest.SkipTest("Tests involvin libX11 can SEGFAULT on ASAN/MSAN builds")

# Skip test_idle if _tkinter wasn't built, if tkinter is missing,
# if tcl/tk is not the 8.5+ needed for ttk widgets,
# or if idlelib is missing (not installed).
tk = import_module('tkinter')  # Also imports _tkinter.
if tk.TkVersion < 8.5:
    raise unittest.SkipTest("IDLE requires tk 8.5 or later.")
idlelib = import_module('idlelib')

# Before importing and executing more of idlelib,
# tell IDLE to avoid changing the environment.
idlelib.testing = True

# Unittest.main and test.libregrtest.runtest.runtest_inner
# call load_tests, when present here, to discover tests to run.
from idlelib.idle_test import load_tests

if __name__ == '__main__':
    tk.NoDefaultRoot()
    unittest.main(exit=False)
    tk._support_default_root = True
    tk._default_root = None
