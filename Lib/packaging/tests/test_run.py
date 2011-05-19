"""Tests for packaging.run."""

import os
import sys
import shutil

from packaging.tests import unittest, support, TESTFN

# setup script that uses __file__
setup_using___file__ = """\

__file__

from packaging.run import setup
setup()
"""

setup_prints_cwd = """\

import os
print os.getcwd()

from packaging.run import setup
setup()
"""


class CoreTestCase(unittest.TestCase):

    def setUp(self):
        super(CoreTestCase, self).setUp()
        self.old_stdout = sys.stdout
        self.cleanup_testfn()
        self.old_argv = sys.argv, sys.argv[:]

    def tearDown(self):
        sys.stdout = self.old_stdout
        self.cleanup_testfn()
        sys.argv = self.old_argv[0]
        sys.argv[:] = self.old_argv[1]
        super(CoreTestCase, self).tearDown()

    def cleanup_testfn(self):
        path = TESTFN
        if os.path.isfile(path):
            os.remove(path)
        elif os.path.isdir(path):
            shutil.rmtree(path)

    def write_setup(self, text, path=TESTFN):
        with open(path, "w") as fp:
            fp.write(text)
        return path

    # TODO restore the tests removed six months ago and port them to pysetup


def test_suite():
    return unittest.makeSuite(CoreTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
