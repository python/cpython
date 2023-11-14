"""Tests for scripts in the Tools/scripts directory.

This file contains extremely basic regression tests for the scripts found in
the Tools directory of a Python checkout or tarball which don't have separate
tests of their own.
"""

import os
import unittest
from test.support import import_helper

from test.test_tools import scriptsdir, import_tool, skip_if_missing

skip_if_missing()

class TestSundryScripts(unittest.TestCase):
    # import logging registers "atfork" functions which keep indirectly the
    # logging module dictionary alive. Mock the function to be able to unload
    # cleanly the logging module.
    @import_helper.mock_register_at_fork
    def test_sundry(self, mock_os):
        for fn in os.listdir(scriptsdir):
            if not fn.endswith('.py'):
                continue
            name = fn[:-3]
            import_tool(name)


if __name__ == '__main__':
    unittest.main()
