import unittest
from test.support import check_warnings, import_fresh_module

with check_warnings(("", PendingDeprecationWarning)):
    load_tests = import_fresh_module('lib2to3.tests', fresh=['lib2to3']).load_tests

if __name__ == '__main__':
    unittest.main()
