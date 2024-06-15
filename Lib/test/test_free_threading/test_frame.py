import sys
import unittest
import threading

from test.support import threading_helper

NTHREADS = 6

@threading_helper.requires_working_threading()
class TestFrame(unittest.TestCase):
    def test_frame_clear_simultaneous(self):

        def gen():
            for _ in range(10000):
                return sys._getframe()

        foo = gen()
        def work():
            for _ in range(4000):
                frame1 = foo
                frame1.clear()


        threads = []
        for i in range(NTHREADS):
            thread = threading.Thread(target=work)
            thread.start()
            threads.append(thread)

        for thread in threads:
            thread.join()
