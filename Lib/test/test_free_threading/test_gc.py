import unittest

import subprocess
import sys
import textwrap
import threading
from threading import Thread
from unittest import TestCase
import gc

from test import support
from test.support import threading_helper


class MyObj:
    pass


@threading_helper.requires_working_threading()
class TestGC(TestCase):
    def test_get_objects(self):
        event = threading.Event()

        def gc_thread():
            for i in range(100):
                o = gc.get_objects()
            event.set()

        def mutator_thread():
            while not event.is_set():
                o1 = MyObj()
                o2 = MyObj()
                o3 = MyObj()
                o4 = MyObj()

        gcs = [Thread(target=gc_thread)]
        mutators = [Thread(target=mutator_thread) for _ in range(4)]
        with threading_helper.start_threads(gcs + mutators):
            pass

    def test_get_referrers(self):
        NUM_GC = 2
        NUM_MUTATORS = 4

        b = threading.Barrier(NUM_GC + NUM_MUTATORS)
        event = threading.Event()

        obj = MyObj()

        def gc_thread():
            b.wait()
            for i in range(100):
                o = gc.get_referrers(obj)
            event.set()

        def mutator_thread():
            b.wait()
            while not event.is_set():
                d1 = { "key": obj }
                d2 = { "key": obj }
                d3 = { "key": obj }
                d4 = { "key": obj }

        gcs = [Thread(target=gc_thread) for _ in range(NUM_GC)]
        mutators = [Thread(target=mutator_thread) for _ in range(NUM_MUTATORS)]
        with threading_helper.start_threads(gcs + mutators):
            pass

    def test_freeze_object_in_brc_queue(self):
        # GH-142975: Freezing objects in the BRC queue could result in some
        # objects having a zero refcount without being deallocated.

        class Weird:
            # We need a destructor to trigger the check for object resurrection
            def __del__(self):
                pass

        # This is owned by the main thread, so the subthread will have to increment
        # this object's reference count.
        weird = Weird()

        def evil():
            gc.freeze()

            # Decrement the reference count from this thread, which will trigger the
            # slow path during resurrection and add our weird object to the BRC queue.
            nonlocal weird
            del weird

            # Collection will merge the object's reference count and make it zero.
            gc.collect()

            # Unfreeze the object, making it visible to the GC.
            gc.unfreeze()
            gc.collect()

        thread = Thread(target=evil)
        thread.start()
        thread.join()

    def test_set_threshold(self):
        # GH-148613: Setting the GC threshold from another thread could cause a
        # race between the `gc_should_collect` and `gc_set_threshold` functions.
        NUM_THREADS = 8
        NUM_ITERS = 100_000
        barrier = threading.Barrier(NUM_THREADS)

        class CyclicReference:
            def __init__(self):
                self.r = self

        def allocator():
            barrier.wait()
            for _ in range(NUM_ITERS):
                CyclicReference()

        def setter():
            barrier.wait()
            for i in range(NUM_ITERS):
                gc.set_threshold(100 + (i % 100), 10 + (i % 10), 10 + (i % 10))

        current_threshold = gc.get_threshold()
        try:
            threads = [Thread(target=allocator) for _ in range(NUM_THREADS - 1)]
            threads.append(Thread(target=setter))
            with threading_helper.start_threads(threads):
                pass
        finally:
            gc.set_threshold(*current_threshold)

    def test_get_count(self):
        class CyclicReference:
            def __init__(self):
                self.ref = self

        NUM_ALLOCATORS = 7
        NUM_READERS = 1
        NUM_THREADS = NUM_ALLOCATORS + NUM_READERS
        NUM_ITERS = 1000

        barrier = threading.Barrier(NUM_THREADS)

        def allocator():
            barrier.wait()
            for _ in range(NUM_ITERS):
                CyclicReference()


        def reader():
            barrier.wait()
            for _ in range(NUM_ITERS):
                gc.get_count()

        threads = [Thread(target=allocator) for _ in range(NUM_ALLOCATORS)]
        threads.extend(Thread(target=reader) for _ in range(NUM_READERS))

        with threading_helper.start_threads(threads):
            pass

    @support.requires_subprocess()
    def test_tight_gc_loop_does_not_starve_attach(self):
        script = textwrap.dedent("""
            import gc
            import importlib
            import threading
            import time

            modules = (
                "abc", "argparse", "collections", "contextlib",
                "decimal", "enum", "functools", "heapq",
                "importlib", "inspect", "itertools", "json",
                "math", "operator", "random", "re",
            )
            for name in modules:
                importlib.import_module(name)

            stop = threading.Event()

            def collect():
                while not stop.is_set():
                    gc.collect()

            thread = threading.Thread(target=collect, daemon=True)
            thread.start()
            time.sleep(1.0)
            stop.set()
            thread.join(5.0)
            if thread.is_alive():
                raise SystemExit("GC thread did not stop")
        """)
        proc = subprocess.run(
            [sys.executable, "-I", "-X", "faulthandler", "-c", script],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=support.SHORT_TIMEOUT,
        )
        self.assertEqual(
            proc.returncode,
            0,
            f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}",
        )


if __name__ == "__main__":
    unittest.main()
