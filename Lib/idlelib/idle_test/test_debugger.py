''' Test idlelib.debugger.

Coverage: 19%
'''
from idlelib import debugger
from test.support import requires
requires('gui')
import unittest
from tkinter import Tk


class NameSpaceTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def test_init(self):
        debugger.NamespaceViewer(self.root, 'Test')


if __name__ == '__main__':
    unittest.main(verbosity=2)
