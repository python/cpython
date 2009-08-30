from . import util
import unittest


class APITest(unittest.TestCase):

    """Test API-specific details for __import__ (e.g. raising the right
    exception when passing in an int for the module name)."""

    def test_name_requires_rparition(self):
        # Raise TypeError if a non-string is passed in for the module name.
        with self.assertRaises(TypeError):
            util.import_(42)


def test_main():
    from test.support import run_unittest
    run_unittest(APITest)


if __name__ == '__main__':
    test_main()
