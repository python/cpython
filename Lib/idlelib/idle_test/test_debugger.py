"Test debugger, coverage 19%"

from idlelib import debugger
import unittest
from unittest import mock
from test.support import requires
#requires('gui')
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


@mock.patch('bdb.Bdb')
class IdbTest(unittest.TestCase):
    def test_init(self, bdb):
        import pdb; pdb.set_trace()
        bdb.__init__.assertCalled()

# Other classes are Idb, Debugger, and StackViewer.

if __name__ == '__main__':
    unittest.main(verbosity=2)
