"Test debugger_r, coverage 30%."

from idlelib import debugger_r
import unittest
from test.support import requires
from tkinter import Tk


class Test(unittest.TestCase):

##    @classmethod
##    def setUpClass(cls):
##        requires('gui')
##        cls.root = Tk()
##
##    @classmethod
##    def tearDownClass(cls):
##        cls.root.destroy()
##        del cls.root

    def test_init(self):
        self.assertTrue(True)  # Get coverage of import


# Classes GUIProxy, IdbAdapter, FrameProxy, CodeProxy, DictProxy,
# GUIAdapter, IdbProxy plus 7 module functions.

if __name__ == '__main__':
    unittest.main(verbosity=2)
