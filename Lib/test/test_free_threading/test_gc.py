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
        event = threading.Event()

        obj = MyObj()

        def gc_thread():
            for i in range(100):
                o = gc.get_referrers(obj)
            event.set()

        def mutator_thread():
            while not event.is_set():
                d1 = { "key": obj }
                d2 = { "key": obj }
                d3 = { "key": obj }
                d4 = { "key": obj }

        gcs = [Thread(target=gc_thread) for _ in range(2)]
        mutators = [Thread(target=mutator_thread) for _ in range(4)]
        with threading_helper.start_threads(gcs + mutators):
            pass


if __name__ == "__main__":
    unittest.main()
