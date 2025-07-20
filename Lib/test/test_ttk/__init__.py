import os.path
import unittest
from test import support
from test.support import import_helper


if support.check_sanitizer(address=True, memory=True):
    raise unittest.SkipTest("Tests involving libX11 can SEGFAULT on ASAN/MSAN builds")

# Skip this test if _tkinter wasn't built.
import_helper.import_module('_tkinter')

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


def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
