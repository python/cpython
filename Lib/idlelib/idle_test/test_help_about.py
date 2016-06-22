'''Test idlelib.help_about.

Coverage:
'''
from idlelib import help_about
from idlelib import textview
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Mbox_func
import unittest

About = help_about.AboutDialog
class Dummy_about_dialog():
    # Dummy class for testing file display functions.
    idle_credits = About.ShowIDLECredits
    idle_readme = About.ShowIDLEAbout
    idle_news = About.ShowIDLENEWS
    # Called by the above
    display_file_text = About.display_file_text


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
