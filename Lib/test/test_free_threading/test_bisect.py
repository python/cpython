import unittest
from test.support import import_helper, threading_helper
import random

py_bisect = import_helper.import_fresh_module('bisect', blocked=['_bisect'])
c_bisect = import_helper.import_fresh_module('bisect', fresh=['_bisect'])


NTHREADS = 4
OBJECT_COUNT = 500


class TestBase:
    def do_racing_insort(self, insert_method):
        def insert(data):
            for _ in range(OBJECT_COUNT):
                x = random.randint(-OBJECT_COUNT, OBJECT_COUNT)
                insert_method(data, x)

        data = list(range(OBJECT_COUNT))
        threading_helper.run_concurrently(
            worker_func=insert, args=(data,), nthreads=NTHREADS
        )
        if False:
            # These functions are not thread-safe and so the list can become
            # unsorted.  However, we don't want Python to crash if these
            # functions are used concurrently on the same sequence.  This
            # should also not produce any TSAN warnings.
            self.assertTrue(self.is_sorted_ascending(data))

    def test_racing_insert_right(self):
        self.do_racing_insort(self.mod.insort_right)

    def test_racing_insert_left(self):
        self.do_racing_insort(self.mod.insort_left)

    @staticmethod
    def is_sorted_ascending(lst):
        """
        Check if the list is sorted in ascending order (non-decreasing).
        """
        return all(lst[i - 1] <= lst[i] for i in range(1, len(lst)))


@threading_helper.requires_working_threading()
class TestPyBisect(unittest.TestCase, TestBase):
    mod = py_bisect


@threading_helper.requires_working_threading()
class TestCBisect(unittest.TestCase, TestBase):
    mod = c_bisect


if __name__ == "__main__":
    unittest.main()
