import unittest

from threading import Thread
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

        def writer_func():
            for i in range(OBJECT_COUNT):
                l.append(C(i + OBJECT_COUNT))

        def reader_func():
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

    def test_racing_iter_extend(self):
        l = []

        def writer_func():
            for i in range(OBJECT_COUNT):
                l.extend([C(i + OBJECT_COUNT)])

        def reader_func():
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


if __name__ == "__main__":
    unittest.main()
