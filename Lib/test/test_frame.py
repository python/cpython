import gc
import sys
import unittest
import weakref

from test import support


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

    def test_clear_generator(self):
        endly = False
        def g():
            nonlocal endly
            try:
                yield
                inner()
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


def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
