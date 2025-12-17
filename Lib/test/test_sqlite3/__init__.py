from test.support import import_helper, load_package_tests, verbose

# Skip test if _sqlite3 module not installed.
import_helper.import_module('_sqlite3')

import os
import sqlite3

# make sure only print once
_printed_version = False

# Implement the unittest "load tests" protocol.
def load_tests(loader, tests, pattern):
    global _printed_version
    if verbose and not _printed_version:
        print(f"test_sqlite3: testing with SQLite version {sqlite3.sqlite_version}")
        _printed_version = True
    pkg_dir = os.path.dirname(__file__)
    return load_package_tests(pkg_dir, loader, tests, pattern)
