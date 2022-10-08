import os.path
import unittest
from test import support
from test.support import import_helper


if support.check_sanitizer(address=True, memory=True):
    raise unittest.SkipTest("Tests involving libX11 can SEGFAULT on ASAN/MSAN builds")

# Skip test if _tkinter wasn't built.
import_helper.import_module('_tkinter')

# Skip test if tk cannot be initialized.
support.requires('gui')


def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
