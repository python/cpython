import unittest
from test.support.warnings_helper import check_warnings

with check_warnings(("", PendingDeprecationWarning)):
    from lib2to3.tests import load_tests

if __name__ == '__main__':
    unittest.main()
