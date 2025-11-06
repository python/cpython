from test.support import import_helper, load_package_tests, verbose

# Skip test if _sqlite3 module not installed.
import_helper.import_module('_sqlite3')

import os
import sqlite3

# Implement the unittest "load tests" protocol.
def load_tests(*args):
    if verbose:
        print(f"test_sqlite3: testing with SQLite version {sqlite3.sqlite_version}")
    pkg_dir = os.path.dirname(__file__)
    return load_package_tests(pkg_dir, *args)
