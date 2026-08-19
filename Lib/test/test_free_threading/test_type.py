import threading
import unittest

from concurrent.futures import ThreadPoolExecutor
from threading import Thread
from unittest import TestCase

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


if __name__ == "__main__":
    unittest.main()
