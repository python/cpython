"Test config_key, coverage 82%"

from idlelib import config_key
from test.support import requires
import unittest
from tkinter import Tk
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Mbox_func


class ValidationTest(unittest.TestCase):
    "Test validation methods: ok, keys_ok, bind_ok."

    class Validator(config_key.GetKeysDialog):
        def __init__(self, *args, **kwargs):
            config_key.GetKeysDialog.__init__(self, *args, **kwargs)
            class list_keys_final:
                get = Func()
            self.list_keys_final = list_keys_final
        get_modifiers = Func()
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
        cls.dialog.cancel()
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.dialog, cls.root

    def setUp(self):
        self.dialog.showerror.message = ''
    # A test that needs a particular final key value should set it.
    # A test that sets a non-blank modifier list should reset it to [].

    def test_ok_empty(self):
        self.dialog.key_string.set(' ')
        self.dialog.ok()
        self.assertEqual(self.dialog.result, '')
        self.assertEqual(self.dialog.showerror.message, 'No key specified.')

    def test_ok_good(self):
        self.dialog.key_string.set('<Key-F11>')
        self.dialog.list_keys_final.get.result = 'F11'
        self.dialog.ok()
        self.assertEqual(self.dialog.result, '<Key-F11>')
        self.assertEqual(self.dialog.showerror.message, '')

    def test_keys_no_ending(self):
        self.assertFalse(self.dialog.keys_ok('<Control-Shift'))
        self.assertIn('Missing the final', self.dialog.showerror.message)

    def test_keys_no_modifier_bad(self):
        self.dialog.list_keys_final.get.result = 'A'
        self.assertFalse(self.dialog.keys_ok('<Key-A>'))
        self.assertIn('No modifier', self.dialog.showerror.message)

    def test_keys_no_modifier_ok(self):
        self.dialog.list_keys_final.get.result = 'F11'
        self.assertTrue(self.dialog.keys_ok('<Key-F11>'))
        self.assertEqual(self.dialog.showerror.message, '')

    def test_keys_shift_bad(self):
        self.dialog.list_keys_final.get.result = 'a'
        self.dialog.get_modifiers.result = ['Shift']
        self.assertFalse(self.dialog.keys_ok('<a>'))
        self.assertIn('shift modifier', self.dialog.showerror.message)
        self.dialog.get_modifiers.result = []

    def test_keys_dup(self):
        for mods, final, seq in (([], 'F12', '<Key-F12>'),
                                 (['Control'], 'x', '<Control-Key-x>'),
                                 (['Control'], 'X', '<Control-Key-X>')):
            with self.subTest(m=mods, f=final, s=seq):
                self.dialog.list_keys_final.get.result = final
                self.dialog.get_modifiers.result = mods
                self.assertFalse(self.dialog.keys_ok(seq))
                self.assertIn('already in use', self.dialog.showerror.message)
        self.dialog.get_modifiers.result = []

    def test_bind_ok(self):
        self.assertTrue(self.dialog.bind_ok('<Control-Shift-Key-a>'))
        self.assertEqual(self.dialog.showerror.message, '')

    def test_bind_not_ok(self):
        self.assertFalse(self.dialog.bind_ok('<Control-Shift>'))
        self.assertIn('not accepted', self.dialog.showerror.message)


if __name__ == '__main__':
    unittest.main(verbosity=2)
