'''Test idlelib.help_about.

Coverage: 100%
'''
from test.support import requires, findfile
from tkinter import Tk, TclError
import unittest
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Mbox_func
from idlelib.help_about import AboutDialog as About
from idlelib import textview

class LiveDialogTest(unittest.TestCase):
    """Simulate user clicking buttons other than [Close].

    Test that invoked textview has text from source.
    """
    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.dialog = About(cls.root, 'About IDLE', _utest=True)

    @classmethod
    def tearDownClass(cls):
        del cls.dialog
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_dialog_title(self):
        """Test about dialog title"""
        self.assertEqual(self.dialog.title(), 'About IDLE')

    def test_printer_buttons(self):
        """Test buttons whose commands use printer function."""
        dialog = self.dialog
        button_sources = [(self.dialog.py_license, license),
                          (self.dialog.py_copyright, copyright),
                          (self.dialog.py_credits, credits)]

        for button, printer in button_sources:
            printer._Printer__setup()
            button.invoke()
            self.assertEqual(
                    printer._Printer__lines[0],
                    dialog._current_textview.text.get('1.0', '1.end'))
            self.assertEqual(
                    printer._Printer__lines[1],
                    dialog._current_textview.text.get('2.0', '2.end'))
            dialog._current_textview.destroy()

    def test_file_buttons(self):
        """Test buttons that display files."""
        dialog = self.dialog
        button_sources = [(self.dialog.readme, 'README.txt'),
                          (self.dialog.idle_news, 'NEWS.txt'),
                          (self.dialog.idle_credits, 'CREDITS.txt')]

        for button, filename in button_sources:
            button.invoke()
            fn = findfile(filename, subdir='idlelib')
            with open(fn) as f:
                self.assertEqual(
                        f.readline().strip(),
                        dialog._current_textview.text.get('1.0', '1.end'))
                f.readline()
                self.assertEqual(
                    f.readline().strip(),
                    dialog._current_textview.text.get('3.0', '3.end'))
            dialog._current_textview.destroy()


class CloseTest(unittest.TestCase):
    """Simulate user clicking [Close] button"""

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.dialog = About(cls.root, 'About IDLE', _utest=True)

    @classmethod
    def tearDownClass(cls):
        del cls.dialog
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_close(self):
        self.assertEqual(self.dialog.winfo_class(), 'Toplevel')
        self.dialog.button_ok.invoke()
        with self.assertRaises(TclError):
            self.dialog.winfo_class()


class Dummy_about_dialog():
    # Dummy class for testing file display functions.
    idle_credits = About.show_idle_credits
    idle_readme = About.show_readme
    idle_news = About.show_idle_news
    # Called by the above
    display_file_text = About.display_file_text
    _utest = True


class DisplayFileTest(unittest.TestCase):
    """Test functions that display files.

    While somewhat redundant with gui-based test_file_dialog,
    these unit tests run on all buildbots, not just a few.
    """
    dialog = Dummy_about_dialog()

    @classmethod
    def setUpClass(cls):
        cls.orig_error = textview.showerror
        cls.orig_view = textview.view_text
        cls.error = Mbox_func()
        cls.view = Func()
        textview.showerror = cls.error
        textview.view_text = cls.view

    @classmethod
    def tearDownClass(cls):
        textview.showerror = cls.orig_error
        textview.view_text = cls.orig_view

    def test_file_display(self):
        for handler in (self.dialog.idle_credits,
                        self.dialog.idle_readme,
                        self.dialog.idle_news):
            self.error.message = ''
            self.view.called = False
            with self.subTest(handler=handler):
                handler()
                self.assertEqual(self.error.message, '')
                self.assertEqual(self.view.called, True)


if __name__ == '__main__':
    unittest.main(verbosity=2)
