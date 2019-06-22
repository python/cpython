import unittest

from .cg.__main__ import main


class ActualChecks(unittest.TestCase):

    # XXX Also run the check in "make check".
    @unittest.expectedFailure
    def test_check_c_statics(self):
        main('check', {})


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
