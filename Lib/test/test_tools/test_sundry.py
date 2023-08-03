"""Tests for scripts in the Tools/scripts directory.

This file contains extremely basic regression tests for the scripts found in
the Tools directory of a Python checkout or tarball which don't have separate
tests of their own.
"""

import os
import unittest

from test.test_tools import scriptsdir, import_tool, skip_if_missing

skip_if_missing()

class TestSundryScripts(unittest.TestCase):

    def test_sundry(self,):
        for fn in os.listdir(scriptsdir):
            if not fn.endswith('.py'):
                continue
            name = fn[:-3]
            import_tool(name)


if __name__ == '__main__':
    unittest.main()
