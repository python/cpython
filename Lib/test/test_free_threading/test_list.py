import unittest

from threading import Thread, Barrier
from unittest import TestCase

from test.support import threading_helper


NTHREAD = 10
OBJECT_COUNT = 5_000


class C:
    def __init__(self, v):
        self.v = v


@threading_helper.requires_working_threading()
class TestList(TestCase):
    def test_racing_iter_append(self):
        l = []

        barrier = Barrier(NTHREAD + 1)
        def writer_func(l):
            barrier.wait()
            for i in range(OBJECT_COUNT):
                l.append(C(i + OBJECT_COUNT))

        def reader_func(l):
            barrier.wait()
            while True:
                count = len(l)
                for i, x in enumerate(l):
                    self.assertEqual(x.v, i + OBJECT_COUNT)
                if count == OBJECT_COUNT:
                    break

        writer = Thread(target=writer_func, args=(l,))
        readers = []
        for x in range(NTHREAD):
            reader = Thread(target=reader_func, args=(l,))
            readers.append(reader)
            reader.start()

        writer.start()
        writer.join()
        for reader in readers:
            reader.join()

    def test_racing_iter_extend(self):
        l = []

        barrier = Barrier(NTHREAD + 1)
        def writer_func():
            barrier.wait()
            for i in range(OBJECT_COUNT):
                l.extend([C(i + OBJECT_COUNT)])

        def reader_func():
            barrier.wait()
            while True:
                count = len(l)
                for i, x in enumerate(l):
                    self.assertEqual(x.v, i + OBJECT_COUNT)
                if count == OBJECT_COUNT:
                    break

        writer = Thread(target=writer_func)
        readers = []
        for x in range(NTHREAD):
            reader = Thread(target=reader_func)
            readers.append(reader)
            reader.start()

        writer.start()
        writer.join()
        for reader in readers:
            reader.join()

    def test_store_list_int(self):
        def copy_back_and_forth(b, l):
            b.wait()
            for _ in range(100):
                l[0] = l[1]
                l[1] = l[0]

        l = [0, 1]
        barrier = Barrier(NTHREAD)
        threads = [Thread(target=copy_back_and_forth, args=(barrier, l))
                   for _ in range(NTHREAD)]
        with threading_helper.start_threads(threads):
            pass


if __name__ == "__main__":
    unittest.main()
