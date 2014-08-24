import os
import unittest
from test import support

# Skip this test if _tkinter wasn't built.
support.import_module('_tkinter')

# Make sure tkinter._fix runs to set up the environment
tkinter = support.import_fresh_module('tkinter')

# Skip test if tk cannot be initialized.
support.requires('gui')

from _tkinter import TclError
from tkinter import ttk
from tkinter.test import runtktests

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
    support.run_unittest(
            *runtktests.get_tests(text=False, packages=['test_ttk']))

if __name__ == '__main__':
    test_main()
