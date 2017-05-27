'''Test idlelib.help_about.

Coverage:
'''
from idlelib import help_about
from idlelib import textview
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Mbox_func
from test.support import requires, findfile
requires('gui')
from tkinter import Tk
import unittest


About = help_about.AboutDialog
class Dummy_about_dialog():
    # Dummy class for testing file display functions.
    idle_credits = About.show_idle_credits
    idle_readme = About.show_readme
    idle_news = About.show_idle_news
    # Called by the above
    display_file_text = About.display_file_text
    _utest = True


class AboutDialogTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()
        cls.dialog = About(cls.root, 'About IDLE', _utest=True)

    @classmethod
    def tearDownClass(cls):
        del cls.dialog
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def tearDown(self):
        if self.dialog._current_textview:
            self.dialog._current_textview.destroy()

    def test_dialog_title(self):
        """This will test about dialog title"""
        self.assertEqual(self.dialog.title(), 'About IDLE')

    def test_printer_dialog(self):
        """This will test dialog which using printer"""
        buttons = [(license, self.dialog.py_license),
                   (copyright, self.dialog.py_copyright),
                   (credits, self.dialog.py_credits)]

        for printer, button in buttons:
            dialog = self.dialog
            printer._Printer__setup()
            button.invoke()
            self.assertEqual(printer._Printer__lines[0],
                             dialog._current_textview.textView.get('1.0', '1.end'))
            self.assertEqual(printer._Printer__lines[1],
                             dialog._current_textview.textView.get('2.0', '2.end'))

            dialog._current_textview.destroy()

    def test_file_dialog(self):
        """This will test dialog which using file"""
        buttons = [('README.txt', self.dialog.readme),
                   ('NEWS.txt', self.dialog.idle_news),
                   ('CREDITS.txt', self.dialog.idle_credits)]

        for filename, button in buttons:
            dialog = self.dialog
            button.invoke()
            fn = findfile(filename, subdir='idlelib')
            with open(fn) as f:
                self.assertEqual(f.readline().strip(),
                                 dialog._current_textview.textView.get('1.0', '1.end'))
                f.readline()
                self.assertEqual(f.readline().strip(),
                                 dialog._current_textview.textView.get('3.0', '3.end'))
            dialog._current_textview.destroy()


class DisplayFileTest(unittest.TestCase):
    dialog = Dummy_about_dialog()

    @classmethod
    def setUpClass(cls):
        cls.orig_error = textview.showerror
        cls.orig_view = textview.view_text
        cls.error = Mbox_func()
        cls.view = Func()
        textview.showerror = cls.error
        textview.view_text = cls.view
        cls.About = Dummy_about_dialog()

    @classmethod
    def tearDownClass(cls):
        textview.showerror = cls.orig_error
        textview.view_text = cls.orig_view

    def test_file_isplay(self):
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
