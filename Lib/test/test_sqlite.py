from test.test_support import run_unittest, TestSkipped

try:
    import _sqlite3
except ImportError:
    raise TestSkipped('no sqlite available')
from sqlite3.test import (dbapi, types, userfunctions,
                                factory, transactions, hooks, regression,
                                dump)

def test_main():
    run_unittest(dbapi.suite(), types.suite(), userfunctions.suite(),
                 factory.suite(), transactions.suite(),
                 hooks.suite(), regression.suite(), dump.suite())

if __name__ == "__main__":
    test_main()
