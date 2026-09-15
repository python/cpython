import unittest
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class TestRlock(unittest.TestCase):
    def test_repr_race(self):
        # gh-153292
        import _thread
        r = _thread.RLock()

        def repr_thread():
            for _ in range(2000):
                repr(r)

        def mutate_thread():
            for _ in range(2000):
                r.acquire()
                r.release()

        threading_helper.run_concurrently([repr_thread, mutate_thread])

    def test_recursion_count_race(self):
        # gh-154928: repr() reads the count while another thread updates it
        import _thread
        r = _thread.RLock()

        def repr_thread():
            for _ in range(2000):
                repr(r)

        def recurse_thread():
            for _ in range(2000):
                r.acquire()
                r.acquire()
                r.release()
                r.release()

        threading_helper.run_concurrently([repr_thread, recurse_thread])


if __name__ == "__main__":
    unittest.main()
