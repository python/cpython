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
        lst = list_new(0)
        self.assertEqual(lst, [])
        self.assertIs(type(lst), list)
        lst2 = list_new(0)
        self.assertIsNot(lst2, lst)

    def test_list_size(self):
        size = _testcapi.list_size
        self.assertEqual(size([1, 2]), 2)
        self.assertEqual(size(ListSubclass([1, 2])), 2)
        self.assertRaises(SystemError, size, UserList())
        self.assertRaises(SystemError, size, {})
        self.assertRaises(SystemError, size, 23)
        self.assertRaises(SystemError, size, object())
        # CRASHES size(NULL)


    def test_list_getitem(self):
        getitem = _testcapi.list_getitem
        lst = [1, 2, NULL]
        self.assertEqual(getitem(lst, 0), 1)
        self.assertRaises(IndexError, getitem, lst, -1)
        self.assertRaises(IndexError, getitem, lst, 10)
        self.assertRaises(SystemError, getitem, 42, 1)

        # CRASHES getitem(NULL, 1)

    def test_list_get_item(self):
        # Test PyList_GET_SIZE()
        get_item = _testcapi.list_get_item
        lst = [1, 2, NULL]
        self.assertEqual(get_item(lst, 0), 1)
        self.assertNotEqual(get_item(lst, 1), 12)

        # CRASHES for out of index: get_item(lst, 3)
        # CRASHES get_item(21, 2)
