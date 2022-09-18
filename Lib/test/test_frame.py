import re
import sys
import textwrap
import types
import unittest
import weakref

from test import support
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
        # Clearing the frame closes the generator
        gen.gi_frame.clear()
        self.assertTrue(endly)

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
        # Clearing the frame closes the generator
        f.clear()
        self.assertTrue(endly)

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

if __name__ == "__main__":
    unittest.main()
