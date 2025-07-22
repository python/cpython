import unittest

from itertools import cycle

from test.support import import_helper, threading_helper, subTests
from test.support.threading_helper import run_concurrently

bisect = import_helper.import_module("bisect")


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestBisect(unittest.TestCase):
    @subTests("insort_func", [bisect.insort_left, bisect.insort_right])
    def test_racing_insort(self, insort_func):
        lst = list(range(10))
        insort_items_iter = cycle(list(range(11)))

        def worker(lst, insort_items_iter):
            for _ in range(10):
                insort_item = next(insort_items_iter)
                insort_func(lst, insort_item)

        run_concurrently(
            worker_func=worker,
            nthreads=NTHREADS,
            args=(lst, insort_items_iter),
        )

        self.assertTrue(self.is_sorted_ascending(lst))

    @staticmethod
    def is_sorted_ascending(lst):
        """
        Check if the list is sorted in ascending order (non-decreasing).
        """
        return all(lst[i - 1] <= lst[i] for i in range(1, len(lst)))


if __name__ == "__main__":
    unittest.main()
