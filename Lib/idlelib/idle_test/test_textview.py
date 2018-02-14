'''Test idlelib.textview.

Since all methods and functions create (or destroy) a ViewWindow, which
is a widget containing a widget, etcetera, all tests must be gui tests.
Using mock Text would not change this.  Other mocks are used to retrieve
information about calls.

Coverage: 100%.
'''
from idlelib import textview as tv
from test.support import requires
requires('gui')

import unittest
import os
from tkinter import Tk
from tkinter.ttk import Button
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Mbox_func

def setUpModule():
    global root
    root = Tk()
    root.withdraw()

def tearDownModule():
    global root
    root.update_idletasks()
    root.destroy()
    del root

# If we call ViewWindow or wrapper functions with defaults
# modal=True, _utest=False, test hangs on call to wait_window.
# Have also gotten tk error 'can't invoke "event" command'.


class VW(tv.ViewWindow):  # Used in ViewWindowTest.
    transient = Func()
    grab_set = Func()
    wait_window = Func()


# Call wrapper class VW with mock wait_window.
class ViewWindowTest(unittest.TestCase):

    def setUp(self):
        VW.transient.__init__()
        VW.grab_set.__init__()
        VW.wait_window.__init__()

    def test_init_modal(self):
        view = VW(root, 'Title', 'test text')
        self.assertTrue(VW.transient.called)
        self.assertTrue(VW.grab_set.called)
        self.assertTrue(VW.wait_window.called)
        view.ok()

    def test_init_nonmodal(self):
        view = VW(root, 'Title', 'test text', modal=False)
        self.assertFalse(VW.transient.called)
        self.assertFalse(VW.grab_set.called)
        self.assertFalse(VW.wait_window.called)
        view.ok()

    def test_ok(self):
        view = VW(root, 'Title', 'test text', modal=False)
        view.destroy = Func()
        view.ok()
        self.assertTrue(view.destroy.called)
        del view.destroy  # Unmask real function.
        view.destroy()


class TextFrameTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        "By itself, this tests that file parsed without exception."
        cls.root = root = Tk()
        root.withdraw()
        cls.frame = tv.TextFrame(root, 'test text')

    @classmethod
    def tearDownClass(cls):
        del cls.frame
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_line1(self):
        get = self.frame.text.get
        self.assertEqual(get('1.0', '1.end'), 'test text')


# Call ViewWindow with modal=False.
class ViewFunctionTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.orig_error = tv.showerror
        tv.showerror = Mbox_func()

    @classmethod
    def tearDownClass(cls):
        tv.showerror = cls.orig_error
        del cls.orig_error

    def test_view_text(self):
        view = tv.view_text(root, 'Title', 'test text', modal=False)
        self.assertIsInstance(view, tv.ViewWindow)
        self.assertIsInstance(view.viewframe, tv.ViewFrame)
        view.ok()

    def test_view_file(self):
        view = tv.view_file(root, 'Title', __file__, 'ascii', modal=False)
        self.assertIsInstance(view, tv.ViewWindow)
        self.assertIsInstance(view.viewframe, tv.ViewFrame)
        get = view.viewframe.textframe.text.get
        self.assertIn('Test', get('1.0', '1.end'))
        view.ok()

    def test_bad_file(self):
        # Mock showerror will be used; view_file will return None.
        view = tv.view_file(root, 'Title', 'abc.xyz', 'ascii', modal=False)
        self.assertIsNone(view)
        self.assertEqual(tv.showerror.title, 'File Load Error')

    def test_bad_encoding(self):
        p = os.path
        fn = p.abspath(p.join(p.dirname(__file__), '..', 'CREDITS.txt'))
        tv.showerror.title = None
        view = tv.view_file(root, 'Title', fn, 'ascii', modal=False)
        self.assertIsNone(view)
        self.assertEqual(tv.showerror.title, 'Unicode Decode Error')


# Call ViewWindow with _utest=True.
class ButtonClickTest(unittest.TestCase):

    def setUp(self):
        self.view = None
        self.called = False

    def tearDown(self):
        if self.view:
            self.view.destroy()

    def test_view_text_bind_with_button(self):
        def _command():
            self.called = True
            self.view = tv.view_text(root, 'TITLE_TEXT', 'COMMAND', _utest=True)
        button = Button(root, text='BUTTON', command=_command)
        button.invoke()
        self.addCleanup(button.destroy)

        self.assertEqual(self.called, True)
        self.assertEqual(self.view.title(), 'TITLE_TEXT')
        self.assertEqual(self.view.viewframe.textframe.text.get('1.0', '1.end'),
                         'COMMAND')

    def test_view_file_bind_with_button(self):
        def _command():
            self.called = True
            self.view = tv.view_file(root, 'TITLE_FILE', __file__,
                                     encoding='ascii', _utest=True)
        button = Button(root, text='BUTTON', command=_command)
        button.invoke()
        self.addCleanup(button.destroy)

        self.assertEqual(self.called, True)
        self.assertEqual(self.view.title(), 'TITLE_FILE')
        get = self.view.viewframe.textframe.text.get
        with open(__file__) as f:
            self.assertEqual(get('1.0', '1.end'), f.readline().strip())
            f.readline()
            self.assertEqual(get('3.0', '3.end'), f.readline().strip())


if __name__ == '__main__':
    unittest.main(verbosity=2)
