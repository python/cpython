''' Test idlelib.scrolledlist.

Coverage: 39%
'''
from idlelib import scrolledlist
from test.support import requires
requires('gui')
import unittest
from tkinter import Tk


class ScrolledListTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root


    def test_init(self):
        scrolledlist.ScrolledList(self.root)


if __name__ == '__main__':
    unittest.main(verbosity=2)
