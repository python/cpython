''' Test idlelib.config_key.

Coverage: 56% from creating and closing dialog.
'''
from idlelib import config_key
from test.support import requires
import sys
import unittest
from tkinter import Tk
from tkinter import messagebox
from idlelib.idle_test.mock_tk import Mbox_func


class GetKeysTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        config_key.showerror = Mbox_func()
        cls.root = Tk()
        cls.root.withdraw()
        cls.dialog = config_key.GetKeysDialog(
            cls.root, 'test', '<<Test>>', ['<Key-F12>'], _utest=True)

    @classmethod
    def tearDownClass(cls):
        del cls.dialog
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root
        config_key.showerror = messagebox.showerror

    def setUp(self):
        config_key.showerror.message = ''

    def test_init(self):
        dia = config_key.GetKeysDialog(
            self.root, 'test', '<<Test>>', [['<Control-Key-x>']], _utest=True)
        dia.Cancel()
        self.assertEqual(config_key.showerror.message, '')

    def test_ok_empty(self):
        self.dialog.keyString.set(' ')
        self.dialog.OK()
        self.assertEqual(self.dialog.result, '')
        self.assertEqual(config_key.showerror.message, 'No key specified.')

# Getting this test to work requires ability to get KeyOK to return True.
# This requires an ability to manipulate modifiers and final key
# or a refactoring of code to make it more easily tested
# or temporary mocks.
##    def test_ok_good(self):
##        self.dialog.keyString.set('Key-F11')
##        self.dialog.OK()
##        self.assertEqual(self.dialog.result, '<Key-F11>')
##        self.assertEqual(config_key.showerror.message, '')

    def test_keys_short(self):
        self.assertFalse(self.dialog.KeysOK('<Control-Shift'))
        self.assertIn('Missing the final', config_key.showerror.message)

    def test_keys_mod(self):
        self.assertFalse(self.dialog.KeysOK('<Key-A>'))
        self.assertIn('No modifier', config_key.showerror.message)

# Need tests for shift, duplication, and success this or test_keys_shift or test_keys_ok to work
# 
# The attempt below failed.  Code should be made easier to test.
##    def test_keys_dup(self):
##        modifier_index = 1 if sys.platform == 'darwin' else 0
##        self.dialog.modifier_vars[modifier_index].set('Control')
##        self.assertFalse(self.dialog.KeysOK('<Control-Key-x>'))
##        self.assertIn('already in use', config_key.showerror.message)


    def test_bind_ok(self):
        self.assertTrue(self.dialog.bind_ok('<Control-Shift-Key-a>'))
        self.assertEqual(config_key.showerror.message, '')

    def test_bind_not_ok(self):
        self.assertFalse(self.dialog.bind_ok('<Control-Shift>'))
        self.assertIn('not accepted', config_key.showerror.message)


if __name__ == '__main__':
    unittest.main(verbosity=2)
