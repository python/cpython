import unittest
import sys
from test.support import import_helper
from collections import UserList

_testcapi = import_helper.import_module('_testcapi')

NULL = None

class ListSubclass(list):
    ...

class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyList_Check()
        check = _testcapi.list_check
        self.assertTrue(check([1, 2]))
        self.assertTrue(check([]))
        self.assertTrue(check(ListSubclass([1, 2])))
        self.assertFalse(check({1: 2}))
        self.assertFalse(check((1, 2)))
        self.assertFalse(check(42))
        self.assertFalse(check(object()))

    def test_list_check_exact(self):
        check = _testcapi.list_check_exact
        self.assertTrue(check([1]))
        self.assertTrue(check([]))
        self.assertFalse(check(ListSubclass([1])))
        self.assertFalse(check(UserList([1, 2])))
        self.assertFalse(check({1: 2}))
        self.assertFalse(check(object()))

    def test_list_new(self):
        list_new = _testcapi.list_new
        lst = list_new()
        self.assertEqual(lst, [])
        self.assertIs(type(lst), list)
        lst2 = list_new()
        self.assertIsNot(lst2, lst)
