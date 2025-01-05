import threading
import unittest

from concurrent.futures import ThreadPoolExecutor
from threading import Thread
from unittest import TestCase

from test import support
from test.support import threading_helper



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

        DONE = False
        def writer_func():
            for i in range(3000):
                C.x
                C.x
                C.x += 1
            nonlocal DONE
            DONE = True

        def reader_func():
            while True:
                # We should always see a greater value read from the type than the
                # dictionary
                a = C.__dict__['x']
                b = C.x
                self.assertGreaterEqual(b, a)

                if DONE:
                    break

        self.run_one(writer_func, reader_func)

    def test_attr_cache_consistency_subclass(self):
        class C:
            x = 0

        class D(C):
            pass

        DONE = False
        def writer_func():
            for i in range(3000):
                D.x
                D.x
                C.x += 1
            nonlocal DONE
            DONE = True

        def reader_func():
            while True:
                # We should always see a greater value read from the type than the
                # dictionary
                a = C.__dict__['x']
                b = D.x
                self.assertGreaterEqual(b, a)

                if DONE:
                    break

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


    def run_one(self, writer_func, reader_func):
        writer = Thread(target=writer_func)
        readers = []
        for x in range(30):
            reader = Thread(target=reader_func)
            readers.append(reader)
            reader.start()

        writer.start()
        writer.join()
        for reader in readers:
            reader.join()

if __name__ == "__main__":
    unittest.main()
