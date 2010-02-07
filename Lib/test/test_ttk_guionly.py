import os
import unittest
from test import test_support

# Skip this test if _tkinter wasn't built.
test_support.import_module('_tkinter')

import ttk
from _tkinter import TclError

try:
    ttk.Button()
except TclError, msg:
    # assuming ttk is not available
    raise unittest.SkipTest("ttk not available: %s" % msg)

this_dir = os.path.dirname(os.path.abspath(__file__))
lib_tk_test = os.path.abspath(os.path.join(this_dir, os.path.pardir,
    'lib-tk', 'test'))

with test_support.DirsOnSysPath(lib_tk_test):
    import runtktests

def test_main(enable_gui=False):
    if enable_gui:
        if test_support.use_resources is None:
            test_support.use_resources = ['gui']
        elif 'gui' not in test_support.use_resources:
            test_support.use_resources.append('gui')

    with test_support.DirsOnSysPath(lib_tk_test):
        test_support.run_unittest(
            *runtktests.get_tests(text=False, packages=['test_ttk']))

if __name__ == '__main__':
    test_main(enable_gui=True)
