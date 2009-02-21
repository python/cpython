"""Tests for distutils.core."""

import StringIO
import distutils.core
import os
import shutil
import sys
import test.test_support
import unittest


# setup script that uses __file__
setup_using___file__ = """\

__file__

from distutils.core import setup
setup()
"""

setup_prints_cwd = """\

import os
print os.getcwd()

from distutils.core import setup
setup()
"""


class CoreTestCase(unittest.TestCase):

    def setUp(self):
        self.old_stdout = sys.stdout
        self.cleanup_testfn()

    def tearDown(self):
        sys.stdout = self.old_stdout
        self.cleanup_testfn()

    def cleanup_testfn(self):
        path = test.test_support.TESTFN
        if os.path.isfile(path):
            os.remove(path)
        elif os.path.isdir(path):
            shutil.rmtree(path)

    def write_setup(self, text, path=test.test_support.TESTFN):
        open(path, "w").write(text)
        return path

    def test_run_setup_provides_file(self):
        # Make sure the script can use __file__; if that's missing, the test
        # setup.py script will raise NameError.
        distutils.core.run_setup(
            self.write_setup(setup_using___file__))

    def test_run_setup_uses_current_dir(self):
        # This tests that the setup script is run with the current directory
        # as its own current directory; this was temporarily broken by a
        # previous patch when TESTFN did not use the current directory.
        sys.stdout = StringIO.StringIO()
        cwd = os.getcwd()

        # Create a directory and write the setup.py file there:
        os.mkdir(test.test_support.TESTFN)
        setup_py = os.path.join(test.test_support.TESTFN, "setup.py")
        distutils.core.run_setup(
            self.write_setup(setup_prints_cwd, path=setup_py))

        output = sys.stdout.getvalue()
        if output.endswith("\n"):
            output = output[:-1]
        self.assertEqual(cwd, output)


def test_suite():
    return unittest.makeSuite(CoreTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
