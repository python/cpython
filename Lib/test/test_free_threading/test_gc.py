import unittest

import threading
from threading import Thread
from unittest import TestCase
import gc

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


if __name__ == "__main__":
    unittest.main()
