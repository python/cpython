import os
import unittest
from tkinter import filedialog
from tkinter.commondialog import Dialog
from test.support import requires, swap_attr
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractTkTest

requires('gui')


class NativeDialogTest(AbstractTkTest, unittest.TestCase):
    # Open, SaveAs and Directory wrap modal native dialogs.  The _test_callback
    # seam is called by show() just before the dialog would open; replacing it
    # with a function that raises exercises show() without blocking on the
    # dialog.

    def check(self, dialog_class, command):
        self.assertEqual(dialog_class.command, command)
        master = None
        def test_callback(dialog, parent):
            nonlocal master
            master = parent
            raise ZeroDivisionError
        with swap_attr(Dialog, '_test_callback', test_callback):
            d = dialog_class(self.root, title='Test')
            self.assertRaises(ZeroDivisionError, d.show)
        self.assertIs(master, self.root)

    def test_open(self):
        self.check(filedialog.Open, 'tk_getOpenFile')

    def test_saveas(self):
        self.check(filedialog.SaveAs, 'tk_getSaveFile')

    def test_directory(self):
        self.check(filedialog.Directory, 'tk_chooseDirectory')


class FileDialogTest(AbstractTkTest, unittest.TestCase):
    # The pure-Python FileDialog runs its own modal loop in go(); its logic is
    # exercised here without entering the loop.

    def test_selection(self):
        d = filedialog.FileDialog(self.root)
        d.directory = os.path.abspath(os.sep)
        d.set_selection('spam.txt')
        self.assertEqual(os.path.basename(d.get_selection()), 'spam.txt')

    def test_filter(self):
        d = filedialog.FileDialog(self.root)
        d.set_filter(os.getcwd(), '*.py')
        self.assertEqual(d.get_filter(), (os.getcwd(), '*.py'))

    def test_ok_cancel(self):
        d = filedialog.FileDialog(self.root)
        d.directory = os.getcwd()
        d.set_selection('spam.txt')
        d.ok_command()  # Accepts the current selection.
        self.assertEqual(os.path.basename(d.how), 'spam.txt')
        d.cancel_command()  # Returns no selection.
        self.assertIsNone(d.how)

    def test_subclasses(self):
        for cls in filedialog.LoadFileDialog, filedialog.SaveFileDialog:
            with self.subTest(cls=cls.__name__):
                d = cls(self.root)
                self.assertIsInstance(d, filedialog.FileDialog)
                self.assertEqual(d.top.title(), cls.title)


if __name__ == "__main__":
    unittest.main()
