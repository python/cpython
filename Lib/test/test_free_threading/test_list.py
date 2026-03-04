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

    def test_reverse(self):
        def reverse_list(b, l):
            b.wait()
            for _ in range(100):
                l.reverse()

        def reader_list(b, l):
            b.wait()
            for _ in range(100):
                for i in range(10):
                    self.assertTrue(0 <= l[i] < 10)

        l = list(range(10))
        barrier = Barrier(2)
        threads = [Thread(target=reverse_list, args=(barrier, l)),
                   Thread(target=reader_list, args=(barrier, l))]
        with threading_helper.start_threads(threads):
            pass

    def test_slice_assignment1(self):
        def assign_slice(b, l):
            b.wait()
            for _ in range(100):
                l[2:5] = [7, 8, 9]

        def reader_list(b, l):
            b.wait()
            for _ in range(100):
                self.assertIn(l[2], (2, 7))
                self.assertIn(l[3], (3, 8))
                self.assertIn(l[4], (4, 9))

        l = list(range(10))
        barrier = Barrier(2)
        threads = [Thread(target=assign_slice, args=(barrier, l)),
                   Thread(target=reader_list, args=(barrier, l))]
        with threading_helper.start_threads(threads):
            pass

    def test_slice_assignment2(self):
        def assign_slice(b, l):
            b.wait()
            for _ in range(100):
                l[::2] = [10, 11, 12, 13, 14]

        def reader_list(b, l):
            b.wait()
            for _ in range(100):
                for i in range(0, 10, 2):
                    self.assertIn(l[i], (i, 10 + i // 2))

        l = list(range(10))
        barrier = Barrier(2)
        threads = [Thread(target=assign_slice, args=(barrier, l)),
                   Thread(target=reader_list, args=(barrier, l))]
        with threading_helper.start_threads(threads):
            pass


if __name__ == "__main__":
    unittest.main()
