"""Tests for scripts in the Tools directory.

This file contains regression tests for some of the scripts found in the
Tools directory of a Python checkout or tarball, such as reindent.py.
"""

import os
import unittest
from test.support.script_helper import assert_python_ok
from test.support import findfile

from test.test_tools import toolsdir, skip_if_missing

skip_if_missing()

class ReindentTests(unittest.TestCase):
    script = os.path.join(toolsdir, 'patchcheck', 'reindent.py')

    def test_noargs(self):
        assert_python_ok(self.script)

    def test_help(self):
        rc, out, err = assert_python_ok(self.script, '-h')
        self.assertEqual(out, b'')
        self.assertGreater(err, b'')

    def test_reindent_file_with_bad_encoding(self):
        bad_coding_path = findfile('bad_coding.py')
        rc, out, err = assert_python_ok(self.script, '-r', bad_coding_path)
        self.assertEqual(out, b'')
        self.assertNotEqual(err, b'')


if __name__ == '__main__':
    unittest.main()
