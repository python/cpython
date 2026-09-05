"""Test util, coverage %"""

import unittest
from test.support import requires
requires('gui')
from idlelib import util  # Creates root and widget.


class UtilTest(unittest.TestCase):
    def test_extensions(self):
        for extension in {'.pyi', '.py', '.pyw'}:
            self.assertIn(extension, util.py_extensions)

class ScrollTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.orig_x11 = util.x11_buttons

    @classmethod
    def tearDownClass(cls):
        util.x11_buttons = cls.orig_x11

    def test_wheel_event(self):
        # Fake widget class containing `yview` only.
        class _Widget:
            def yview(widget, scroll, lines, units):
                self.assertTupleEqual((scroll, lines, units),
                                      ('scroll', widget.lines, 'units'))
        # Fake event class
        class _Event:
            pass

        event = _Event()
        event.widget = widget = _Widget()
        tests = ((False, 120, -5),
                 (False, -120, 5),
                 (True, 4, -5),
                 (True, 5, 5))
        for x11, value, lines in tests:
            util.x11_buttons = x11
            event.delta, event.num = (None, value) if x11 else (value, None)
            widget.lines = lines
            self.assertEqual(util.wheel_event(event), "break")

        (widget2 := _Widget()).lines = widget.lines
        widget.lines = 0  # Make assert fail if widget.yview called.
        util.wheel_event(event, widget2)  # Test delagation to passed widget.


if __name__ == '__main__':
    unittest.main(verbosity=2)
