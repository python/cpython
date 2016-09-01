'''Test the functions and main class method of textView.py.'''

import unittest
import os
from test.test_support import requires
from Tkinter import Tk
from idlelib import textView as tv
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Mbox


class TV(tv.TextViewer):  # Use in TextViewTest
    transient = Func()
    grab_set = Func()
    wait_window = Func()

class textviewClassTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def setUp(self):
        TV.transient.__init__()
        TV.grab_set.__init__()
        TV.wait_window.__init__()

    def test_init_modal(self):
        view = TV(self.root, 'Title', 'test text')
        self.assertTrue(TV.transient.called)
        self.assertTrue(TV.grab_set.called)
        self.assertTrue(TV.wait_window.called)
        view.Ok()

    def test_init_nonmodal(self):
        view = TV(self.root, 'Title', 'test text', modal=False)
        self.assertFalse(TV.transient.called)
        self.assertFalse(TV.grab_set.called)
        self.assertFalse(TV.wait_window.called)
        view.Ok()

    def test_ok(self):
        view = TV(self.root, 'Title', 'test text', modal=False)
        view.destroy = Func()
        view.Ok()
        self.assertTrue(view.destroy.called)
        del view.destroy  # Unmask the real function.
        view.destroy()


class ViewFunctionTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.orig_mbox = tv.tkMessageBox
        tv.tkMessageBox = Mbox

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root
        tv.tkMessageBox = cls.orig_mbox
        del cls.orig_mbox

    def test_view_text(self):
        # If modal True, get tkinter error 'can't invoke "event" command'.
        view = tv.view_text(self.root, 'Title', 'test text', modal=False)
        self.assertIsInstance(view, tv.TextViewer)
        view.Ok()

    def test_view_file(self):
        test_dir = os.path.dirname(__file__)
        testfile = os.path.join(test_dir, 'test_textview.py')
        view = tv.view_file(self.root, 'Title', testfile, modal=False)
        self.assertIsInstance(view, tv.TextViewer)
        self.assertIn('Test', view.textView.get('1.0', '1.end'))
        view.Ok()

        # Mock messagebox will be used; view_file will return None.
        testfile = os.path.join(test_dir, '../notthere.py')
        view = tv.view_file(self.root, 'Title', testfile, modal=False)
        self.assertIsNone(view)


if __name__ == '__main__':
    unittest.main(verbosity=2)
