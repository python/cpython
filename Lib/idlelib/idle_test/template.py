"Test , coverage %."

from idlelib import
import unittest
from test.support import requires
from tkinter import Tk


class Test(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_init(self):
        self.assert


if __name__ == '__main__':
    unittest.main(verbosity=2)
