import re
import unittest

from test.support import threading_helper
from test.support.threading_helper import run_concurrently


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestRe(unittest.TestCase):
    def test_pattern_sub(self):
        """Pattern substitution should work across threads"""
        pattern = re.compile(r"\w+@\w+\.\w+")
        text = "e-mail: test@python.org or user@pycon.org. " * 5
        results = []

        def worker():
            substituted = pattern.sub("(redacted)", text)
            results.append(substituted.count("(redacted)"))

        run_concurrently(worker_func=worker, nthreads=NTHREADS)
        self.assertEqual(results, [2 * 5] * NTHREADS)

    def test_pattern_search(self):
        """Pattern search should work across threads."""
        emails = ["alice@python.org", "bob@pycon.org"] * 10
        pattern = re.compile(r"\w+@\w+\.\w+")
        results = []

        def worker():
            matches = [pattern.search(e).group() for e in emails]
            results.append(len(matches))

        run_concurrently(worker_func=worker, nthreads=NTHREADS)
        self.assertEqual(results, [2 * 10] * NTHREADS)

    def test_scanner_concurrent_access(self):
        """Shared scanner should reject concurrent access."""
        pattern = re.compile(r"\w+")
        scanner = pattern.scanner("word " * 10)

        def worker():
            for _ in range(100):
                try:
                    scanner.search()
                except ValueError as e:
                    if "already executing" in str(e):
                        pass
                    else:
                        raise

        run_concurrently(worker_func=worker, nthreads=NTHREADS)
        # This test has no assertions. Its purpose is to catch crashes and
        # enable thread sanitizer to detect race conditions. While "already
        # executing" errors are very likely, they're not guaranteed due to
        # non-deterministic thread scheduling, so we can't assert errors > 0.


if __name__ == "__main__":
    unittest.main()
