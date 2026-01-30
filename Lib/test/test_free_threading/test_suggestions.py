import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

suggestions = import_helper.import_module("_suggestions")

NTHREADS = 10


@threading_helper.requires_working_threading()
class SuggestionsTests(unittest.TestCase):
    def test_generate_suggestions(self):
        candidates = [str(i) for i in range(100)]

        def worker():
            _ = suggestions._generate_suggestions(candidates, "42")
            candidates.clear()

        run_concurrently(worker_func=worker, nthreads=NTHREADS)


if __name__ == "__main__":
    unittest.main()
