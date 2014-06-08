'''Test the functions and main class method of textView.py.'''

import unittest
import os
from test.test_support import requires
from Tkinter import Tk, Text, TclError
from idlelib import textView as tv
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Mbox

orig_mbox = tv.tkMessageBox

class textviewClassTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.TV = TV = tv.TextViewer
        TV.transient = Func()
        TV.grab_set = Func()
        TV.wait_window = Func()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        TV = cls.TV
        del cls.root, cls.TV
        del TV.transient, TV.grab_set, TV.wait_window

    def setUp(self):
        TV = self.TV
        TV.transient.__init__()
        TV.grab_set.__init__()
        TV.wait_window.__init__()


    def test_init_modal(self):
        TV = self.TV
        view = TV(self.root, 'Title', 'test text')
        self.assertTrue(TV.transient.called)
        self.assertTrue(TV.grab_set.called)
        self.assertTrue(TV.wait_window.called)
        view.Ok()

    def test_init_nonmodal(self):
        TV = self.TV
        view = TV(self.root, 'Title', 'test text', modal=False)
        self.assertFalse(TV.transient.called)
        self.assertFalse(TV.grab_set.called)
        self.assertFalse(TV.wait_window.called)
        view.Ok()

    def test_ok(self):
        view = self.TV(self.root, 'Title', 'test text', modal=False)
        view.destroy = Func()
        view.Ok()
        self.assertTrue(view.destroy.called)
        del view.destroy  # unmask real function
        view.destroy


class textviewTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        tv.tkMessageBox = Mbox

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root
        tv.tkMessageBox = orig_mbox

    def test_view_text(self):
        # If modal True, tkinter will error with 'can't invoke "event" command'
        view = tv.view_text(self.root, 'Title', 'test text', modal=False)
        self.assertIsInstance(view, tv.TextViewer)

    def test_view_file(self):
        test_dir = os.path.dirname(__file__)
        testfile = os.path.join(test_dir, 'test_textview.py')
        view = tv.view_file(self.root, 'Title', testfile, modal=False)
        self.assertIsInstance(view, tv.TextViewer)
        self.assertIn('Test', view.textView.get('1.0', '1.end'))
        view.Ok()

        # Mock messagebox will be used and view_file will not return anything
        testfile = os.path.join(test_dir, '../notthere.py')
        view = tv.view_file(self.root, 'Title', testfile, modal=False)
        self.assertIsNone(view)

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
    from idlelib.idle_test.htest import run
    run(TextViewer)
