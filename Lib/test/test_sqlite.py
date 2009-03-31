import unittest
from test.support import run_unittest, import_module

# Skip test if _sqlite3 module not installed
import_module('_sqlite3')

from sqlite3.test import (dbapi, types, userfunctions,
                                factory, transactions, hooks, regression,
                                dump)

def test_main():
    run_unittest(dbapi.suite(), types.suite(), userfunctions.suite(),
                 factory.suite(), transactions.suite(),
                 hooks.suite(), regression.suite(), dump.suite())

if __name__ == "__main__":
    test_main()
