import unittest
from test import support
import _fuzz

class TestFuzz(unittest.TestCase):

    def test_fuzz(self):
        """Run the fuzz tests on blank input.

        This isn't meaningful and only checks it doesn't crash.
        """
        _fuzz._fuzz_run_all()

if __name__ == "__main__":
    support.run_unittest(TestFuzz)
