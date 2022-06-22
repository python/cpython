'''Tests the IDLE application as part of the stdlib test suite.
Run IDLE tests alone with "python -m test.test_idle".
Starting with Python 3.6, IDLE requires tcl/tk 8.5 or later.

This package and its contained modules are subject to change and
any direct use is at your own risk.
'''
from os.path import dirname
from test import support
from test.support.import_helper import import_module


if support.check_sanitizer(address=True, memory=True):
    raise unittest.SkipTest("Tests involving libX11 can SEGFAULT on ASAN/MSAN builds")

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


def load_tests(loader, standard_tests, pattern):
    this_dir = dirname(__file__)
    top_dir = dirname(dirname(this_dir))
    package_tests = loader.discover(start_dir=this_dir, pattern='test*.py',
                                    top_level_dir=top_dir)
    standard_tests.addTests(package_tests)
    return standard_tests
