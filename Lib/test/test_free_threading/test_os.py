import os
import shutil
import tempfile
import threading
import unittest

from test import support
from test.support import threading_helper


if support.check_sanitizer(thread=True):
    NUMITEMS = 200
    N_NEXT = 2
    N_CLOSE = 2
    REPEAT = 10
else:
    NUMITEMS = 1000
    N_NEXT = 6
    N_CLOSE = 3
    REPEAT = 20


@threading_helper.requires_working_threading()
class ScandirThreadingTest(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.dir, ignore_errors=True)
        self.names = set()
        for i in range(NUMITEMS):
            name = f"f{i}"
            with open(os.path.join(self.dir, name), "w"):
                pass
            self.names.add(name)

    def run_threads(self, funcs):
        threading_helper.run_concurrently(funcs)

    def test_close_racing_next(self):
        # gh-152754: one thread's next() racing another's close() must not crash.
        def nexter():
            for _ in self.it:
                pass

        def closer():
            self.it.close()

        for _ in range(REPEAT):
            self.it = os.scandir(self.dir)
            try:
                self.run_threads([nexter] * N_NEXT + [closer] * N_CLOSE)
            finally:
                self.it.close()

    def test_shared_next(self):
        # gh-152754: threads sharing one iterator must not crash or lose entries.
        self.it = os.scandir(self.dir)
        results = []
        results_lock = threading.Lock()

        def worker():
            local = []
            for entry in self.it:
                local.append(entry.name)
            with results_lock:
                results.extend(local)

        try:
            self.run_threads([worker] * (N_NEXT + N_CLOSE))
        finally:
            self.it.close()

        self.assertEqual(sorted(results), sorted(self.names))


if __name__ == "__main__":
    unittest.main()
