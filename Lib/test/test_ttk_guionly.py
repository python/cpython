import unittest
from test import support
from test.support import check_sanitizer

if check_sanitizer(address=True, memory=True):
    raise unittest.SkipTest("Tests involvin libX11 can SEGFAULT on ASAN/MSAN builds")

# Skip this test if _tkinter wasn't built.
support.import_module('_tkinter')

# Skip test if tk cannot be initialized.
support.requires('gui')

import tkinter
from _tkinter import TclError
from tkinter import ttk


def setUpModule():
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

def load_tests(loader, tests, pattern):
    return loader.discover('tkinter.test.test_ttk')


if __name__ == '__main__':
    unittest.main()
