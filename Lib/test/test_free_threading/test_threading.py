import unittest
import textwrap

from test import support
from test.support import script_helper, threading_helper

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

    @support.requires_subprocess()
    def test_tight_stw_loop_does_not_starve_attach(self):
        script = textwrap.dedent(f"""
            import faulthandler

            faulthandler.dump_traceback_later({support.SHORT_TIMEOUT}, exit=True)

            import _testinternalcapi
            import threading
            import time

            started = threading.Event()
            stop = threading.Event()

            def stop_the_world():
                _testinternalcapi.test_stop_the_world()
                started.set()
                while not stop.is_set():
                    _testinternalcapi.test_stop_the_world()

            thread = threading.Thread(target=stop_the_world)
            thread.start()
            started.wait()
            # Each reattachment must make progress between consecutive pauses.
            for _ in range(50):
                time.sleep(0.02)
            stop.set()
            thread.join()
            faulthandler.cancel_dump_traceback_later()
        """)
        script_helper.assert_python_ok("-X", "gil=0", "-c", script)


if __name__ == "__main__":
    unittest.main()
