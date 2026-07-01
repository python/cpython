"Test multicall, coverage 33%."

from idlelib import multicall
import io
import unittest
from test.support import requires
from tkinter import Tk, Text
from unittest import mock


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
##        for id in cls.root.after_info():
##            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_creator(self):
        mc = self.mc
        self.assertIs(multicall._multicall_dict[Text], mc)
        self.assertIsSubclass(mc, Text)
        mc2 = multicall.MultiCallCreator(Text)
        self.assertIs(mc, mc2)

    def test_init(self):
        mctext = self.mc(self.root)
        self.assertIsInstance(mctext._MultiCall__binders, list)

    def test_yview(self):
        # Added for tree.wheel_event
        # (it depends on yview to not be overridden)
        mc = self.mc
        self.assertIs(mc.yview, Text.yview)
        mctext = self.mc(self.root)
        self.assertIs(mctext.yview.__func__, Text.yview)

    def test_invalid_binding(self):
        # gh-55646: an invalid key binding must not crash IDLE.
        mctext = self.mc(self.root)
        # 'up' is not a keysym; it should be 'Up'.
        mctext.event_add('<<test-bad>>', '<Alt-Key-up>')
        with mock.patch('sys.stderr', new_callable=io.StringIO) as stderr:
            mctext.bind('<<test-bad>>', lambda e: None)  # Must not raise.
        self.assertIn('invalid key binding', stderr.getvalue())


if __name__ == '__main__':
    unittest.main(verbosity=2)
