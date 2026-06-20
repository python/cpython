import unittest
import tkinter
from tkinter import dnd
from test.support import requires
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractTkTest

requires('gui')


class Target:
    def __init__(self, widget, log):
        self.widget = widget
        self.log = log
        widget.dnd_accept = self.dnd_accept

    def dnd_accept(self, source, event):
        self.log.append('accept')
        return self

    def dnd_enter(self, source, event):
        self.log.append('enter')

    def dnd_motion(self, source, event):
        self.log.append('motion')

    def dnd_leave(self, source, event):
        self.log.append('leave')

    def dnd_commit(self, source, event):
        self.log.append('commit')


class Source:
    def __init__(self, log):
        self.log = log

    def dnd_end(self, target, event):
        self.log.append('end')


class FakeEvent:
    def __init__(self, widget, num=1):
        self.num = num
        self.widget = widget
        self.x = self.y = self.x_root = self.y_root = 0


class DndTest(AbstractTkTest, unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.canvas = tkinter.Canvas(self.root)
        self.canvas.pack()
        # on_motion() locates the target with winfo_containing().  Bypass that
        # real screen lookup, which depends on the window being visible and
        # unobscured, so the test does not hinge on the window manager.
        self.canvas.winfo_containing = lambda x, y: self.canvas
        self.log = []
        self.source = Source(self.log)
        self.target = Target(self.canvas, self.log)

    def test_drag_and_drop(self):
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        self.assertIsNotNone(handler)
        handler.on_motion(FakeEvent(self.canvas))  # Enter the target.
        handler.on_motion(FakeEvent(self.canvas))  # Move within the target.
        handler.on_release(FakeEvent(self.canvas))  # Drop on the target.
        self.assertEqual(self.log,
                         ['accept', 'enter', 'accept', 'motion', 'commit', 'end'])

    def test_cancel(self):
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        handler.on_motion(FakeEvent(self.canvas))  # Enter the target.
        handler.cancel()  # Leaves the target without committing.
        self.assertEqual(self.log, ['accept', 'enter', 'leave', 'end'])

    def test_no_target(self):
        # Nothing under the pointer accepts the drag.
        self.canvas.winfo_containing = lambda x, y: None
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        handler.on_motion(FakeEvent(self.canvas))
        handler.on_release(FakeEvent(self.canvas))
        self.assertEqual(self.log, ['end'])

    def test_no_recursive_start(self):
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        self.assertIsNotNone(handler)
        # A drag is already in progress, so a second start is ignored.
        self.assertIsNone(dnd.dnd_start(self.source, FakeEvent(self.canvas)))
        handler.cancel()

    def test_high_button_number_ignored(self):
        self.assertIsNone(dnd.dnd_start(self.source, FakeEvent(self.canvas, num=6)))


if __name__ == "__main__":
    unittest.main()
