import gc
import time
import unittest
import weakref

from ast import Or
from functools import partial
from threading import Barrier, Thread
from unittest import TestCase

try:
    import _testcapi
except ImportError:
    _testcapi = None

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestDict(TestCase):
    def test_racing_creation_shared_keys(self):
        """Verify that creating dictionaries is thread safe when we
        have a type with shared keys"""
        class C(int):
            pass

        self.racing_creation(C)

    def test_racing_creation_no_shared_keys(self):
        """Verify that creating dictionaries is thread safe when we
        have a type with an ordinary dict"""
        self.racing_creation(Or)

    def test_racing_creation_inline_values_invalid(self):
        """Verify that re-creating a dict after we have invalid inline values
        is thread safe"""
        class C:
            pass

        def make_obj():
            a = C()
            # Make object, make inline values invalid, and then delete dict
            a.__dict__ = {}
            del a.__dict__
            return a

        self.racing_creation(make_obj)

    def test_racing_creation_nonmanaged_dict(self):
        """Verify that explicit creation of an unmanaged dict is thread safe
        outside of the normal attribute setting code path"""
        def make_obj():
            def f(): pass
            return f

        def set(func, name, val):
            # Force creation of the dict via PyObject_GenericGetDict
            func.__dict__[name] = val

        self.racing_creation(make_obj, set)

    def racing_creation(self, cls, set=setattr):
        objects = []
        processed = []

        OBJECT_COUNT = 100
        THREAD_COUNT = 10
        CUR = 0

        for i in range(OBJECT_COUNT):
            objects.append(cls())

        def writer_func(name):
            last = -1
            while True:
                if CUR == last:
                    time.sleep(0.001)
                    continue
                elif CUR == OBJECT_COUNT:
                    break

                obj = objects[CUR]
                set(obj, name, name)
                last = CUR
                processed.append(name)

        writers = []
        for x in range(THREAD_COUNT):
            writer = Thread(target=partial(writer_func, f"a{x:02}"))
            writers.append(writer)
            writer.start()

        for i in range(OBJECT_COUNT):
            CUR = i
            while len(processed) != THREAD_COUNT:
                time.sleep(0.001)
            processed.clear()

        CUR = OBJECT_COUNT

        for writer in writers:
            writer.join()

        for obj_idx, obj in enumerate(objects):
            assert (
                len(obj.__dict__) == THREAD_COUNT
            ), f"{len(obj.__dict__)} {obj.__dict__!r} {obj_idx}"
            for i in range(THREAD_COUNT):
                assert f"a{i:02}" in obj.__dict__, f"a{i:02} missing at {obj_idx}"

    def test_racing_set_dict(self):
        """Races assigning to __dict__ should be thread safe"""

        def f(): pass
        l = []
        THREAD_COUNT = 10
        class MyDict(dict): pass

        def writer_func(l):
            for i in range(1000):
                d = MyDict()
                l.append(weakref.ref(d))
                f.__dict__ = d

        lists = []
        writers = []
        for x in range(THREAD_COUNT):
            thread_list = []
            lists.append(thread_list)
            writer = Thread(target=partial(writer_func, thread_list))
            writers.append(writer)

        for writer in writers:
            writer.start()

        for writer in writers:
            writer.join()

        f.__dict__ = {}
        gc.collect()

        for thread_list in lists:
            for ref in thread_list:
                self.assertIsNone(ref())

    def test_racing_get_set_dict(self):
        """Races getting and setting a dict should be thread safe"""
        THREAD_COUNT = 10
        barrier = Barrier(THREAD_COUNT)
        def work(d):
            barrier.wait()
            for _ in range(1000):
                d[10] = 0
                d.get(10, None)
                _ = d[10]

        d = {}
        worker_threads = []
        for ii in range(THREAD_COUNT):
            worker_threads.append(Thread(target=work, args=[d]))
        for t in worker_threads:
            t.start()
        for t in worker_threads:
            t.join()


    def test_racing_set_object_dict(self):
        """Races assigning to __dict__ should be thread safe"""
        class C: pass
        class MyDict(dict): pass
        for cyclic in (False, True):
            f = C()
            f.__dict__ = {"foo": 42}
            THREAD_COUNT = 10

            def writer_func(l):
                for i in range(1000):
                    if cyclic:
                        other_d = {}
                    d = MyDict({"foo": 100})
                    if cyclic:
                        d["x"] = other_d
                        other_d["bar"] = d
                    l.append(weakref.ref(d))
                    f.__dict__ = d

            def reader_func():
                for i in range(1000):
                    f.foo

            lists = []
            readers = []
            writers = []
            for x in range(THREAD_COUNT):
                thread_list = []
                lists.append(thread_list)
                writer = Thread(target=partial(writer_func, thread_list))
                writers.append(writer)

            for x in range(THREAD_COUNT):
                reader = Thread(target=partial(reader_func))
                readers.append(reader)

            for writer in writers:
                writer.start()
            for reader in readers:
                reader.start()

            for writer in writers:
                writer.join()

            for reader in readers:
                reader.join()

            f.__dict__ = {}
            gc.collect()
            gc.collect()

            count = 0
            ids = set()
            for thread_list in lists:
                for i, ref in enumerate(thread_list):
                    if ref() is None:
                        continue
                    count += 1
                    ids.add(id(ref()))
                    count += 1

            self.assertEqual(count, 0)


if __name__ == "__main__":
    unittest.main()
