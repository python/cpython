''' Test idlelib.config_key.

Coverage: 56% from creating and closing dialog.
'''
from idlelib import config_key
from test.support import requires
import unittest
from tkinter import Tk
from idlelib.idle_test.mock_tk import Mbox_func


class GetKeysTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.update()  # Stop "can't run event command" warning.
        cls.root.destroy()
        del cls.root

    def test_init(self):
        dia = config_key.GetKeysDialog(
            self.root, 'test', '<<Test>>', ['<Key-F12>'], _utest=True)
        dia.Cancel()


class SequenceOKTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.orig_error = config_key.tkMessageBox.showerror
        config_key.tkMessageBox.showerror = Mbox_func()
        cls.root = Tk()
        cls.root.withdraw()
        cls.dialog = config_key.GetKeysDialog(
            cls.root, 'test', '<<Test>>', ['<Key-F12>'], _utest=True)

    @classmethod
    def tearDownClass(cls):
        config_key.tkMessageBox.showerror = cls.orig_error
        del cls.orig_error
        del cls.dialog
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_sequence_ok_valid(self):
        self.assertTrue(self.dialog.sequence_ok('<Control-Shift-Key-a>'))

    def test_sequence_ok_invalid(self):
        self.assertFalse(self.dialog.sequence_ok('<Control-Shift>'))
        self.assertEqual(config_key.tkMessageBox.showerror.title,
                         self.dialog.keyerror_title)

if __name__ == '__main__':
    unittest.main(verbosity=2)
