import unittest
from test.support.import_helper import import_module
from test.support import check_sanitizer

if check_sanitizer(address=True, memory=True):
    raise unittest.SkipTest("Tests involvin libX11 can SEGFAULT on ASAN/MSAN builds")

# Skip test_idle if _tkinter, tkinter, or idlelib are missing.
tk = import_module('tkinter')  # Also imports _tkinter.
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
