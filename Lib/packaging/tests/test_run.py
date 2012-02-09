"""Tests for packaging.run."""

import os
import sys
from io import StringIO

from packaging import install
from packaging.tests import unittest, support
from packaging.run import main

from test.script_helper import assert_python_ok

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


class RunTestCase(support.TempdirManager,
                  support.LoggingCatcher,
                  unittest.TestCase):

    def setUp(self):
        super(RunTestCase, self).setUp()
        self.old_argv = sys.argv, sys.argv[:]

    def tearDown(self):
        sys.argv = self.old_argv[0]
        sys.argv[:] = self.old_argv[1]
        super(RunTestCase, self).tearDown()

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

    def test_show_help(self):
        # smoke test, just makes sure some help is displayed
        status, out, err = assert_python_ok('-m', 'packaging.run', '--help')
        self.assertEqual(status, 0)
        self.assertGreater(out, b'')
        self.assertEqual(err, b'')

    def test_list_commands(self):
        status, out, err = assert_python_ok('-m', 'packaging.run', 'run',
                                            '--list-commands')
        # check that something is displayed
        self.assertEqual(status, 0)
        self.assertGreater(out, b'')
        self.assertEqual(err, b'')

        # make sure the manual grouping of commands is respected
        check_position = out.find(b'  check: ')
        build_position = out.find(b'  build: ')
        self.assertTrue(check_position, out)  # "out" printed as debugging aid
        self.assertTrue(build_position, out)
        self.assertLess(check_position, build_position, out)

        # TODO test that custom commands don't break --list-commands


def test_suite():
    return unittest.makeSuite(RunTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
