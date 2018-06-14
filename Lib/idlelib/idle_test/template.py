"Test , coverage %."

import unittest
from test.support import requires
from tkinter import Tk

import idlelib.

class Test(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def test_init(self):
        self.assert


if __name__ == '__main__':
    unittest.main(verbosity=2)
