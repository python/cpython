import test.support
from test.support import import_helper

# Skip test if _sqlite3 module not installed
import_helper.import_module('_sqlite3')

import unittest
import sqlite3
from sqlite3.test import (dbapi, types, userfunctions,
                                factory, transactions, hooks, regression,
                                dump, backup)

def load_tests(*args):
    if test.support.verbose:
        print("test_sqlite: testing with version",
              "{!r}, sqlite_version {!r}".format(sqlite3.version,
                                                 sqlite3.sqlite_version))
    return unittest.TestSuite([dbapi.suite(), types.suite(),
                               userfunctions.suite(),
                               factory.suite(), transactions.suite(),
                               hooks.suite(), regression.suite(),
                               dump.suite(),
                               backup.suite()])

if __name__ == "__main__":
    unittest.main()
