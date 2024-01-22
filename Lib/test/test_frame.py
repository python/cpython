import gc
import operator
import re
import sys
import textwrap
import threading
import types
import unittest
import weakref
try:
    import _testcapi
except ImportError:
    _testcapi = None

from test import support
from test.support import threading_helper
from test.support.script_helper import assert_python_ok


class ClearTest(unittest.TestCase):
    """
    Tests for frame.clear().
    """

    def inner(self, x=5, **kwargs):
        1/0

    def outer(self, **kwargs):
        try:
            self.inner(**kwargs)
        except ZeroDivisionError as e:
            exc = e
        return exc

    def clear_traceback_frames(self, tb):
        """
        Clear all frames in a traceback.
        """
        while tb is not None:
            tb.tb_frame.clear()
            tb = tb.tb_next

    def test_clear_locals(self):
        class C:
            pass
        c = C()
        wr = weakref.ref(c)
        exc = self.outer(c=c)
        del c
        support.gc_collect()
        # A reference to c is held through the frames
        self.assertIsNot(None, wr())
        self.clear_traceback_frames(exc.__traceback__)
        support.gc_collect()
        # The reference was released by .clear()
        self.assertIs(None, wr())

    def test_clear_does_not_clear_specials(self):
        class C:
            pass
        c = C()
        exc = self.outer(c=c)
        del c
        f = exc.__traceback__.tb_frame
        f.clear()
        self.assertIsNot(f.f_code, None)
        self.assertIsNot(f.f_locals, None)
        self.assertIsNot(f.f_builtins, None)
        self.assertIsNot(f.f_globals, None)

    def test_clear_generator(self):
        endly = False
        def g():
            nonlocal endly
            try:
                yield
                self.inner()
            finally:
                endly = True
        gen = g()
        next(gen)
        self.assertFalse(endly)

        # Cannot clear a suspended frame
        with self.assertRaisesRegex(RuntimeError, r'suspended frame'):
            gen.gi_frame.clear()
        self.assertFalse(endly)

    def test_clear_executing(self):
        # Attempting to clear an executing frame is forbidden.
        try:
            1/0
        except ZeroDivisionError as e:
            f = e.__traceback__.tb_frame
        with self.assertRaises(RuntimeError):
            f.clear()
        with self.assertRaises(RuntimeError):
            f.f_back.clear()

    def test_clear_executing_generator(self):
        # Attempting to clear an executing generator frame is forbidden.
        endly = False
        def g():
            nonlocal endly
            try:
                1/0
            except ZeroDivisionError as e:
                f = e.__traceback__.tb_frame
                with self.assertRaises(RuntimeError):
                    f.clear()
                with self.assertRaises(RuntimeError):
                    f.f_back.clear()
                yield f
            finally:
                endly = True
        gen = g()
        f = next(gen)
        self.assertFalse(endly)
        # Cannot clear a suspended frame
        with self.assertRaisesRegex(RuntimeError, 'suspended frame'):
            f.clear()
        self.assertFalse(endly)

    def test_lineno_with_tracing(self):
        def record_line():
            f = sys._getframe(1)
            lines.append(f.f_lineno-f.f_code.co_firstlineno)

        def test(trace):
            record_line()
            if trace:
                sys._getframe(0).f_trace = True
            record_line()
            record_line()

        expected_lines = [1, 4, 5]
        lines = []
        test(False)
        self.assertEqual(lines, expected_lines)
        lines = []
        test(True)
        self.assertEqual(lines, expected_lines)

    @support.cpython_only
    def test_clear_refcycles(self):
        # .clear() doesn't leave any refcycle behind
        with support.disable_gc():
            class C:
                pass
            c = C()
            wr = weakref.ref(c)
            exc = self.outer(c=c)
            del c
            self.assertIsNot(None, wr())
            self.clear_traceback_frames(exc.__traceback__)
            self.assertIs(None, wr())


