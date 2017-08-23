import test.support
import unittest

_fuzz = test.support.import_module('_fuzz')

class TestFuzz(unittest.TestCase):

    def test_fuzz(self):
        """Run the fuzz tests on sample input.

        This isn't meaningful and only checks it doesn't crash.
        """
        _fuzz.run(b"")
        _fuzz.run(b"\0")
        _fuzz.run(b"{")
        _fuzz.run(b" ")
        _fuzz.run(b"x")
        _fuzz.run(b"1")

if __name__ == "__main__":
    unittest.main()
