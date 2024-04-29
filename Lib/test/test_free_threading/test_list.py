import unittest

from threading import Thread
from unittest import TestCase

from test.support import is_wasi


class C:
    def __init__(self, v):
        self.v = v


@unittest.skipIf(is_wasi, "WASI has no threads.")
class TestList(TestCase):
    def test_racing_iter_append(self):

        l = []
        OBJECT_COUNT = 10000

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
        for x in range(30):
            reader = Thread(target=reader_func)
            readers.append(reader)
            reader.start()

        writer.start()
        writer.join()
        for reader in readers:
            reader.join()

    def test_racing_iter_extend(self):
        iters = [
            lambda x: [x],
        ]
        for iter_case in iters:
            with self.subTest(iter=iter_case):
                l = []
                OBJECT_COUNT = 10000

                def writer_func():
                    for i in range(OBJECT_COUNT):
                        l.extend(iter_case(C(i + OBJECT_COUNT)))

                def reader_func():
                    while True:
                        count = len(l)
                        for i, x in enumerate(l):
                            self.assertEqual(x.v, i + OBJECT_COUNT)
                        if count == OBJECT_COUNT:
                            break

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
