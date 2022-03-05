"""Tests for scripts in the Tools directory.

This file contains extremely basic regression tests for the scripts found in
the Tools directory of a Python checkout or tarball which don't have separate
tests of their own, such as h2py.py.
"""

import os
import sys
import unittest
from test import support

from test.test_tools import scriptsdir, import_tool, skip_if_missing

skip_if_missing()

class TestSundryScripts(unittest.TestCase):
    # At least make sure the rest don't have syntax errors.  When tests are
    # added for a script it should be added to the whitelist below.

    # scripts that have independent tests.
    whitelist = ['reindent', 'pdeps', 'gprof2html', 'md5sum']
    # scripts that can't be imported without running
    blacklist = ['make_ctype']
    # scripts that use windows-only modules
    windows_only = ['win_add2path']
    # blacklisted for other reasons
    other = ['analyze_dxp']

    skiplist = blacklist + whitelist + windows_only + other

    def test_sundry(self):
        for fn in os.listdir(scriptsdir):
            name = fn[:-3]
            if fn.endswith('.py') and name not in self.skiplist:
                import_tool(name)

    @unittest.skipIf(sys.platform != "win32", "Windows-only test")
    def test_sundry_windows(self):
        for name in self.windows_only:
            import_tool(name)

    def test_analyze_dxp_import(self):
        if hasattr(sys, 'getdxp'):
            import_tool('analyze_dxp')
        else:
            with self.assertRaises(RuntimeError):
                import_tool('analyze_dxp')


if __name__ == '__main__':
    unittest.main()
