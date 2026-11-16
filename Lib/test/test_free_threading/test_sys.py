import sys
import unittest
from test.support import threading_helper


class SysModuleTest(unittest.TestCase):
    @unittest.skipUnless(hasattr(sys, "setdlopenflags"),
                         "test needs sys.setdlopenflags()")
    def test_dlopenflags_concurrent(self):
        # gh-151644: getdlopenflags() and setdlopenflags() must be safe to
        # call concurrently in free-threaded builds.
        original = sys.getdlopenflags()
        self.addCleanup(sys.setdlopenflags, original)

        # Use a small set of known-valid flag values to avoid integer overflow.
        flag_values = [1, 2, 256, 257]

        def worker(worker_id):
            for i in range(20_000):
                if worker_id % 2 == 0:
                    sys.getdlopenflags()
                else:
                    sys.setdlopenflags(flag_values[worker_id % len(flag_values)])

        workers = [lambda i=i: worker(i) for i in range(6)]
        threading_helper.run_concurrently(workers)

    def test_int_max_str_digits_thread(self):
        # gh-151218: Check that it's safe to call get_int_max_str_digits() and
        # set_int_max_str_digits() in parallel. Previously, this test triggered
        # warnings in TSan on a free threaded build.

        old_limit = sys.get_int_max_str_digits()
        self.addCleanup(sys.set_int_max_str_digits, old_limit)

        def worker(worker_id):
            if not worker_id:
                for i in range (20_000):
                    sys.get_int_max_str_digits()
            else:
                for i in range (20_000):
                    sys.set_int_max_str_digits(4300 + (i & 7))

        workers = [lambda: worker(i) for i in range(5)]
        threading_helper.run_concurrently(workers)


if __name__ == "__main__":
    unittest.main()
