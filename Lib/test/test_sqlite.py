from test.test_support import run_unittest, TestSkipped
import unittest

try:
    import _sqlite3
except ImportError:
    raise TestSkipped('no sqlite available')
from sqlite3.test import (dbapi, types, userfunctions,
                                factory, transactions)

def test_main():
    run_unittest(dbapi.suite(), types.suite(), userfunctions.suite(),
                 factory.suite(), transactions.suite())

if __name__ == "__main__":
    test_main()
