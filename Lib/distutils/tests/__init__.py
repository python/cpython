"""Test suite for distutils.

This test suite consists of a collection of test modules in the
distutils.tests package.

Tests for the command classes in the distutils.command package are
included in distutils.tests as well, instead of using a separate
distutils.command.tests package, since command identification is done
by import rather than matching pre-defined names.

"""

import os
import unittest
from test.support.warnings_helper import save_restore_warnings_filters
from test.support import warnings_helper
from test.support import load_package_tests


def load_tests(*args):
    # bpo-40055: Save/restore warnings filters to leave them unchanged.
    # Importing tests imports docutils which imports pkg_resources
    # which adds a warnings filter.
    with (save_restore_warnings_filters(),
          warnings_helper.check_warnings(
            ("The distutils.sysconfig module is deprecated", DeprecationWarning),
            quiet=True)):
        return load_package_tests(os.path.dirname(__file__), *args)

if __name__ == "__main__":
    unittest.main()
