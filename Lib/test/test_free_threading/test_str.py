import unittest

from itertools import cycle
from threading import Event, Thread
from unittest import TestCase

from test.support import threading_helper

@threading_helper.requires_working_threading()
class TestStr(TestCase):
    def test_racing_join_extend(self):
        '''Test joining a string being extended by another thread'''
        l = []
        ITERS = 100
        READERS = 10
        done_event = Event()
        def writer_func():
            for i in range(ITERS):
                l.extend(map(str, range(i)))
                l.clear()
            done_event.set()
        def reader_func():
            while not done_event.is_set():
                ''.join(l)
        writer = Thread(target=writer_func)
        readers = []
        for x in range(READERS):
            reader = Thread(target=reader_func)
            readers.append(reader)
            reader.start()

        writer.start()
        writer.join()
        for reader in readers:
            reader.join()

    def test_racing_join_replace(self):
        '''
        Test joining a string of characters being replaced with ephemeral
        strings by another thread.
        '''
        l = [*'abcdefg']
        MAX_ORDINAL = 1_000
        READERS = 10
        done_event = Event()

        def writer_func():
            for i, c in zip(cycle(range(len(l))),
                            map(chr, range(128, MAX_ORDINAL))):
                l[i] = c
            done_event.set()

        def reader_func():
            while not done_event.is_set():
                ''.join(l)
                ''.join(l)
                ''.join(l)
                ''.join(l)

        writer = Thread(target=writer_func)
        readers = []
        for x in range(READERS):
            reader = Thread(target=reader_func)
            readers.append(reader)
            reader.start()

        writer.start()
        writer.join()
        for reader in readers:
            reader.join()

    def test_maketrans_dict_concurrent_modification(self):
        for _ in range(5):
            d = {2000: 'a'}

            def work(dct):
                for i in range(100):
                    str.maketrans(dct)
                    dct[2000 + i] = chr(i % 16)
                    dct.pop(2000 + i, None)

            threading_helper.run_concurrently(
                work,
                nthreads=5,
                args=(d,),
            )


if __name__ == "__main__":
    unittest.main()
