"""A module to test whether doctest works properly with
un-unwrappable functions.

See: https://bugs.python.org/issue35753
"""

# This should trigger issue35753 when doctest called
from unittest.mock import call

import sys
import unittest
from test import support
if sys.flags.optimize >= 2:
    raise unittest.SkipTest("Cannot test docstrings with -O2")


def test_main():
    from test import test_doctest5
    EXPECTED = 0
    try:
        f, t = support.run_doctest(test_doctest5)
        if t != EXPECTED:
            raise support.TestFailed("expected %d tests to run, not %d" %
                                          (EXPECTED, t))
    except ValueError as e:
        raise support.TestFailed("Doctest unwrap failed") from e

# Pollute the namespace with a bunch of imported functions and classes,
# to make sure they don't get tested.
# from doctest import *

if __name__ == '__main__':
    test_main()
