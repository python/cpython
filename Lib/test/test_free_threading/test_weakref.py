#!/usr/bin/env python3
"""
    Free-threading regression tests for weakref races in subtype_getweakref().

    Regression test for https://github.com/python/cpython/issues/149816
    finding #61: "Racy weakref head load before incref in Objects/typeobject.c".

    subtype_getweakref() is the C implementation of the obj.__weakref__
    descriptor getter (Objects/typeobject.c).  Before the fix it performed an
    unsynchronized load of *weaklistptr followed by Py_NewRef(), creating a
    use-after-free window: a concurrent weakref_dealloc() could free the
    weakref list head between the load and the incref.

    The fix holds LOCK_WEAKREFS(obj) across the read+incref and uses
    _Py_TryIncref() instead of Py_NewRef() so that a weakref whose refcount
    has already hit zero is handled gracefully (returns None semantics).
    LOCK_WEAKREFS / UNLOCK_WEAKREFS are no-ops in GIL builds, so the fix has
    zero overhead outside the free-threaded build.
"""

import gc
import sys
import threading
import unittest
import weakref

from test.support import threading_helper


def run_in_threads(targets, *, barrier=None):
    """Start all callables in *targets* in separate threads and join them.

    If *barrier* is given it must be a threading.Barrier whose party count
    equals len(targets); each target is wrapped so that it waits at the
    barrier before doing any work, maximising contention.
    """
    if barrier is not None:
        def wrap(fn):
            def wrapper():
                barrier.wait()
                fn()
            return wrapper
        targets = [wrap(t) for t in targets]

    threads = [threading.Thread(target=t) for t in targets]
    for th in threads:
        th.start()
    for th in threads:
        th.join()


@threading_helper.requires_working_threading()
class TestWeakrefRaces(unittest.TestCase):
    """Race-condition tests for subtype_getweakref() (gh-149816 finding #61)."""

    # Number of iterations for the hot-loop race tests.  High enough to
    # exercise the race window reliably on multi-core machines, low enough
    # to keep the test suite fast (no time-based loops).
    ITERATIONS = 5_000

    def test_getweakref_no_crash(self):
        """obj.__weakref__ racing with concurrent weakref creation/destruction.

        Regression test for gh-149816 #61.  Without the fix,
        subtype_getweakref() could incref a freed PyWeakReference object
        (use-after-free), causing a native crash or memory corruption.

        The callback on weakref.ref() forces CPython to allocate a fresh
        PyWeakReference each time (instead of reusing an existing one),
        keeping list-head creation and destruction hot so the race window
        is exercised as often as possible.

        A crash here means the LOCK_WEAKREFS + _Py_TryIncref fix is broken.
        """
        class Target:
            pass

        obj = Target()
        # Barrier ensures all threads enter their hot loops simultaneously,
        # maximising the chance of hitting the race window.
        barrier = threading.Barrier(2)

        def reader():
            barrier.wait()
            for _ in range(self.ITERATIONS):
                # Calls subtype_getweakref() in C — the previously buggy path.
                ref = obj.__weakref__
                # Must be None or a weakref.ref — never corrupted memory.
                self.assertIn(
                    type(ref),
                    (type(None), weakref.ref),
                    f"__weakref__ returned unexpected type {type(ref)!r}",
                )
                # If it is a live weakref it must still point to obj.
                if ref is not None:
                    target = ref()
                    self.assertIn(
                        target,
                        (None, obj),
                        "__weakref__ returned a weakref pointing to wrong object",
                    )

        def mutator():
            barrier.wait()
            for _ in range(self.ITERATIONS):
                # Create a weakref with a callback.  Because no variable
                # holds the result the refcount drops to 0 immediately,
                # triggering weakref_dealloc() — the "freeing" side of
                # the race.  The callback prevents CPython from reusing
                # an existing weakref object, keeping alloc/free hot.
                weakref.ref(obj, lambda _: None)

        run_in_threads([reader, mutator])

    def test_getweakref_return_type(self):
        """__weakref__ must return None or a weakref.ref, never garbage.

        Verifies the correctness of subtype_getweakref() return values
        across the three possible states of the weakref list.
        """
        class Target:
            pass

        obj = Target()

        # State 1: no weakrefs exist → must return None.
        result = obj.__weakref__
        self.assertIsNone(
            result,
            "__weakref__ should be None when no weakrefs exist",
        )

        # State 2: one live weakref exists → must return it and it must
        # resolve back to obj.
        ref = weakref.ref(obj)
        result = obj.__weakref__
        self.assertIsNotNone(result, "__weakref__ should not be None with a live ref")
        self.assertIsInstance(result, weakref.ref)
        self.assertIs(
            result(),
            obj,
            "__weakref__ returned a weakref that does not point to obj",
        )

        # State 3: all strong refs to the weakref object dropped → must
        # return None again.  Both `ref` AND `result` (from State 2) must
        # be deleted — either one alone keeps the weakref alive.
        del ref, result
        gc.collect()
        result3 = obj.__weakref__
        self.assertIsNone(
            result3,
            "__weakref__ should be None after all weakrefs are deleted",
        )

    def test_getweakref_no_refcount_leak(self):
        """Each __weakref__ access must not inflate the weakref's refcount.

        A double-incref bug in subtype_getweakref() would add one extra
        reference on every call, preventing the weakref from ever being
        freed (reference leak).  sys.getrefcount() includes one transient
        reference for the function argument, so the count should be stable
        across repeated accesses.
        """
        class Target:
            pass

        obj = Target()
        ref = weakref.ref(obj)

        # Baseline: one ref from `ref`, one transient from getrefcount().
        before = sys.getrefcount(ref)

        for _ in range(1_000):
            r = obj.__weakref__
            del r   # release the returned reference immediately

        gc.collect()
        after = sys.getrefcount(ref)

        self.assertEqual(
            before,
            after,
            f"Refcount grew from {before} to {after} after 1000 "
            f"__weakref__ accesses — possible double-incref in "
            f"subtype_getweakref()",
        )

    def test_getweakref_many_readers(self):
        """Multiple concurrent readers of __weakref__ must not crash.

        Extends test_getweakref_no_crash to N reader threads + N mutator
        threads, matching the repro script's default cpu_count-per-role
        configuration, to stress true multi-core parallelism.
        """
        import os

        class Target:
            pass

        obj = Target()
        n = max(2, (os.cpu_count() or 2) // 2)   # at least 2 of each role
        barrier = threading.Barrier(n * 2)
        iters = max(1_000, self.ITERATIONS // n)  # scale down per-thread iters

        def reader():
            barrier.wait()
            for _ in range(iters):
                ref = obj.__weakref__
                self.assertIn(type(ref), (type(None), weakref.ref))

        def mutator():
            barrier.wait()
            for _ in range(iters):
                weakref.ref(obj, lambda _: None)

        run_in_threads([reader] * n + [mutator] * n)


if __name__ == "__main__":
    unittest.main()