class FrameAttrsTest(unittest.TestCase):

    def make_frames(self):
        def outer():
            x = 5
            y = 6
            def inner():
                z = x + 2
                1/0
                t = 9
            return inner()
        try:
            outer()
        except ZeroDivisionError as e:
            tb = e.__traceback__
            frames = []
            while tb:
                frames.append(tb.tb_frame)
                tb = tb.tb_next
        return frames

    def test_locals(self):
        f, outer, inner = self.make_frames()
        outer_locals = outer.f_locals
        self.assertIsInstance(outer_locals.pop('inner'), types.FunctionType)
        self.assertEqual(outer_locals, {'x': 5, 'y': 6})
        inner_locals = inner.f_locals
        self.assertEqual(inner_locals, {'x': 5, 'z': 7})

    def test_clear_locals(self):
        # Test f_locals after clear() (issue #21897)
        f, outer, inner = self.make_frames()
        outer.clear()
        inner.clear()
        self.assertEqual(outer.f_locals, {})
        self.assertEqual(inner.f_locals, {})

    def test_locals_clear_locals(self):
        # Test f_locals before and after clear() (to exercise caching)
        f, outer, inner = self.make_frames()
        outer.f_locals
        inner.f_locals
        outer.clear()
        inner.clear()
        self.assertEqual(outer.f_locals, {})
        self.assertEqual(inner.f_locals, {})

    def test_f_lineno_del_segfault(self):
        f, _, _ = self.make_frames()
        with self.assertRaises(AttributeError):
            del f.f_lineno


class ReprTest(unittest.TestCase):
    """
    Tests for repr(frame).
    """

    def test_repr(self):
        def outer():
            x = 5
            y = 6
            def inner():
                z = x + 2
                1/0
                t = 9
            return inner()

        offset = outer.__code__.co_firstlineno
        try:
            outer()
        except ZeroDivisionError as e:
            tb = e.__traceback__
            frames = []
            while tb:
                frames.append(tb.tb_frame)
                tb = tb.tb_next
        else:
            self.fail("should have raised")

        f_this, f_outer, f_inner = frames
        file_repr = re.escape(repr(__file__))
        self.assertRegex(repr(f_this),
                         r"^<frame at 0x[0-9a-fA-F]+, file %s, line %d, code test_repr>$"
                         % (file_repr, offset + 23))
        self.assertRegex(repr(f_outer),
                         r"^<frame at 0x[0-9a-fA-F]+, file %s, line %d, code outer>$"
                         % (file_repr, offset + 7))
        self.assertRegex(repr(f_inner),
                         r"^<frame at 0x[0-9a-fA-F]+, file %s, line %d, code inner>$"
                         % (file_repr, offset + 5))

