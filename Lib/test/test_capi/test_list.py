import unittest
import sys
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')

NULL = None

class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyList_Check()
        check = _testcapi.list_check()
        self.assertFalse(check({1: 2}))
        self.assertTrue(check([1, 2]))
        self.assertTrue(check((1, 2)))
        self.assertTrue(check('abc'))
        self.assertTrue(check(b'abc'))
        self.assertFalse(check(42))
        self.assertFalse(check(object()))
        # CRASHES check(NULL)