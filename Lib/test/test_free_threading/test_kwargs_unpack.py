import unittest
from unittest import TestCase

from test.support import threading_helper, import_helper

_testcapi = import_helper.import_module("_testcapi")

threading_helper.requires_working_threading(module=True)


class TestKwargsUnpackRace(TestCase):
    def test_mutate_kwargs_during_unpack(self):
        # gh-86199: unpacking a shared kwargs dict must tolerate another
        # thread resizing it.
        num_mutators, num_callers = 2, 6
        iters = 1000
        min_keys, max_keys = 4, 3000

        fastcalldict = _testcapi.pyobject_fastcalldict

        def target(**kwargs):
            return len(kwargs)

        shared = {f"k{i}": i for i in range(min_keys)}

        def resize_kwargs():
            for _ in range(iters):
                for i in range(min_keys, max_keys):
                    shared[f"k{i}"] = i
                for i in range(max_keys - 1, min_keys - 1, -1):
                    shared.pop(f"k{i}", None)

        def call_target():
            for _ in range(iters):
                try:
                    fastcalldict(target, (), shared)
                except Exception:
                    pass

        threading_helper.run_concurrently(
            [resize_kwargs] * num_mutators + [call_target] * num_callers)


if __name__ == "__main__":
    unittest.main()
