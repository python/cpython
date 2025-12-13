import unittest
from collections import defaultdict

from threading import Barrier, Thread
from unittest import TestCase

try:
    import _testcapi
except ImportError:
    _testcapi = None

from test.support import threading_helper

@threading_helper.requires_working_threading()
class TestDefaultDict(TestCase):
    def test_default_factory_race(self):
        wait_barrier = Barrier(2)
        write_barrier = Barrier(2)
        key = "default_race_key"

        def default_factory():
            wait_barrier.wait()
            write_barrier.wait()
            return "default_value"

        test_dict = defaultdict(default_factory)

        def writer():
            wait_barrier.wait()
            test_dict[key] = "writer_value"
            write_barrier.wait()

        default_factory_thread = Thread(target=lambda: test_dict[key])
        writer_thread = Thread(target=writer)
        default_factory_thread.start()
        writer_thread.start()
        default_factory_thread.join()
        writer_thread.join()
        self.assertEqual(test_dict[key], "writer_value")


if __name__ == "__main__":
    unittest.main()
