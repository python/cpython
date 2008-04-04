"""Tests for distutils.core."""

import StringIO
import distutils.core
import os
import test.test_support
import unittest


# setup script that uses __file__
setup_using___file__ = """\

__file__

from distutils.core import setup
setup()
"""


class CoreTestCase(unittest.TestCase):

    def tearDown(self):
        os.remove(test.test_support.TESTFN)

    def write_setup(self, text):
        return fn

    def test_run_setup_provides_file(self):
        # Make sure the script can use __file__; if that's missing, the test
        # setup.py script will raise NameError.
        fn = test.test_support.TESTFN
        open(fn, "w").write(setup_using___file__)
        distutils.core.run_setup(fn)


def test_suite():
    return unittest.makeSuite(CoreTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
