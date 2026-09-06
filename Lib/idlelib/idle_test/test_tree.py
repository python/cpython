"Test tree. coverage 56%."

from idlelib import tree
import unittest
from test.support import requires
requires('gui')
from tkinter import Tk, EventType, SCROLL


class TreeTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def test_init(self):
        # Start with code slightly adapted from htest.
        sc = tree.ScrolledCanvas(
            self.root, bg="white", highlightthickness=0, takefocus=1)
        sc.frame.pack(expand=1, fill="both", side='left')
        item = tree.FileTreeItem(tree.ICONDIR)
        node = tree.TreeNode(sc.canvas, None, item)
        node.expand()


class TestScrollEvent(unittest.TestCase):

    def test_wheel_event(self):
        # Fake widget class containing `xview` and `yview` only.
        class _Widget:
            def __init__(widget, direction, *expected):
                widget.state = state
                widget.expected = expected
            def xview(widget, *args):
                self.assertEqual(widget.state&1, 1)
                self.assertTupleEqual(widget.expected, args)
            def yview(widget, *args):
                self.assertEqual(widget.state&1, 0)
                self.assertTupleEqual(widget.expected, args)
        # Fake event class
        class _Event:
            pass
        #        (type, delta, num, state, amount)
        # If the first bit of state is set, scroll horizontally
        tests = ((EventType.MouseWheel, 120, -1, 0, -5),
                 (EventType.MouseWheel, -120, -1, 0, 5),
                 (EventType.ButtonPress, -1, 4, 0, -5),
                 (EventType.ButtonPress, -1, 5, 0, 5),
                 (EventType.MouseWheel, 120, -1, 1, -5),
                 (EventType.MouseWheel, -120, -1, 1, 5),
                 (EventType.ButtonPress, -1, 4, 1, -5),
                 (EventType.ButtonPress, -1, 5, 1, 5))

        event = _Event()
        for ty, delta, num, state, amount in tests:
            event.type = ty
            event.delta = delta
            event.num = num
            event.state = state
            res = tree.wheel_event(event,
                                   _Widget(state, SCROLL, amount, "units"))
            self.assertEqual(res, "break")


if __name__ == '__main__':
    unittest.main(verbosity=2)
