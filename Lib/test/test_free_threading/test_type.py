import threading
import unittest

from concurrent.futures import ThreadPoolExecutor
from threading import Thread
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

if __name__ == "__main__":
    unittest.main()
