import datetime
import unittest

from test.support import threading_helper
from test.support.threading_helper import run_concurrently


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestDatetime(unittest.TestCase):
    def test_concurrent_strftime_normalize_century(self):
        d = datetime.date(2026, 4, 12)
        results = []

        def worker():
            results.append(d.strftime("%Y-%m-%d"))

        run_concurrently(worker_func=worker, nthreads=NTHREADS)
        self.assertEqual(results, ["2026-04-12"] * NTHREADS)


if __name__ == "__main__":
    unittest.main()
