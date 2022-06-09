import unittest
from test import support
from test.support import import_helper
from test.support import check_sanitizer

if check_sanitizer(address=True, memory=True):
    raise unittest.SkipTest("Tests involvin libX11 can SEGFAULT on ASAN/MSAN builds")

# Skip test if _tkinter wasn't built.
import_helper.import_module('_tkinter')

# Skip test if tk cannot be initialized.
support.requires('gui')

def load_tests(loader, tests, pattern):
    return loader.discover('tkinter.test.test_tkinter')


if __name__ == '__main__':
    unittest.main()
