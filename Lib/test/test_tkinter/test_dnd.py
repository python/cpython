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
    def __init__(self, widget, num=1, x_root=0, y_root=0):
        self.num = num
        self.widget = widget
        self.x = self.y = 0
        self.x_root = x_root
        self.y_root = y_root


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

    def tearDown(self):
        # Make sure no drag-and-drop is left active between tests: the
        # recursion guard is a name-mangled attribute on the root.
        try:
            del self.root._DndHandler__dnd
        except AttributeError:
            pass
        super().tearDown()

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

    def test_restart_after_finish(self):
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        handler.cancel()
        # Once a drag has finished a new one can start.
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        self.assertIsNotNone(handler)
        handler.cancel()

    def test_drag_cursor(self):
        # The drag cursor is not shown on the initial press, only once the
        # pointer moves past the threshold, so a plain click does not flash
        # it (gh-43699).  The original cursor is restored afterwards.
        self.canvas['cursor'] = 'watch'
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        self.assertEqual(handler.save_cursor, 'watch')
        self.assertEqual(str(self.canvas['cursor']), 'watch')
        handler.on_motion(FakeEvent(self.canvas, x_root=2))  # below threshold
        self.assertEqual(str(self.canvas['cursor']), 'watch')
        handler.on_motion(FakeEvent(self.canvas, x_root=20))  # past threshold
        self.assertEqual(str(self.canvas['cursor']), 'hand2')
        handler.cancel()
        self.assertEqual(str(self.canvas['cursor']), 'watch')

    def test_bindings_added_and_removed(self):
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        self.assertIn('<Motion>', self.canvas.bind())
        self.assertIn('<B1-ButtonRelease-1>', self.canvas.bind())
        handler.cancel()
        self.assertNotIn('<Motion>', self.canvas.bind())
        self.assertNotIn('<B1-ButtonRelease-1>', self.canvas.bind())

    def test_switch_target(self):
        log1, log2 = [], []
        w1, w2 = tkinter.Frame(self.root), tkinter.Frame(self.root)
        target1, target2 = Target(w1, log1), Target(w2, log2)
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        self.canvas.winfo_containing = lambda x, y: w1
        handler.on_motion(FakeEvent(self.canvas))  # Enter target1.
        self.canvas.winfo_containing = lambda x, y: w2
        handler.on_motion(FakeEvent(self.canvas))  # Leave target1, enter target2.
        self.assertIs(handler.target, target2)
        self.assertEqual(log1, ['accept', 'enter', 'leave'])
        self.assertEqual(log2, ['accept', 'enter'])
        handler.cancel()

    def test_target_in_ancestor(self):
        # The widget under the pointer has no dnd_accept, but an ancestor
        # does: the search walks up the master chain to find it.
        parent = tkinter.Frame(self.root)
        target = Target(parent, self.log)
        child = tkinter.Frame(parent)
        self.canvas.winfo_containing = lambda x, y: child
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        handler.on_motion(FakeEvent(self.canvas))
        self.assertIs(handler.target, target)
        self.assertEqual(self.log, ['accept', 'enter'])
        handler.cancel()

    def test_accept_returning_none_continues(self):
        # dnd_accept() returning None means "not me, keep looking up".
        parent = tkinter.Frame(self.root)
        target = Target(parent, self.log)
        child = tkinter.Frame(parent)
        child.dnd_accept = lambda source, event: None
        self.canvas.winfo_containing = lambda x, y: child
        handler = dnd.dnd_start(self.source, FakeEvent(self.canvas))
        handler.on_motion(FakeEvent(self.canvas))
        self.assertIs(handler.target, target)
        handler.cancel()


if __name__ == "__main__":
    unittest.main()
