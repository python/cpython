from array import array
import unittest

from test.support import threading_helper

threading_helper.requires_working_threading(module=True)

class TestArray(unittest.TestCase):
    def test_array_export_race(self):
        arr = array('i', [1, 2, 3, 4, 5])

        ITER_TIMES = 1_000
        THREAD_NUMS = 4
        def incre_export(arr: array):
            for _ in range(ITER_TIMES):
                view = memoryview(arr)
                view.release()

        threading_helper.run_concurrently(incre_export, THREAD_NUMS, (arr,))
