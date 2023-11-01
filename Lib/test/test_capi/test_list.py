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

        # CRASHES check(NULL)


    def test_list_check_exact(self):
        # Test PyList_CheckExact()
        check = _testcapi.list_check_exact
        self.assertTrue(check([1]))
        self.assertTrue(check([]))
        self.assertFalse(check(ListSubclass([1])))
        self.assertFalse(check(UserList([1, 2])))
        self.assertFalse(check({1: 2}))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_list_new(self):
        # Test PyList_New()
        list_new = _testcapi.list_new
        lst = list_new(0)
        self.assertEqual(lst, [])
        self.assertIs(type(lst), list)
        lst2 = list_new(0)
        self.assertIsNot(lst2, lst)
        self.assertRaises(SystemError, list_new, NULL)
        self.assertRaises(SystemError, list_new, -1)

    def test_list_size(self):
        # Test PyList_Size()
        size = _testcapi.list_size
        self.assertEqual(size([1, 2]), 2)
        self.assertEqual(size(ListSubclass([1, 2])), 2)
        self.assertRaises(SystemError, size, UserList())
        self.assertRaises(SystemError, size, {})
        self.assertRaises(SystemError, size, 23)
        self.assertRaises(SystemError, size, object())
        # CRASHES size(NULL)
    
    def test_list_get_size(self):
        # Test PyList_GET_SIZE()
        size = _testcapi.list_get_size
        self.assertEqual(size([1, 2]), 2)
        self.assertEqual(size(ListSubclass([1, 2])), 2)
        # CRASHES size(object())
        # CRASHES size(23)
        # CRASHES size({})
        # CRASHES size(UserList())
        # CRASHES size(NULL)


    def test_list_getitem(self):
        # Test PyList_GetItem()
        getitem = _testcapi.list_getitem
        lst = [1, 2, NULL]
        self.assertEqual(getitem(lst, 0), 1)
        self.assertRaises(IndexError, getitem, lst, -1)
        self.assertRaises(IndexError, getitem, lst, 10)
        self.assertRaises(SystemError, getitem, 42, 1)

        # CRASHES getitem(NULL, 1)

    def test_list_get_item(self):
        # Test PyList_GET_ITEM()
        get_item = _testcapi.list_get_item
        lst = [1, 2, NULL]
        self.assertEqual(get_item(lst, 0), 1)
        self.assertNotEqual(get_item(lst, 1), 12)
        # CRASHES for out of index: get_item(lst, 3)
        # CRASHES get_item(21, 2)
        # CRASHES get_item(Null,1)


    def test_list_setitem(self):
        # Test PyList_SET_ITEM()
        setitem = _testcapi.list_setitem
        lst = [1, 2, 3]
        setitem(lst, 1, 10)
        self.assertEqual(lst[1], 10)
        self.assertRaises(IndexError, setitem, [1], -1, 5)
        self.assertRaises(TypeError, setitem, lst, 1.5, 10)
        self.assertRaises(TypeError, setitem, 23, 'a', 5)
        self.assertRaises(SystemError, setitem, {}, 0, 5)

        # CRASHES setitem(NULL, 'a', 5)

    def test_list_insert(self):
        # Test PyList_Insert()
        insert = _testcapi.list_insert
        lst = [1, 2, 3]
        insert(lst, 1, 23)
        self.assertEqual(lst[1], 23)
        insert(lst, -1, 22)
        self.assertEqual(lst[-2], 22)
        self.assertRaises(TypeError, insert, lst, 1.5, 10)
        self.assertRaises(TypeError, insert, 23, 'a', 5)
        self.assertRaises(SystemError, insert, {}, 0, 5)
        # CRASHES insert(NULL, 'a', 5)

    def test_list_append(self):
        # Test PyList_Append()
        append = _testcapi.list_append
        lst = [1, 2, 3]
        append(lst, 1)
        self.assertEqual(lst[-1], 1)
        append(lst, [4,5,6])
        self.assertEqual(lst[-1], [4,5,6])
        self.assertRaises(SystemError, append, lst, NULL)
        # CRASHES append(NULL, 0)

    def test_list_getslice(self):
        # Test PyList_GetSlice()
        getslice = _testcapi.list_getslice
        lst = [1,2,3,4,5,6,7,8,9,10]
        self.assertEqual(getslice(lst, 0, 3), [1, 2, 3])
        self.assertEqual(getslice(lst, 4, 6), [5,6])
        self.assertEqual(getslice(lst, 6, 10), [7,8,9,10])
        self.assertEqual(getslice(lst, 6, 100), [7,8,9,10])
        self.assertNotEqual(getslice(lst, -2, -1), [9])
        self.assertRaises(TypeError, lst, 'a', '2')

        # CRASHES getslice(NULL, 1, 2)
