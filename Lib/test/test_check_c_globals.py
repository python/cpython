import os
import unittest

import test.test_tools
import test.support
from test.support.warnings_helper import save_restore_warnings_filters


test.test_tools.skip_if_missing('c-analyzer')
with test.test_tools.imports_under_tool('c-analyzer'):
    # gh-95349: Save/restore warnings filters to leave them unchanged.
    # Importing the c-analyzer imports docutils which imports pkg_resources
    # which adds a warnings filter.
    with save_restore_warnings_filters():
        from cpython.__main__ import main


class ActualChecks(unittest.TestCase):

    # XXX Also run the check in "make check".
    def test_check_c_globals(self):
        olddir = os.getcwd()
        os.chdir(test.support.REPO_ROOT)
        try:
            main('check', {})
        except NotImplementedError:
            raise unittest.SkipTest('not supported on this host')
        finally:
            os.chdir(olddir)


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
