"""Unittests for idlelib.configHelpSourceEdit"""
import unittest
from idlelib.idle_test.mock_tk import Var, Mbox, Entry
from idlelib import configHelpSourceEdit as help_dialog_module

help_dialog = help_dialog_module.GetHelpSourceDialog


class Dummy_help_dialog:
    # Mock for testing the following methods of help_dialog
    menu_ok = help_dialog.menu_ok
    path_ok = help_dialog.path_ok
    ok = help_dialog.ok
    cancel = help_dialog.cancel
    # Attributes, constant or variable, needed for tests
    menu = Var()
    entryMenu = Entry()
    path = Var()
    entryPath = Entry()
    result = None
    destroyed = False

    def destroy(self):
        self.destroyed = True


# menu_ok and path_ok call Mbox.showerror if menu and path are not ok.
orig_mbox = help_dialog_module.tkMessageBox
showerror = Mbox.showerror


class ConfigHelpTest(unittest.TestCase):
    dialog = Dummy_help_dialog()

    @classmethod
    def setUpClass(cls):
        help_dialog_module.tkMessageBox = Mbox

    @classmethod
    def tearDownClass(cls):
        help_dialog_module.tkMessageBox = orig_mbox

    def test_blank_menu(self):
        self.dialog.menu.set('')
        self.assertFalse(self.dialog.menu_ok())
        self.assertEqual(showerror.title, 'Menu Item Error')
        self.assertIn('No', showerror.message)

    def test_long_menu(self):
        self.dialog.menu.set('hello' * 10)
        self.assertFalse(self.dialog.menu_ok())
        self.assertEqual(showerror.title, 'Menu Item Error')
        self.assertIn('long', showerror.message)

    def test_good_menu(self):
        self.dialog.menu.set('help')
        showerror.title = 'No Error'  # should not be called
        self.assertTrue(self.dialog.menu_ok())
        self.assertEqual(showerror.title, 'No Error')

    def test_blank_path(self):
        self.dialog.path.set('')
        self.assertFalse(self.dialog.path_ok())
        self.assertEqual(showerror.title, 'File Path Error')
        self.assertIn('No', showerror.message)

    def test_invalid_file_path(self):
        self.dialog.path.set('foobar' * 100)
        self.assertFalse(self.dialog.path_ok())
        self.assertEqual(showerror.title, 'File Path Error')
        self.assertIn('not exist', showerror.message)

    def test_invalid_url_path(self):
        self.dialog.path.set('ww.foobar.com')
        self.assertFalse(self.dialog.path_ok())
        self.assertEqual(showerror.title, 'File Path Error')
        self.assertIn('not exist', showerror.message)

        self.dialog.path.set('htt.foobar.com')
        self.assertFalse(self.dialog.path_ok())
        self.assertEqual(showerror.title, 'File Path Error')
        self.assertIn('not exist', showerror.message)

    def test_good_path(self):
        self.dialog.path.set('https://docs.python.org')
        showerror.title = 'No Error'  # should not be called
        self.assertTrue(self.dialog.path_ok())
        self.assertEqual(showerror.title, 'No Error')

    def test_ok(self):
        self.dialog.destroyed = False
        self.dialog.menu.set('help')
        self.dialog.path.set('https://docs.python.org')
        self.dialog.ok()
        self.assertEqual(self.dialog.result, ('help',
                                              'https://docs.python.org'))
        self.assertTrue(self.dialog.destroyed)

    def test_cancel(self):
        self.dialog.destroyed = False
        self.dialog.cancel()
        self.assertEqual(self.dialog.result, None)
        self.assertTrue(self.dialog.destroyed)

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
