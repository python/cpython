import test.support
from test.support import import_helper

# Skip test if _sqlite3 module not installed
import_helper.import_module('_sqlite3')

import os
import sqlite3

def load_tests(*args):
    if test.support.verbose:
        print("test_sqlite3: testing with version",
              "{!r}, sqlite_version {!r}".format(sqlite3.version,
                                                 sqlite3.sqlite_version))

    pkg_dir = os.path.dirname(__file__)
    return test.support.load_package_tests(pkg_dir, *args)
