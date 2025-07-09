"Test help, coverage 94%."

from idlelib import help
import unittest
from test.support import requires
requires('gui')
from os.path import abspath, dirname, join
from tkinter import Tk


class IdleDocTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        "By itself, this tests that file parsed without exception."
        cls.root = root = Tk()
        root.withdraw()
        cls.window = help.show_idlehelp(root)

    @classmethod
    def tearDownClass(cls):
        del cls.window
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_1window(self):
        self.assertIn('IDLE Doc', self.window.wm_title())

    def test_4text(self):
        text = self.window.frame.text
        self.assertEqual(text.get('1.0', '1.end'), ' IDLE — Python editor and shell ')


if __name__ == '__main__':
    unittest.main(verbosity=2)
