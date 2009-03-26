import unittest
from test.test_support import run_unittest

try:
    import _sqlite3
except ImportError:
    raise unittest.SkipTest('no sqlite available')
from sqlite3.test import (dbapi, types, userfunctions, py25tests,
                                factory, transactions, hooks, regression,
                                dump)

def test_main():
    run_unittest(dbapi.suite(), types.suite(), userfunctions.suite(),
                 py25tests.suite(), factory.suite(), transactions.suite(),
                 hooks.suite(), regression.suite(), dump.suite())

if __name__ == "__main__":
    test_main()
