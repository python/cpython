import test.support

# Skip test if _sqlite3 module not installed
test.support.import_module('_sqlite3')

import sqlite3
from sqlite3.test import (dbapi, types, userfunctions,
                                factory, transactions, hooks, regression,
                                dump)

def test_main():
    if test.support.verbose:
        print("test_sqlite: testing with version",
              "{!r}, sqlite_version {!r}".format(sqlite3.version,
                                                 sqlite3.sqlite_version))
    test.support.run_unittest(dbapi.suite(), types.suite(),
                               userfunctions.suite(),
                               factory.suite(), transactions.suite(),
                               hooks.suite(), regression.suite(),
                               dump.suite())

if __name__ == "__main__":
    test_main()
