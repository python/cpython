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


class TestThreadState(unittest.TestCase):
    def test_stop_the_world(self):
        import _testinternalcapi

        _testinternalcapi.stop_the_world()
        _testinternalcapi.stop_the_world(0)

        def pause():
            for _ in range(10):
                _testinternalcapi.stop_the_world(0.0001)

        threading_helper.run_concurrently(pause, nthreads=2)
        for duration in (-1, -0.1, float("nan")):
            with self.subTest(duration=duration):
                with self.assertRaises(ValueError):
                    _testinternalcapi.stop_the_world(duration)
        for duration in (float("inf"), 10**100):
            with self.subTest(duration=duration):
                with self.assertRaises(OverflowError):
                    _testinternalcapi.stop_the_world(duration)
        for duration in (None, "0"):
            with self.subTest(duration=duration):
                with self.assertRaises(TypeError):
                    _testinternalcapi.stop_the_world(duration)


if __name__ == "__main__":
    unittest.main()
