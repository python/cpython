import struct
import unittest
from threading import Thread, Barrier, Event
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class BytesThreading(unittest.TestCase):
    @threading_helper.reap_threads
    def test_conversion_from_mutating_list(self):
        number_of_threads = 10
        number_of_iterations = 10
        barrier = Barrier(number_of_threads)

        x = [1, 2, 3, 4, 5]
        extends = [(ii,) * (2 + ii) for ii in range(number_of_threads)]

        def work(ii):
            barrier.wait()
            for _ in range(100):
                bytes(x)
                x.extend(extends[ii])
                if len(x) > 10:
                    x[:] = [0]

        for it in range(number_of_iterations):
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(Thread(target=work, args=[ii]))
            with threading_helper.start_threads(worker_threads):
                pass

            barrier.reset()

    def test_racing_join_replace(self):
        # gh-158803: join() must not use a list item that another thread
        # replaces (and frees) concurrently.
        lst = [bytes(10) for _ in range(100)]
        done = Event()

        def writer():
            try:
                for _ in range(100):
                    for i in range(len(lst)):
                        lst[i] = bytearray(10) if i % 2 else bytes(10)
            finally:
                done.set()

        def reader():
            while not done.is_set():
                b''.join(lst)
                b'-'.join(lst)
                bytearray().join(lst)

        threading_helper.run_concurrently([writer] + [reader] * 4)

    def test_read_bytearray_during_resize(self):
        # Each read must see one complete slice assignment, even if the
        # writer replaces the backing storage.
        short = b'1'
        long = b'2' * 512  # Below the minimum configurable int digit limit.
        data = bytearray(short)
        pack_string = struct.Struct('1024s').pack
        pack_pascal = struct.Struct('1024p').pack
        strings = (pack_string(short), pack_string(long))
        pascals = (pack_pascal(short), pack_pascal(long))
        numbers = (int(short), int(long))

        def writer():
            for _ in range(2000):
                data[:] = long
                data[:] = short

        def reader():
            for _ in range(2000):
                self.assertIn(b'%b' % data, (short, long))
                self.assertIn(pack_string(data), strings)
                self.assertIn(pack_pascal(data), pascals)
                self.assertIn(int(data), numbers)
                self.assertIn(int(data, 10), numbers)

        threading_helper.run_concurrently([writer, reader, reader])


if __name__ == "__main__":
    unittest.main()
