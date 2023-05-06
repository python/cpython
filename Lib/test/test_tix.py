import sys
import unittest
from test import support
from test.support import import_helper
from test.support import check_sanitizer

if check_sanitizer(address=True, memory=True):
    raise unittest.SkipTest("Tests involving libX11 can SEGFAULT on ASAN/MSAN builds")


# Skip this test if the _tkinter module wasn't built.
_tkinter = import_helper.import_module('_tkinter')

# Skip test if tk cannot be initialized.
support.requires('gui')

# Suppress the deprecation warning
tix = import_helper.import_module('tkinter.tix', deprecated=True)
from tkinter import TclError


class TestTix(unittest.TestCase):

    def setUp(self):
        try:
            self.root = tix.Tk()
        except TclError:
            if sys.platform.startswith('win'):
                self.fail('Tix should always be available on Windows')
            self.skipTest('Tix not available')
        else:
            self.addCleanup(self.root.destroy)

    def test_tix_available(self):
        # this test is just here to make setUp run
        pass


if __name__ == '__main__':
    unittest.main()
