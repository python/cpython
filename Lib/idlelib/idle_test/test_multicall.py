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

    def test_event_delete_unbound_sequence(self):
        # gh-89360: deleting a sequence that was not added to a virtual
        # event is ignored instead of raising ValueError.
        mctext = self.mc(self.root)
        mctext.event_add('<<tester>>', '<Control-Key-a>')
        info = mctext.event_info('<<tester>>')
        self.assertEqual(len(info), 1)

        # A different sequence, never added: a no-op, not an error.
        mctext.event_delete('<<tester>>', '<Control-Key-b>')
        self.assertEqual(mctext.event_info('<<tester>>'), info)

        # The added sequence can still be deleted normally.
        mctext.event_delete('<<tester>>', '<Control-Key-a>')
        self.assertNotIn(info[0], mctext.event_info('<<tester>>'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
