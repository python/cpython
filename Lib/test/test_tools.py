"""Tests for scripts in the Tools directory.

This file contains regression tests for some of the scripts found in the
Tools directory of a Python checkout or tarball, such as reindent.py.
"""

import os
import unittest
import sysconfig
from test import test_support
from test.script_helper import assert_python_ok

if not sysconfig.is_python_build():
    # XXX some installers do contain the tools, should we detect that
    # and run the tests in that case too?
    raise unittest.SkipTest('test irrelevant for an installed Python')

srcdir = sysconfig.get_config_var('projectbase')
basepath = os.path.join(os.getcwd(), srcdir, 'Tools')


class ReindentTests(unittest.TestCase):
    script = os.path.join(basepath, 'scripts', 'reindent.py')

    def test_noargs(self):
        assert_python_ok(self.script)

    def test_help(self):
        rc, out, err = assert_python_ok(self.script, '-h')
        self.assertEqual(out, b'')
        self.assertGreater(err, b'')


def test_main():
    test_support.run_unittest(ReindentTests)


if __name__ == '__main__':
    unittest.main()
