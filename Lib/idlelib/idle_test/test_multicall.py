"Test multicall, coverage 33%."

from idlelib import multicall
import unittest
from test.support import requires
from tkinter import Tk, Text


class MultiCallTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.mc = multicall.MultiCallCreator(Text)

    @classmethod
    def tearDownClass(cls):
        del cls.mc
        cls.root.update_idletasks()
##        for id in cls.root.tk.call('after', 'info'):
##            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_creator(self):
        mc = self.mc
        self.assertIs(multicall._multicall_dict[Text], mc)
        self.assertTrue(issubclass(mc, Text))
        mc2 = multicall.MultiCallCreator(Text)
        self.assertIs(mc, mc2)

    def test_init(self):
        mctext = self.mc(self.root)
        self.assertIsInstance(mctext._MultiCall__binders, list)


if __name__ == '__main__':
    unittest.main(verbosity=2)
