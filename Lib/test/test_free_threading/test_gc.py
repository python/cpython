import unittest

import threading
from threading import Thread
import time
from unittest import TestCase
import gc
import weakref

from test import support
from test.support import threading_helper

try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None


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

    def test_merge_brc_queue_of_detached_thread(self):
        # GH-157838: objects queued for merging by a thread that is detached
        # (blocked in a lock acquire, sleep, etc.) are merged and freed on its
        # behalf instead of staying alive until it runs Python code again.
        lock = threading.Lock()
        lock.acquire()
        ready = threading.Event()
        objs = []

        def worker():
            # Objects owned by this thread; only the list holds a reference.
            objs.extend(MyObj() for _ in range(100))
            ready.set()
            lock.acquire()  # block while detached

        thread = Thread(target=worker)
        thread.start()
        try:
            ready.wait()
            # The worker may not have detached yet when the first objects
            # are dropped; keep trying until one is freed immediately.
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT, error=False):
                obj = objs.pop()
                wr = weakref.ref(obj)
                del obj
                if wr() is None:
                    break
            else:
                self.fail("object not freed while owning thread was detached")
        finally:
            lock.release()
            thread.join()

    def test_gc_callbacks_race_with_mutation(self):
        def collect():
            b.wait()
            while not stop.is_set():
                gc.collect()

        def mutate():
            b.wait()
            while not stop.is_set():
                gc.callbacks[:] = [lambda *_: _ for _ in range(16)]
                time.sleep(0)
                gc.callbacks.clear()

        threads = [threading.Thread(target=f) for f in (collect, mutate) * 4]
        b = threading.Barrier(len(threads) + 1)
        stop = threading.Event()

        with threading_helper.start_threads(threads, stop.set):
            b.wait()
            time.sleep(0.2)
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


def allocate_and_free():
    # Free enough small objects to leave many mimalloc pages empty. Empty
    # pages in the object heaps wait on QSBR before they are freed.
    junk = [[] for _ in range(100_000)]
    del junk


@threading_helper.requires_working_threading()
@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
class TestQSBRPageList(TestCase):
    # gh-158972: gc.collect() should free empty pages waiting on QSBR, even
    # if the thread that parked them is idle.

    def parked_pages(self):
        return _testinternalcapi.get_qsbr_page_list_length()

    def test_gc_collect_frees_parked_pages(self):
        gc.collect()
        allocate_and_free()
        parked = self.parked_pages()
        if parked < 10:
            self.skipTest(f"only {parked} pages were parked")
        gc.collect()
        self.assertLess(self.parked_pages(), parked // 4)

    def test_gc_collect_frees_pages_parked_by_idle_thread(self):
        done = threading.Lock()
        done.acquire()
        resume = threading.Lock()
        resume.acquire()

        def worker():
            allocate_and_free()
            done.release()
            # Block without allocating so the worker doesn't free its parked
            # pages itself.
            resume.acquire()

        t = Thread(target=worker)
        t.start()
        try:
            done.acquire()
            parked = self.parked_pages()
            if parked < 10:
                self.skipTest(f"only {parked} pages were parked")
            gc.collect()
            self.assertLess(self.parked_pages(), parked // 4)
        finally:
            resume.release()
            t.join()

    def test_gc_collect_while_starting_threads(self):
        # A new thread is added to the interpreter before it initializes its
        # mimalloc state, so the GC can see thread states without a usable
        # page list.
        stop = threading.Event()

        def collector():
            while not stop.is_set():
                gc.collect()

        deadline = time.monotonic() + 2.0

        def starter():
            while time.monotonic() < deadline:
                threads = [Thread(target=lambda: None) for _ in range(16)]
                with threading_helper.start_threads(threads):
                    pass

        c = Thread(target=collector)
        c.start()
        try:
            starters = [Thread(target=starter) for _ in range(4)]
            with threading_helper.start_threads(starters):
                pass
        finally:
            stop.set()
            c.join()


if __name__ == "__main__":
    unittest.main()
