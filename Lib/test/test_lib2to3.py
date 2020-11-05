import unittest
from test.support.import_helper import import_fresh_module
from test.support.warnings_helper import check_warnings

with check_warnings(("", PendingDeprecationWarning)):
    load_tests = import_fresh_module('lib2to3.tests', fresh=['lib2to3']).load_tests

if __name__ == '__main__':
    unittest.main()
