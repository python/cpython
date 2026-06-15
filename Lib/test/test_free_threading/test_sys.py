import sys
import unittest
from test.support import threading_helper


class SysModuleTest(unittest.TestCase):
    def test_int_max_str_digits_thread(self):
        # gh-151218: Check that it's safe to call set_int_max_str_digits()
        # in parallel. Previously, this test triggered warnings in TSan
        # on a free threaded build.

        old_limit = sys.get_int_max_str_digits()
        self.addCleanup(sys.set_int_max_str_digits, old_limit)

        def worker():
            for i in range (20_000):
                sys.set_int_max_str_digits(4300 + (i & 7))

        threading_helper.run_concurrently(worker, nthreads=4)


if __name__ == "__main__":
    unittest.main()
