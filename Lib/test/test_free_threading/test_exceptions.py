import unittest
import copy

from test.support import threading_helper

threading_helper.requires_working_threading(module=True)
class ExceptionTests(unittest.TestCase):
    def test_setstate_data_race(self):
        E = Exception()

        def func():
            for i in range(100):
                setattr(E, 'x', i)
                copy.copy(E)

        threading_helper.run_concurrently(func, nthreads=4)
