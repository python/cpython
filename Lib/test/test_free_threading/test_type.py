import threading
import unittest

from concurrent.futures import ThreadPoolExecutor
from threading import Barrier, Thread
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

if __name__ == "__main__":
    unittest.main()
