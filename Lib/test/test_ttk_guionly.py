import os
import unittest
from test import test_support

# Skip this test if _tkinter wasn't built or gui resource is not available.
test_support.import_module('_tkinter')
test_support.requires('gui')

this_dir = os.path.dirname(os.path.abspath(__file__))
lib_tk_test = os.path.abspath(os.path.join(this_dir, os.path.pardir,
    'lib-tk', 'test'))

with test_support.DirsOnSysPath(lib_tk_test):
    import runtktests

import Tkinter as tkinter
import ttk
from _tkinter import TclError

root = None
try:
    root = tkinter.Tk()
    button = ttk.Button(root)
    button.destroy()
    del button
except TclError as msg:
    # assuming ttk is not available
    raise unittest.SkipTest("ttk not available: %s" % msg)
finally:
    if root is not None:
        root.destroy()
    del root

def test_main():
    with test_support.DirsOnSysPath(lib_tk_test):
        test_support.run_unittest(
            *runtktests.get_tests(text=False, packages=['test_ttk']))

if __name__ == '__main__':
    test_main()
