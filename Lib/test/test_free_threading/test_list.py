import unittest

from threading import Thread
from unittest import TestCase

class TestList(TestCase):
    def test_racing_iter_append(self):

        l = []
        OBJECT_COUNT = 10000
        def writer_func():
            for i in range(OBJECT_COUNT):
                l.append(i + OBJECT_COUNT)

        def reader_func():
            while True:
                count = len(l)
                for i, x in enumerate(l):
                   self.assertEqual(x, i + OBJECT_COUNT)
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
        print('writer done')
        for reader in readers:
            reader.join()
            print('reader done')

if __name__ == "__main__":
    unittest.main()
