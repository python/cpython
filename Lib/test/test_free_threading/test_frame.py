import sys
import unittest
import threading

from test.support import threading_helper

NTHREADS = 6

@threading_helper.requires_working_threading()
class TestFrame(unittest.TestCase):
    def test_frame_clear_simultaneous(self):

        def getframe():
            return sys._getframe()

        foo = getframe()
        def work():
            for _ in range(4000):
                foo.clear()


        threads = []
        for i in range(NTHREADS):
            thread = threading.Thread(target=work)
            thread.start()
            threads.append(thread)

        for thread in threads:
            thread.join()
