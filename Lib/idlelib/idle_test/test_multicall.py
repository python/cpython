"Test multicall, coverage 33%."

from idlelib import multicall
import unittest
from test.support import requires, captured_stderr
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

    def test_valid_binding(self):
        # A valid key binding must bind without a warning (cf. gh-55646).
        mctext = self.mc(self.root)
        with captured_stderr() as stderr:
            mctext.event_add('<<test-good>>', '<Control-Key-Up>')
            mctext.bind('<<test-good>>', lambda e: None)
        self.assertEqual(stderr.getvalue(), '')

    def test_invalid_triplet_binding(self):
        # gh-55646: '<Control-Key-up>' parses into a triplet on every platform,
        # so Tk rejects it in bind().
        mctext = self.mc(self.root)
        with captured_stderr() as stderr:
            mctext.event_add('<<test-bad>>', '<Control-Key-up>')  # Must not raise.
            mctext.bind('<<test-bad>>', lambda e: None)  # Must not raise.
        self.assertIn('invalid key binding', stderr.getvalue())

    def test_invalid_nontriplet_binding(self):
        # gh-55646: '<Foo-Key-Up>' has no valid modifier, so MultiCall does not
        # parse it and falls back to Tk's event_add, which rejects it.
        mctext = self.mc(self.root)
        with captured_stderr() as stderr:
            mctext.event_add('<<test-bad>>', '<Foo-Key-Up>')  # Must not raise.
            mctext.bind('<<test-bad>>', lambda e: None)  # Must not raise.
        self.assertIn('invalid key binding', stderr.getvalue())


if __name__ == '__main__':
    unittest.main(verbosity=2)
