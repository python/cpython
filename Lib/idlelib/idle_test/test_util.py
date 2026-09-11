"""Test util, coverage 100%"""

import sys
import unittest
from unittest import mock
from test.support import requires
from test.support.isolation import runInSubprocess
import tkinter
from tkinter import EventType
from idlelib import util
from idlelib.idle_test.mock_tk import Event


class UtilTest(unittest.TestCase):

    def test_extensions(self):
        for extension in {'.pyi', '.py', '.pyw'}:
            self.assertIn(extension, util.py_extensions)

    @unittest.skipUnless(sys.platform == 'win32', 'Windows only')
    @runInSubprocess()
    def test_fix_win_hidpi(self):
        # Awareness is process-wide and cannot be undone.
        import ctypes
        PROCESS_DPI_UNAWARE = 0
        util.fix_win_hidpi()
        awareness = ctypes.c_int()
        ctypes.OleDLL('shcore').GetProcessDpiAwareness(
                None, ctypes.byref(awareness))
        self.assertNotEqual(awareness.value, PROCESS_DPI_UNAWARE)


class WheelTest(unittest.TestCase):
    "Test the wheel functions with a widget on this display."

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tkinter.Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.text = tkinter.Text(self.root)
        self.addCleanup(self.text.destroy)

    def test_x11_buttons(self):
        # Only X11 before Tk 8.7 sends the wheel as button events.
        text = self.text
        if text._windowingsystem == 'x11' and tkinter.TkVersion < 8.7:
            self.assertTrue(util.x11_buttons(text))
        else:
            self.assertFalse(util.x11_buttons(text))

    def test_bind_wheel(self):
        # The events Tk sends here are the ones bound.
        text = self.text
        util.bind_wheel(text, util.wheel_event)
        if util.x11_buttons(text):
            self.assertEqual(sorted(text.bind()),
                             ['<Button-4>', '<Button-5>'])
        else:
            self.assertEqual(sorted(text.bind()), ['<MouseWheel>'])


class WheelEventTest(unittest.TestCase):
    "Test the direction and the amount of the scroll."

    # An unmapped widget has no height and does not scroll by lines,
    # so record the yview call instead of a real scroll.
    def event(self, event_type, delta=0, num='??'):
        # Tk leaves num '??' for a wheel event and delta 0 for a button.
        return Event(type=event_type, delta=delta, num=num,
                     widget=mock.Mock())

    def scroll(self, event, widget=None):
        "Return the arguments of the yview call."
        self.assertEqual(util.wheel_event(event, widget), 'break')
        scrolled = event.widget if widget is None else widget
        scrolled.yview.assert_called_once()
        return scrolled.yview.call_args.args

    def test_mousewheel(self):
        # Delta is positive for up on all systems.
        for delta in 120, 1, 1200:
            self.assertEqual(self.scroll(self.event(EventType.MouseWheel,
                                                    delta)),
                             ('scroll', -5, 'units'))
            self.assertEqual(self.scroll(self.event(EventType.MouseWheel,
                                                    -delta)),
                             ('scroll', 5, 'units'))

    def test_buttons(self):
        self.assertEqual(self.scroll(self.event(EventType.ButtonPress, num=4)),
                         ('scroll', -5, 'units'))
        self.assertEqual(self.scroll(self.event(EventType.ButtonPress, num=5)),
                         ('scroll', 5, 'units'))

    def test_widget_argument(self):
        # A tree label scrolls the canvas, not itself.
        event = self.event(EventType.MouseWheel, 120)
        canvas = mock.Mock()
        self.assertEqual(self.scroll(event, canvas), ('scroll', -5, 'units'))
        event.widget.yview.assert_not_called()


class FixTest(unittest.TestCase):
    "Test the fix_ functions, which need a display."

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tkinter.Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def test_fix_scaling(self):
        from tkinter import font
        root = self.root
        scaling = root.tk.call('tk', 'scaling')  # No Misc.tk_scaling yet.
        self.addCleanup(root.tk.call, 'tk', 'scaling', scaling)
        # Both fonts go with the root; Font.delete_font is a flag.
        pixels = font.Font(root=root, name='TestPixelFont', size=-16)
        points = font.Font(root=root, name='TestPointFont', size=12)

        root.tk.call('tk', 'scaling', 1.0)
        util.fix_scaling(root)  # No scaling, no change.
        self.assertEqual(int(pixels['size']), -16)

        root.tk.call('tk', 'scaling', 2.0)
        util.fix_scaling(root)  # A size in pixels becomes one in points.
        self.assertEqual(int(pixels['size']), 12)  # round(-0.75 * -16)
        self.assertEqual(int(points['size']), 12)  # Points are left alone.

    def test_fix_word_breaks(self):
        root = self.root
        util.fix_word_breaks(root)
        self.assertEqual(root.tk.call('set', 'tcl_wordchars'), r'\w')
        self.assertEqual(root.tk.call('set', 'tcl_nonwordchars'), r'\W')

    def test_fix_x11_paste(self):
        root = self.root
        classes = 'Text', 'Entry', 'Spinbox'
        before = {cls: root.bind_class(cls, '<<Paste>>') for cls in classes}
        util.fix_x11_paste(root)
        for cls in classes:
            with self.subTest(cls=cls):
                after = root.bind_class(cls, '<<Paste>>')
                if root._windowingsystem == 'x11':
                    # Deleting the selection makes paste replace it.
                    self.assertEqual(
                        after,
                        'catch {%W delete sel.first sel.last}\n' + before[cls])
                else:
                    self.assertEqual(after, before[cls])


if __name__ == '__main__':
    unittest.main(verbosity=2)
