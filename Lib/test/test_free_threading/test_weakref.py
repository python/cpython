"""Regression tests for weakref races in subtype_getweakref() (gh-149816)."""

import gc
import sys
import unittest
import weakref

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestWeakrefRaces(unittest.TestCase):

    ITERATIONS = 5_000

    def test_getweakref_no_crash(self):
        """obj.__weakref__ racing with concurrent weakref creation/destruction."""
        class Target:
            pass

        obj = Target()

        def reader():
            for _ in range(self.ITERATIONS):
                ref = obj.__weakref__
                self.assertIn(type(ref), (type(None), weakref.ref))
                if ref is not None:
                    self.assertIn(ref(), (None, obj))

        def mutator():
            for _ in range(self.ITERATIONS):
                weakref.ref(obj, lambda _: None)

        threading_helper.run_concurrently([reader, mutator])

    def test_getweakref_return_type(self):
        """__weakref__ must return None or a live weakref.ref."""
        class Target:
            pass

        obj = Target()

        self.assertIsNone(obj.__weakref__)

        ref = weakref.ref(obj)
        result = obj.__weakref__
        self.assertIsInstance(result, weakref.ref)
        self.assertIs(result(), obj)

        del ref, result
        gc.collect()
        self.assertIsNone(obj.__weakref__)

    def test_getweakref_no_refcount_leak(self):
        """Each __weakref__ access must not inflate the weakref's refcount."""
        class Target:
            pass

        obj = Target()
        ref = weakref.ref(obj)
        before = sys.getrefcount(ref)

        for _ in range(1_000):
            r = obj.__weakref__
            del r

        gc.collect()
        self.assertEqual(sys.getrefcount(ref), before)

    def test_getweakref_many_readers(self):
        """Multiple concurrent readers and mutators must not crash."""
        class Target:
            pass

        obj = Target()
        n = 4
        iters = self.ITERATIONS // n

        def reader():
            for _ in range(iters):
                ref = obj.__weakref__
                self.assertIn(type(ref), (type(None), weakref.ref))

        def mutator():
            for _ in range(iters):
                weakref.ref(obj, lambda _: None)

        threading_helper.run_concurrently([reader] * n + [mutator] * n)


if __name__ == "__main__":
    unittest.main()
