import unittest

from unittest import TestCase


class TestMemoryViewErrors(TestCase):
    def test_memoryview_release(self):
        av = memoryview(b"something")
        av.release()

        with self.assertRaises(ValueError):
            av.__iter__()

    def test_memoryview_direct_iter_no_error(self):
        av = memoryview(b"something")

        try:
            iterator = av.__iter__()
            list(iterator)
        except Exception as e:
            self.fail(f"Direct __iter__() call raised an exception: {e}")

if __name__ == "__main__":
    unittest.main()
