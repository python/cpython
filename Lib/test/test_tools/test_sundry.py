"""Tests for scripts in the Tools directory.

This file contains extremely basic regression tests for the scripts found in
the Tools directory of a Python checkout or tarball which don't have separate
tests of their own.
"""

import os
import sys
import unittest
from test.support import import_helper

from test.test_tools import scriptsdir, import_tool, skip_if_missing

skip_if_missing()

class TestSundryScripts(unittest.TestCase):
    # At least make sure the rest don't have syntax errors.  When tests are
    # added for a script it should be added to the allowlist below.

    # scripts that have independent tests.
    allowlist = ['reindent']
    # scripts that can't be imported without running
    denylist = ['make_ctype']
    # denylisted for other reasons
    other = ['2to3']

    skiplist = denylist + allowlist + other

    def test_sundry(self):
        old_modules = import_helper.modules_setup()
        try:
            for fn in os.listdir(scriptsdir):
                if not fn.endswith('.py'):
                    continue

                name = fn[:-3]
                if name in self.skiplist:
                    continue

                import_tool(name)
        finally:
            # Unload all modules loaded in this test
            import_helper.modules_cleanup(*old_modules)


if __name__ == '__main__':
    unittest.main()
