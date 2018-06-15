"Test colorizer, coverage 25%."

from idlelib import colorizer
from test.support import requires
from tkinter import Tk, Text
import unittest


class FunctionTest(unittest.TestCase):

    def test_any(self):
        self.assertTrue(colorizer.any('test', ('a', 'b')))

    def test_make_pat(self):
        self.assertTrue(colorizer.make_pat())


class ColorConfigTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.text = Text(cls.root)

    @classmethod
    def tearDownClass(cls):
        del cls.text
        cls.root.destroy()
        del cls.root

    def test_colorizer(self):
        colorizer.color_config(self.text)


class ColorDelegatorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def test_colorizer(self):
        colorizer.ColorDelegator()


if __name__ == '__main__':
    unittest.main(verbosity=2)
