"""Tests for packaging.run."""

import os
import sys
import shutil
from tempfile import mkstemp
from io import StringIO

from packaging import install
from packaging.tests import unittest, support, TESTFN
from packaging.run import main

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


class CoreTestCase(support.TempdirManager, support.LoggingCatcher,
                   unittest.TestCase):

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

    def test_install(self):
        # making sure install returns 0 or 1 exit codes
        project = os.path.join(os.path.dirname(__file__), 'package.tgz')
        install_path = self.mkdtemp()
        old_get_path = install.get_path
        install.get_path = lambda path: install_path
        old_mod = os.stat(install_path).st_mode
        os.chmod(install_path, 0)
        old_stderr = sys.stderr
        sys.stderr = StringIO()
        try:
            self.assertFalse(install.install(project))
            self.assertEqual(main(['install', 'blabla']), 1)
        finally:
            sys.stderr = old_stderr
            os.chmod(install_path, old_mod)
            install.get_path = old_get_path


def test_suite():
    return unittest.makeSuite(CoreTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
