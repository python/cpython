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

    def test_racing_object_get_set_dict(self):
        e = Exception()

        def writer():
            for i in range(10000):
                e.__dict__ = {1:2}

        def reader():
            for i in range(10000):
                e.__dict__

        t1 = Thread(target=writer)
        t2 = Thread(target=reader)

        with threading_helper.start_threads([t1, t2]):
            pass

    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_racing_watch_unwatch_dict(self):
        # gh-148393: race between PyDict_Watch / PyDict_Unwatch
        # and concurrent dict mutation reading _ma_watcher_tag.
        wid = _testcapi.add_dict_watcher(0)
        try:
            d = {}
            ITERS = 1000

            def writer():
                for i in range(ITERS):
                    d[i] = i
                    del d[i]

            def watcher():
                for _ in range(ITERS):
                    _testcapi.watch_dict(wid, d)
                    _testcapi.unwatch_dict(wid, d)

            threading_helper.run_concurrently([writer, watcher])
        finally:
            _testcapi.clear_dict_watcher(wid)

    def test_racing_split_dict_clear_and_lookup(self):
        class C:
            pass

        keys = [f"a{i}" for i in range(16)]

        def make_split_nonembedded():
            inst = C()
            for key in keys:
                setattr(inst, key, keys.index(key))
            # dict.copy() of a split instance dict yields a split table
            # with non-embedded values
            return inst.__dict__.copy()

        d = make_split_nonembedded()

        def clearer():
            for _ in range(1000):
                d.clear()
                d.update(make_split_nonembedded())

        def reader():
            for _ in range(1000):
                for k in keys:
                    d.get(k)

        threading_helper.run_concurrently([clearer, reader, reader])

    def test_racing_embedded_values_clear_and_lookup(self):
        class C:
            pass

        obj = C()
        def writer():
            for _ in range(1000):
                obj.x = 1
                obj.y = 2
                obj.z = 3
                obj.__dict__.clear()

        def reader():
            for _ in range(1000):
                obj.__dict__.get('x')

        threading_helper.run_concurrently([writer, reader, reader])

    def test_racing_dict_update_and_method_lookup(self):
        # gh-144295: test race between dict modifications and method lookups.
        # Uses BytesIO because the race requires a type without Py_TPFLAGS_INLINE_VALUES
        # for the _PyDict_GetMethodStackRef code path.
        import io
        obj = io.BytesIO()

        def writer():
            for _ in range(10000):
                obj.x = 1
                del obj.x

        def reader():
            for _ in range(10000):
                obj.getvalue()

        t1 = Thread(target=writer)
        t2 = Thread(target=reader)

        with threading_helper.start_threads([t1, t2]):
            pass


@threading_helper.requires_working_threading()
class TestDictDeadlock(TestCase):
    """Regression test for the free-threaded deadlock fixed in gh-151593.

    The deadlock needs several threads and was only triggered intermittently by
    normal test runs (e.g. ``test_abc --parallel-threads=10``); this test sets up
    the required roles directly so a regression shows up reliably.

    The bug: ``insert_split_key()`` called ``PyType_Modified()`` while still holding
    the dictobject.c shared-keys lock (``LOCK_KEYS``).  ``PyType_Modified()`` wants
    the typeobject.c ``TYPE_LOCK``, so a thread could hold ``LOCK_KEYS`` and block on
    ``TYPE_LOCK`` -- a lock-ordering inversion.  The fix moves the
    ``PyType_Modified()`` call to after ``LOCK_KEYS`` is released.

    The cycle that hangs:

      * one mutator thread holds ``LOCK_KEYS`` and waits for ``TYPE_LOCK``
        (``insert_split_key`` -> ``PyType_Modified``);
      * the "stopper" thread holds ``TYPE_LOCK`` and runs ``types_stop_world()``
        (here via assigning ``__abstractmethods__``), waiting for every thread to
        reach a safe point;
      * the remaining mutator threads are parked acquiring ``LOCK_KEYS`` without
        detaching, so they never reach a safe point and the world never stops.

    The stopper therefore never finishes and never drops ``TYPE_LOCK``, the first
    mutator never makes progress, and ``LOCK_KEYS`` is never released.  Several
    mutator threads are required so that some are blocked on ``LOCK_KEYS`` while
    another holds it.
    """

    # Each round operates on one fresh class so that ``insert_split_key()`` keeps
    # taking the slow path that inserts a brand new shared key.
    NROUNDS = 100

    # Mutator threads racing to insert new shared keys / allocate inline values.
    NMUTATORS = 4

    def test_insert_split_key_vs_type_modified(self):
        box = [None]
        # +1 for the type-modifying "stopper" thread, +1 for the main thread
        # which produces a fresh class for each round.
        nparties = self.NMUTATORS + 2
        round_start = Barrier(nparties)
        round_end = Barrier(nparties)

        def fresh_class():
            class C:
                pass

            # Prime the type: give it a non-zero tp_version_tag (so that the
            # later PyType_Modified() actually takes the type lock instead of
            # returning early) and create its shared keys / inline values.
            c = C()
            c.warm = 0
            getattr(c, "warm")
            return C

        def mutator(tid):
            for _ in range(self.NROUNDS):
                round_start.wait()
                cls = box[0]
                # Allocating exercises _PyObject_InitInlineValues (LOCK_KEYS);
                # setting fresh attributes exercises insert_split_key, whose
                # first insertion invalidates the type while holding LOCK_KEYS.
                obj = cls()
                setattr(obj, f"x{tid}_a", 1)
                setattr(obj, f"x{tid}_b", 2)
                setattr(obj, f"x{tid}_c", 3)
                round_end.wait()

        def stopper():
            for _ in range(self.NROUNDS):
                round_start.wait()
                cls = box[0]
                # type_set_abstractmethods() holds the type lock and calls
                # types_stop_world().  An empty set keeps the class
                # instantiable while still stopping the world.
                cls.__abstractmethods__ = frozenset()
                cls.__abstractmethods__ = frozenset()
                cls.__abstractmethods__ = frozenset()
                round_end.wait()

        with threading_helper.catch_threading_exception() as cm:
            threads = [
                Thread(target=mutator, args=(i,))
                for i in range(self.NMUTATORS)
            ]
            threads.append(Thread(target=stopper))
            for t in threads:
                t.start()

            for r in range(self.NROUNDS):
                box[0] = fresh_class()
                round_start.wait()
                round_end.wait()

            for t in threads:
                t.join()

            self.assertIsNone(cm.exc_value)


if __name__ == "__main__":
    unittest.main()