class TestIncompleteFrameAreInvisible(unittest.TestCase):

    def test_issue95818(self):
        # See GH-95818 for details
        code = textwrap.dedent(f"""
            import gc

            gc.set_threshold(1,1,1)
            class GCHello:
                def __del__(self):
                    print("Destroyed from gc")

            def gen():
                yield

            fd = open({__file__!r})
            l = [fd, GCHello()]
            l.append(l)
            del fd
            del l
            gen()
        """)
        assert_python_ok("-c", code)

    @support.cpython_only
    def test_sneaky_frame_object(self):

        def trace(frame, event, arg):
            """
            Don't actually do anything, just force a frame object to be created.
            """

        def callback(phase, info):
            """
            Yo dawg, I heard you like frames, so I'm allocating a frame while
            you're allocating a frame, so you can have a frame while you have a
            frame!
            """
            nonlocal sneaky_frame_object
            sneaky_frame_object = sys._getframe().f_back.f_back
            # We're done here:
            gc.callbacks.remove(callback)

        def f():
            while True:
                yield

        old_threshold = gc.get_threshold()
        old_callbacks = gc.callbacks[:]
        old_enabled = gc.isenabled()
        old_trace = sys.gettrace()
        try:
            # Stop the GC for a second while we set things up:
            gc.disable()
            # Create a paused generator:
            g = f()
            next(g)
            # Move all objects to the oldest generation, and tell the GC to run
            # on the *very next* allocation:
            gc.collect()
            gc.set_threshold(1, 0, 0)
            # Okay, so here's the nightmare scenario:
            # - We're tracing the resumption of a generator, which creates a new
            #   frame object.
            # - The allocation of this frame object triggers a collection
            #   *before* the frame object is actually created.
            # - During the collection, we request the exact same frame object.
            #   This test does it with a GC callback, but in real code it would
            #   likely be a trace function, weakref callback, or finalizer.
            # - The collection finishes, and the original frame object is
            #   created. We now have two frame objects fighting over ownership
            #   of the same interpreter frame!
            sys.settrace(trace)
            gc.callbacks.append(callback)
            sneaky_frame_object = None
            gc.enable()
            next(g)
            # g.gi_frame should be the frame object from the callback (the
            # one that was *requested* second, but *created* first):
            self.assertIs(g.gi_frame, sneaky_frame_object)
        finally:
            gc.set_threshold(*old_threshold)
            gc.callbacks[:] = old_callbacks
            sys.settrace(old_trace)
            if old_enabled:
                gc.enable()

    @support.cpython_only
    @threading_helper.requires_working_threading()
    def test_sneaky_frame_object_teardown(self):

        class SneakyDel:
            def __del__(self):
                """
                Stash a reference to the entire stack for walking later.

                It may look crazy, but you'd be surprised how common this is
                when using a test runner (like pytest). The typical recipe is:
                ResourceWarning + -Werror + a custom sys.unraisablehook.
                """
                nonlocal sneaky_frame_object
                sneaky_frame_object = sys._getframe()

        class SneakyThread(threading.Thread):
            """
            A separate thread isn't needed to make this code crash, but it does
            make crashes more consistent, since it means sneaky_frame_object is
            backed by freed memory after the thread completes!
            """

            def run(self):
                """Run SneakyDel.__del__ as this frame is popped."""
                ref = SneakyDel()

        sneaky_frame_object = None
        t = SneakyThread()
        t.start()
        t.join()
        # sneaky_frame_object can be anything, really, but it's crucial that
        # SneakyThread.run's frame isn't anywhere on the stack while it's being
        # torn down:
        self.assertIsNotNone(sneaky_frame_object)
        while sneaky_frame_object is not None:
            self.assertIsNot(
                sneaky_frame_object.f_code, SneakyThread.run.__code__
            )
            sneaky_frame_object = sneaky_frame_object.f_back

    def test_entry_frames_are_invisible_during_teardown(self):
        class C:
            """A weakref'able class."""

        def f():
            """Try to find globals and locals as this frame is being cleared."""
            ref = C()
            # Ignore the fact that exec(C()) is a nonsense callback. We're only
            # using exec here because it tries to access the current frame's
            # globals and locals. If it's trying to get those from a shim frame,
            # we'll crash before raising:
            return weakref.ref(ref, exec)

        with support.catch_unraisable_exception() as catcher:
            # Call from C, so there is a shim frame directly above f:
            weak = operator.call(f)  # BOOM!
            # Cool, we didn't crash. Check that the callback actually happened:
            self.assertIs(catcher.unraisable.exc_type, TypeError)
        self.assertIsNone(weak())

@unittest.skipIf(_testcapi is None, 'need _testcapi')
class TestCAPI(unittest.TestCase):
    def getframe(self):
        return sys._getframe()

    def test_frame_getters(self):
        frame = self.getframe()
        self.assertEqual(frame.f_locals, _testcapi.frame_getlocals(frame))
        self.assertIs(frame.f_globals, _testcapi.frame_getglobals(frame))
        self.assertIs(frame.f_builtins, _testcapi.frame_getbuiltins(frame))
        self.assertEqual(frame.f_lasti, _testcapi.frame_getlasti(frame))

    def test_getvar(self):
        current_frame = sys._getframe()
        x = 1
        self.assertEqual(_testcapi.frame_getvar(current_frame, "x"), 1)
        self.assertEqual(_testcapi.frame_getvarstring(current_frame, b"x"), 1)
        with self.assertRaises(NameError):
            _testcapi.frame_getvar(current_frame, "y")
        with self.assertRaises(NameError):
            _testcapi.frame_getvarstring(current_frame, b"y")

        # wrong name type
        with self.assertRaises(TypeError):
            _testcapi.frame_getvar(current_frame, b'x')
        with self.assertRaises(TypeError):
            _testcapi.frame_getvar(current_frame, 123)

    def getgenframe(self):
        yield sys._getframe()

    def test_frame_get_generator(self):
        gen = self.getgenframe()
        frame = next(gen)
        self.assertIs(gen, _testcapi.frame_getgenerator(frame))

    def test_frame_fback_api(self):
        """Test that accessing `f_back` does not cause a segmentation fault on
        a frame created with `PyFrame_New` (GH-99110)."""
        def dummy():
            pass

        frame = _testcapi.frame_new(dummy.__code__, globals(), locals())
        # The following line should not cause a segmentation fault.
        self.assertIsNone(frame.f_back)

if __name__ == "__main__":
    unittest.main()
