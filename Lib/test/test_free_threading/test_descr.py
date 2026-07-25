import unittest

from test import support
from test.support import threading_helper


N = 8


@threading_helper.requires_working_threading()
class TestDescrQualnameRace(unittest.TestCase):
    # gh-154044: reading __qualname__ on a shared descriptor for the first time
    # concurrently raced on the lazy d_qualname cache.

    def race_first_access(self, descr):
        results = []

        def read():
            results.append(descr.__qualname__)

        threading_helper.run_concurrently(read, N)
        self.assertEqual(len(set(results)), 1)
        self.assertIsNotNone(results[0])

    def test_slot_member_descriptors(self):
        count = 100 if support.check_sanitizer(thread=True) else 300
        for _ in range(count):
            class C:
                __slots__ = ("value",)
            self.race_first_access(C.__dict__["value"])

    def test_builtin_descriptors(self):
        kinds = {"method_descriptor", "getset_descriptor", "wrapper_descriptor"}
        descrs = [
            v
            for tp in (str, bytes, list, dict, set, int, float, tuple,
                       frozenset, bytearray)
            for v in vars(tp).values()
            if type(v).__name__ in kinds
        ]
        for descr in descrs:
            self.race_first_access(descr)


if __name__ == "__main__":
    unittest.main()
