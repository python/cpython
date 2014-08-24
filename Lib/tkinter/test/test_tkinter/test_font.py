import unittest
import tkinter
from tkinter import font
from test.support import requires, run_unittest
from tkinter.test.support import AbstractTkTest

requires('gui')

class FontTest(AbstractTkTest, unittest.TestCase):

    def test_font_eq(self):
        fontname = "TkDefaultFont"
        try:
            f = font.Font(root=self.root, name=fontname, exists=True)
        except tkinter._tkinter.TclError:
            f = font.Font(root=self.root, name=fontname, exists=False)
        font1 = font.Font(root=self.root, name=fontname, exists=True)
        font2 = font.Font(root=self.root, name=fontname, exists=True)
        self.assertIsNot(font1, font2)
        self.assertEqual(font1, font2)
        self.assertNotEqual(font1, font1.copy())
        self.assertNotEqual(font1, 0)

tests_gui = (FontTest, )

if __name__ == "__main__":
    run_unittest(*tests_gui)
