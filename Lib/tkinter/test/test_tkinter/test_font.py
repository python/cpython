import unittest
import tkinter
from tkinter import font
from test.support import requires, run_unittest

requires('gui')

class FontTest(unittest.TestCase):
    def test_font_eq(self):
        font1 = font.nametofont("system")
        font2 = font.nametofont("system")
        self.assertIsNot(font1, font2)
        self.assertEqual(font1, font2)
        self.assertNotEqual(font1, font1.copy())
        self.assertNotEqual(font1, 0)

tests_gui = (FontTest, )

if __name__ == "__main__":
    run_unittest(*tests_gui)
