import gc
import threading
import unittest
import weakref

from concurrent.futures import ThreadPoolExecutor
from threading import Barrier, Thread
from unittest import TestCase
import sys
from test.support import import_helper, threading_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")



NTHREADS = 6
BOTTOM = 0
TOP = 1000
ITERS = 100

class A:
    attr = 1

def world_stoppers():
    """Finite workloads that contend for the stop-the-world mutex.

    Waiting for the mutex releases the type lock, allowing queued type
    updates to go stale.  Use a short pause rather than full collections
    to prolong the contention without depending on the size of the heap.
    """
    def stopper():
        for _ in range(100):
            _testinternalcapi.stop_the_world()

    def pauser():
        for _ in range(100):
            _testinternalcapi.stop_the_world(0.0001)

    return [stopper, pauser]


@threading_helper.requires_working_threading()
class TestType(TestCase):
    def test_attr_cache(self):
        def read(id0):
            for _ in range(ITERS):
                for _ in range(BOTTOM, TOP):
                    A.attr

        def write(id0):
            for _ in range(ITERS):
                for _ in range(BOTTOM, TOP):
                    # Make _PyType_Lookup cache hot first
                    A.attr
                    A.attr
                    x = A.attr
                    x += 1
                    A.attr = x


        with ThreadPoolExecutor(NTHREADS) as pool:
            pool.submit(read, (1,))
            pool.submit(write, (1,))
            pool.shutdown(wait=True)

    def test_attr_cache_consistency(self):
        class C:
            x = 0

        def writer_func():
            for _ in range(3000):
                C.x
                C.x
                C.x += 1

        def reader_func():
            for _ in range(3000):
                # We should always see a greater value read from the type than the
                # dictionary
                a = C.__dict__['x']
                b = C.x
                self.assertGreaterEqual(b, a)

        self.run_one(writer_func, reader_func)

    def test_attr_cache_consistency_subclass(self):
        class C:
            x = 0

        class D(C):
            pass

        def writer_func():
            for _ in range(3000):
                D.x
                D.x
                C.x += 1

        def reader_func():
            for _ in range(3000):
                # We should always see a greater value read from the type than the
                # dictionary
                a = C.__dict__['x']
                b = D.x
                self.assertGreaterEqual(b, a)

        self.run_one(writer_func, reader_func)

    def test_attr_cache_mortal(self):
        class C:
            x = object()

        class D(C):
            pass

        def writer_func():
            for _ in range(3000):
                C.x = object()

        def reader_func():
            for _ in range(3000):
                C.x
                D.x

        self.run_one(writer_func, reader_func)

    def test___class___modification(self):
        loops = 200

        class Foo:
            pass

        class Bar:
            pass

        thing = Foo()
        def work():
            foo = thing
            for _ in range(loops):
                foo.__class__ = Bar
                type(foo)
                foo.__class__ = Foo
                type(foo)


        threads = []
        for i in range(NTHREADS):
            thread = threading.Thread(target=work)
            thread.start()
            threads.append(thread)

        for thread in threads:
            thread.join()

    def test_object_class_change(self):
        class Base:
            def __init__(self):
                self.attr = 123
        class ClassA(Base):
            pass
        class ClassB(Base):
            pass

        obj = ClassA()
        # keep reference to __dict__
        d = obj.__dict__
        obj.__class__ = ClassB


    def test_name_change(self):
        class Foo:
            pass

        def writer():
            for _ in range(1000):
                Foo.__name__ = 'Bar'

        def reader():
            for _ in range(1000):
                Foo.__name__

        self.run_one(writer, reader)

    def test_bases_change(self):
        class BaseA:
            pass

        class Derived(BaseA):
            pass

        def writer():
            for _ in range(1000):
                class BaseB:
                    pass
                Derived.__bases__ = (BaseB,)

        def reader():
            for _ in range(1000):
                Derived.__base__

        self.run_one(writer, reader)

    def test_per_type_cache_concurrent_reads(self):
        class C:
            pass

        names = [sys.intern(f"attr_{i}") for i in range(
            _testinternalcapi._Py_TYPECACHE_MINSIZE * 4)]
        for name in names:
            setattr(C, name, name)
        # Prime the cache.
        for name in names:
            getattr(C, name)

        lookup = _testinternalcapi.type_cache_lookup

        def reader():
            for _ in range(500):
                for name in names:
                    hit, value, _ = lookup(C, name)
                    self.assertEqual(hit, 1, name)
                    self.assertEqual(value, name)

        threading_helper.run_concurrently(reader, nthreads=NTHREADS)

    def test_per_type_cache_concurrent_invalidate(self):
        class C:
            x = "value"

        # Prime the cache.
        C.x
        hit, value, version = _testinternalcapi.type_cache_lookup(C, "x")
        self.assertEqual(hit, 1)
        self.assertIs(value, "value")
        self.assertGreater(version, 0)

        def reader():
            for _ in range(10_000):
                self.assertIs(C.x, "value")

        def invalidator():
            for _ in range(10_000):
                _testinternalcapi.type_cache_invalidate(C)

        workers = [invalidator] + [reader] * (NTHREADS - 1)
        threading_helper.run_concurrently(workers)

    def test_race_type_attr_added(self):
        NROUNDS = 50
        NSTOPPERS = 4
        NWRITERS = 4
        WARM = 8
        KEY = "foo"
        def make_reader():
            ns = {}
            exec(
                "def read(o):\n return o.%s\n" % KEY, ns
            )  # fresh code object per round
            return ns["read"]


        stop_all = [False]


        def stopper():
            class Dummy:
                pass

            Dummy()
            while not stop_all[0]:
                try:
                    Dummy.__abstractmethods__ = frozenset()
                except Exception:
                    pass


        box = {}
        bugs = []  # (round, tid, stored, read_back)

        def writer(tid):
            for _ in range(NROUNDS):
                box["start"].wait()
                sentinel = box["sentinel"]
                reader = box["reader"]
                obj = box["objs"][tid]
                val = box["vals"][tid]
                # 1) warm THIS thread's own copy while KEY is absent -> NDV cached at V
                for _ in range(WARM):
                    reader(obj)
                box["race"].wait()
                # 2) race: store, then read back through our own NDV site
                setattr(obj, KEY, val)
                got = reader(obj)
                if got is sentinel:
                    bugs.append((box["round"], tid, val, got))
                box["end"].wait()

        box["start"] = Barrier(NWRITERS + 1)
        box["race"] = Barrier(NWRITERS)
        box["end"] = Barrier(NWRITERS + 1)

        stoppers = [Thread(target=stopper, daemon=True) for _ in range(NSTOPPERS)]
        writers = [Thread(target=writer, args=(i,)) for i in range(NWRITERS)]
        for t in stoppers + writers:
            t.start()

        for r in range(NROUNDS):
            sentinel = type(
                "SENTINEL_%d" % r, (), {}
            )  # non-descriptor, deferred refcount
            C = type("C", (), {KEY: sentinel})
            box["round"] = r
            box["sentinel"] = sentinel
            box["reader"] = make_reader()
            box["objs"] = [C() for _ in range(NWRITERS)]
            box["vals"] = [["value-%d-%d" % (r, i)] for i in range(NWRITERS)]
            box["start"].wait()
            box["end"].wait()

        stop_all[0] = True
        for t in writers:
            t.join()

        self.assertFalse(bugs)

    def run_one(self, writer_func, reader_func):
        barrier = threading.Barrier(NTHREADS)

        def wrap_target(target):
            def wrapper():
                barrier.wait()
                target()
            return wrapper

        writer = Thread(target=wrap_target(writer_func))
        readers = []
        for x in range(NTHREADS - 1):
            reader = Thread(target=wrap_target(reader_func))
            readers.append(reader)
            reader.start()

        writer.start()
        writer.join()
        for reader in readers:
            reader.join()

    def test_setattr_many_subclasses(self):
        # gh-155978: Updating a special method queues a slot update for every
        # affected subclass.  Keep enough subclasses alive to require
        # heap-allocated queue chunks in addition to the stack chunk.
        class Base:
            pass

        subclasses = [type(f"Sub{i}", (Base,), {}) for i in range(100)]

        def custom_repr(self):
            return "custom repr"

        Base.__repr__ = custom_repr
        self.assertTrue(all(repr(cls()) == "custom repr"
                            for cls in subclasses))

        del Base.__repr__
        self.assertTrue(all(repr(cls()) != "custom repr"
                            for cls in subclasses))

        # The slot update queue must release its references to the types,
        # including those stored in heap-allocated chunks.
        del subclasses
        gc.collect()
        self.assertEqual(Base.__subclasses__(), [])

    def test_concurrent_setattr_deadlock(self):
        # gh-155400: two threads assigning to a special method of the same
        # class could deadlock.  One thread held the type lock and waited for
        # the type dict mutex, which its critical section had released when it
        # blocked on the stop-the-world mutex, while the other held the type
        # dict mutex and waited for the type lock.
        # This is fairly difficult to trigger the race but this N seems to do
        # it at least sometimes.
        N = 200
        done = False

        class Base:
            pass

        def setter():
            func = lambda self: "x"
            barrier.wait()
            while not done:
                Base.__repr__ = func
                try:
                    del Base.__repr__
                except AttributeError:
                    pass

        def subclasser():
            barrier.wait()
            while not done:
                type('Sub', (Base,), {})()

        def lister():
            barrier.wait()
            while not done:
                Base.__subclasses__()

        def basesetter():
            nonlocal done
            barrier.wait()
            for _ in range(N):
                class A:
                    pass
                class C:
                    pass
                class B(A):
                    pass
                B.__bases__ = (C,)
            done = True

        # The setter threads are the ones that deadlock.  The others are there
        # to keep the type lock and the stop-the-world mutex contended, which
        # is what gets the setters into the window where it happens.
        targets = (setter, setter, subclasser, subclasser,
                   lister, lister, basesetter)
        barrier = threading.Barrier(len(targets))
        threads = [Thread(target=target) for target in targets]
        with threading_helper.start_threads(threads):
            pass

    def check_stop_the_world_under_type_lock(self, setter):
        # gh-158195: Python code that runs with the type lock held and stops
        # the world could deadlock with a thread that held the type lock
        # pinned while it waited for the stop-the-world mutex (__bases__
        # assignment).  The first thread released the type lock while it was
        # blocked on the stop-the-world mutex, was handed the mutex, and then
        # waited for the type lock while owning the mutex.
        def basesetter():
            class A:
                pass
            class B:
                pass
            class C(A):
                pass
            for _ in range(100):
                C.__bases__ = (B,)
                C.__bases__ = (A,)

        def stopper():
            for _ in range(100):
                _testinternalcapi.stop_the_world()

        # The stopper keeps the stop-the-world mutex contended, which makes
        # the setters block on it.  All workers have finite workloads so
        # that repeated invalidation cannot indefinitely delay a setter.
        threading_helper.run_concurrently(
            [setter, setter, basesetter, stopper])

    def test_stop_the_world_in_mro_deadlock(self):
        # type_ready() calls a metaclass mro() with the type lock held.
        N = 50

        class Meta(type):
            def mro(cls):
                _testinternalcapi.stop_the_world()
                return type.mro(cls)

        def setter():
            for _ in range(N):
                Meta("K", (), {})

        self.check_stop_the_world_under_type_lock(setter)

    def test_stop_the_world_in_setattr_key_eq_deadlock(self):
        # type_setattro() compares the attribute name with a str subclass key
        # of the type's __dict__ (calling its __eq__) with the type lock held.
        N = 50

        class S(str):
            __hash__ = str.__hash__
            def __eq__(self, other):
                _testinternalcapi.stop_the_world()
                return str.__eq__(self, other)

        C = type("C", (), {S("x"): 0})

        def setter():
            for i in range(N):
                C.x = i

        self.check_stop_the_world_under_type_lock(setter)

    def test_reentrant_slot_update_consistency(self):
        # Updating Base queues its slot update before visiting Sub.  A key
        # comparison in Sub's dict re-enters the setter and makes that queue
        # stale.  It must be rebuilt rather than overwriting the inner update.
        armed = False

        class Base:
            pass

        class Key(str):
            def __hash__(self):
                return hash("__hash__")

            def __eq__(self, other):
                nonlocal armed
                if armed and other == "__hash__":
                    armed = False
                    Base.__hash__ = object.__hash__
                return str.__eq__(self, other)

        # Collide with the special-method name without defining it on Sub,
        # so that slot updates still propagate from Base to Sub.
        Sub = type("Sub", (Base,), {Key("trigger"): None})
        objects = [Base(), Sub()]

        # Check actual slot dispatch, not just the class dictionaries.  None
        # and object.__hash__ use distinct C slots, unlike Python functions
        # which share a generic wrapper that looks up the current method.
        Base.__hash__ = None
        for obj in objects:
            with self.assertRaises(TypeError):
                hash(obj)
        Base.__hash__ = object.__hash__
        for obj in objects:
            self.assertEqual(hash(obj), object.__hash__(obj))

        armed = True
        Base.__hash__ = None
        self.assertFalse(armed)
        self.assertIs(Base.__dict__["__hash__"], object.__hash__)
        for obj in objects:
            self.assertEqual(hash(obj), object.__hash__(obj))

        # Both applied and discarded queues must release their references.
        base_ref = weakref.ref(Base)
        sub_ref = weakref.ref(Sub)
        del obj, objects, Sub, Base
        gc.collect()
        self.assertIsNone(base_ref())
        self.assertIsNone(sub_ref())

    def test_concurrent_bases_assignment(self):
        # Several threads reassign __bases__ of the same class, and of its
        # base, while others stop the world.  The validated bases and the
        # computed MRO can go stale while the type lock is released and
        # must be recomputed.  At the end, the MRO must agree with __bases__.
        N = 50

        class A:
            def meth(self):
                return "A"
            def __repr__(self):
                return "A"
        class B:
            def meth(self):
                return "B"
            def __repr__(self):
                return "B"
        class C(A):
            pass
        class D(C):
            pass

        def setter():
            for _ in range(N):
                C.__bases__ = (B,)
                C.__bases__ = (A,)

        def subsetter():
            for _ in range(N):
                D.__bases__ = (C, B)
                D.__bases__ = (C,)

        threading_helper.run_concurrently(
            [setter, setter, subsetter] + world_stoppers())

        self.assertEqual(C.__bases__, (A,))
        self.assertEqual(C.__mro__, (C, A, object))
        self.assertEqual(D.__bases__, (C,))
        self.assertEqual(D.__mro__, (D, C, A, object))
        self.assertEqual(D().meth(), "A")
        self.assertEqual(repr(C()), "A")
        self.assertEqual(repr(D()), "A")
        self.assertIn(C, A.__subclasses__())
        self.assertNotIn(C, B.__subclasses__())


if __name__ == "__main__":
    unittest.main()
