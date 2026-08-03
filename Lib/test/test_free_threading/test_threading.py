import unittest
import threading
import _thread
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class TestRlock(unittest.TestCase):
    def test_repr_race(self):
        # gh-153292
        r = _thread.RLock()

        def repr_thread():
            for _ in range(2000):
                repr(r)

        def mutate_thread():
            for _ in range(2000):
                r.acquire()
                r.release()

        threading_helper.run_concurrently([repr_thread, mutate_thread])


class TestShutdown(unittest.TestCase):
    def test_shutdown_race(self):
        ITERATIONS = 1_000
        STARTUP_THREADS = 4

        def shutdown_worker():
            for _ in range(ITERATIONS):
                try:
                    _thread._shutdown()
                except RuntimeError:
                    pass

        def startup_worker():
            for _ in range(ITERATIONS):
                handle = _thread.start_joinable_thread(
                    lambda: None,
                    daemon=False,
                )
                handle.join()

        # The workers must be daemon threads so _thread._shutdown() ignores them
        # and only scans the non-daemon handles created by startup_worker().
        workers = [
            threading.Thread(target=shutdown_worker, daemon=True),
            *[
                threading.Thread(target=startup_worker, daemon=True)
                for _ in range(STARTUP_THREADS)
            ],
        ]

        with threading_helper.catch_threading_exception() as cm:
            with threading_helper.start_threads(workers):
                pass
            if cm.exc_value is not None:
                raise cm.exc_value


if __name__ == "__main__":
    unittest.main()
