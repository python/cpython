"Test config_key, coverage 75%"

from idlelib import config_key
from test.support import requires
import sys
import unittest
from tkinter import Tk
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Var, Mbox_func


class ValidationTest(unittest.TestCase):
    "Test validation methods: OK, KeysOK, bind_ok."

    class Validator(config_key.GetKeysDialog):
        def __init__(self, *args, **kwargs):
            config_key.GetKeysDialog.__init__(self, *args, **kwargs)
            class listKeysFinal:
                get = Func()
            self.listKeysFinal = listKeysFinal
        GetModifiers = Func()
        showerror = Mbox_func()

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        keylist = [['<Key-F12>'], ['<Control-Key-x>', '<Control-Key-X>']]
        cls.dialog = cls.Validator(
            cls.root, 'Title', '<<Test>>', keylist, _utest=True)

    @classmethod
    def tearDownClass(cls):
        cls.dialog.Cancel()
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.dialog, cls.root

    def setUp(self):
        self.dialog.showerror.message = ''
    # A test that needs a particular final key value should set it.
    # A test that sets a non-blank modifier list should reset it to [].

    def test_ok_empty(self):
        self.dialog.keyString.set(' ')
        self.dialog.OK()
        self.assertEqual(self.dialog.result, '')
        self.assertEqual(self.dialog.showerror.message, 'No key specified.')

    def test_ok_good(self):
        self.dialog.keyString.set('<Key-F11>')
        self.dialog.listKeysFinal.get.result = 'F11'
        self.dialog.OK()
        self.assertEqual(self.dialog.result, '<Key-F11>')
        self.assertEqual(self.dialog.showerror.message, '')

    def test_keys_no_ending(self):
        self.assertFalse(self.dialog.KeysOK('<Control-Shift'))
        self.assertIn('Missing the final', self.dialog.showerror.message)

    def test_keys_no_modifier_bad(self):
        self.dialog.listKeysFinal.get.result = 'A'
        self.assertFalse(self.dialog.KeysOK('<Key-A>'))
        self.assertIn('No modifier', self.dialog.showerror.message)

    def test_keys_no_modifier_ok(self):
        self.dialog.listKeysFinal.get.result = 'F11'
        self.assertTrue(self.dialog.KeysOK('<Key-F11>'))
        self.assertEqual(self.dialog.showerror.message, '')

    def test_keys_shift_bad(self):
        self.dialog.listKeysFinal.get.result = 'a'
        self.dialog.GetModifiers.result = ['Shift']
        self.assertFalse(self.dialog.KeysOK('<a>'))
        self.assertIn('shift modifier', self.dialog.showerror.message)
        self.dialog.GetModifiers.result = []

    def test_keys_dup(self):
        for mods, final, seq in (([], 'F12', '<Key-F12>'),
                                 (['Control'], 'x', '<Control-Key-x>'),
                                 (['Control'], 'X', '<Control-Key-X>')):
            with self.subTest(m=mods, f=final, s=seq):
                self.dialog.listKeysFinal.get.result = final
                self.dialog.GetModifiers.result = mods
                self.assertFalse(self.dialog.KeysOK(seq))
                self.assertIn('already in use', self.dialog.showerror.message)
        self.dialog.GetModifiers.result = []

    def test_bind_ok(self):
        self.assertTrue(self.dialog.bind_ok('<Control-Shift-Key-a>'))
        self.assertEqual(self.dialog.showerror.message, '')

    def test_bind_not_ok(self):
        self.assertFalse(self.dialog.bind_ok('<Control-Shift>'))
        self.assertIn('not accepted', self.dialog.showerror.message)


if __name__ == '__main__':
    unittest.main(verbosity=2)
