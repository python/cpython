import gc
import weakref
import unittest
from test.support import import_helper
from collections import UserList
_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')

NULL = None
PY_SSIZE_T_MIN = _testcapi.PY_SSIZE_T_MIN
PY_SSIZE_T_MAX = _testcapi.PY_SSIZE_T_MAX

class ListSubclass(list):
    pass


class DelAppend:
    def __init__(self, lst, item):
        self.lst = lst
        self.item = item

    def __del__(self):
        self.lst.append(self.item)


class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyList_Check()
        check = _testlimitedcapi.list_check
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
        check = _testlimitedcapi.list_check_exact
        self.assertTrue(check([1]))
        self.assertTrue(check([]))
        self.assertFalse(check(ListSubclass([1])))
        self.assertFalse(check(UserList([1, 2])))
        self.assertFalse(check({1: 2}))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_list_new(self):
        # Test PyList_New()
        list_new = _testlimitedcapi.list_new
        lst = list_new(0)
        self.assertEqual(lst, [])
        self.assertIs(type(lst), list)
        lst2 = list_new(0)
        self.assertIsNot(lst2, lst)
        self.assertRaises(SystemError, list_new, NULL)
        self.assertRaises(SystemError, list_new, -1)

    def test_list_size(self):
        # Test PyList_Size()
        size = _testlimitedcapi.list_size
        self.assertEqual(size([]), 0)
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
        self.assertEqual(size([]), 0)
        self.assertEqual(size([1, 2]), 2)
        self.assertEqual(size(ListSubclass([1, 2])), 2)
        # CRASHES size(object())
        # CRASHES size(23)
        # CRASHES size({})
        # CRASHES size(UserList())
        # CRASHES size(NULL)

    def check_list_get_item(self, getitem, exctype):
        # Common test cases for PyList_GetItem() and PyList_GetItemRef()
        lst = [1, 2, 3]
        self.assertEqual(getitem(lst, 0), 1)
        self.assertEqual(getitem(lst, 2), 3)
        self.assertRaises(IndexError, getitem, lst, 3)
        self.assertRaises(IndexError, getitem, lst, -1)
        self.assertRaises(IndexError, getitem, lst, PY_SSIZE_T_MIN)
        self.assertRaises(IndexError, getitem, lst, PY_SSIZE_T_MAX)
        self.assertRaises(exctype, getitem, 42, 1)
        self.assertRaises(exctype, getitem, (1, 2, 3), 1)
        self.assertRaises(exctype, getitem, {1: 2}, 1)
        # CRASHES getitem(NULL, 1)

    def test_list_getitem(self):
        # Test PyList_GetItem()
        self.check_list_get_item(_testlimitedcapi.list_getitem, SystemError)

    def test_list_get_item_ref(self):
        # Test PyList_GetItemRef()
        self.check_list_get_item(_testlimitedcapi.list_get_item_ref, TypeError)

    def test_list_get_item(self):
        # Test PyList_GET_ITEM()
        get_item = _testcapi.list_get_item
        lst = [1, 2, [1, 2, 3]]
        self.assertEqual(get_item(lst, 0), 1)
        self.assertEqual(get_item(lst, 2), [1, 2, 3])

        # CRASHES for out of index: get_item(lst, 3)
        # CRASHES for get_item(lst, PY_SSIZE_T_MIN)
        # CRASHES for get_item(lst, PY_SSIZE_T_MAX)
        # CRASHES get_item(21, 2)
        # CRASHES get_item(NULL, 1)

    def test_list_setitem(self):
        # Test PyList_SetItem()
        setitem = _testlimitedcapi.list_setitem
        lst = [1, 2, 3]
        setitem(lst, 0, 10)
        self.assertEqual(lst, [10, 2, 3])
        setitem(lst, 2, 12)
        self.assertEqual(lst, [10, 2, 12])
        self.assertRaises(IndexError, setitem, lst, 3 , 5)
        self.assertRaises(IndexError, setitem, lst, -1, 5)
        self.assertRaises(IndexError, setitem, lst, PY_SSIZE_T_MIN, 5)
        self.assertRaises(IndexError, setitem, lst, PY_SSIZE_T_MAX, 5)
        self.assertRaises(SystemError, setitem, (1, 2, 3), 1, 5)
        self.assertRaises(SystemError, setitem, {1: 2}, 1, 5)

        # CRASHES setitem(NULL, 'a', 5)

    def test_list_set_item(self):
        # Test PyList_SET_ITEM()
        set_item = _testcapi.list_set_item
        lst = [1, 2, 3]
        set_item(lst, 1, 10)
        set_item(lst, 2, [1, 2, 3])
        self.assertEqual(lst, [1, 10, [1, 2, 3]])

        # CRASHES for set_item([1], -1, 5)
        # CRASHES for set_item([1], PY_SSIZE_T_MIN, 5)
        # CRASHES for set_item([1], PY_SSIZE_T_MAX, 5)
        # CRASHES for set_item([], 0, 1)
        # CRASHES for set_item(NULL, 0, 1)


    def test_list_insert(self):
        # Test PyList_Insert()
        insert = _testlimitedcapi.list_insert
        lst = [1, 2, 3]
        insert(lst, 0, 23)
        self.assertEqual(lst, [23, 1, 2, 3])
        insert(lst, -1, 22)
        self.assertEqual(lst, [23, 1, 2, 22, 3])
        insert(lst, PY_SSIZE_T_MIN, 1)
        self.assertEqual(lst[0], 1)
        insert(lst, len(lst), 123)
        self.assertEqual(lst[-1], 123)
        insert(lst, len(lst)-1, 124)
        self.assertEqual(lst[-2], 124)
        insert(lst, PY_SSIZE_T_MAX, 223)
        self.assertEqual(lst[-1], 223)

        self.assertRaises(SystemError, insert, (1, 2, 3), 1, 5)
        self.assertRaises(SystemError, insert, {1: 2}, 1, 5)

        # CRASHES insert(NULL, 1, 5)

    def test_list_append(self):
        # Test PyList_Append()
        append = _testlimitedcapi.list_append
        lst = [1, 2, 3]
        append(lst, 10)
        self.assertEqual(lst, [1, 2, 3, 10])
        append(lst, [4, 5])
        self.assertEqual(lst, [1, 2, 3, 10, [4, 5]])
        self.assertRaises(SystemError, append, lst, NULL)
        self.assertRaises(SystemError, append, (), 0)
        self.assertRaises(SystemError, append, 42, 0)
        # CRASHES append(NULL, 0)

    def test_list_getslice(self):
        # Test PyList_GetSlice()
        getslice = _testlimitedcapi.list_getslice
        lst = [1, 2, 3]

        # empty
        self.assertEqual(getslice(lst, PY_SSIZE_T_MIN, 0), [])
        self.assertEqual(getslice(lst, -1, 0), [])
        self.assertEqual(getslice(lst, 3, PY_SSIZE_T_MAX), [])

        # slice
        self.assertEqual(getslice(lst, 1, 3), [2, 3])

        # whole
        self.assertEqual(getslice(lst, 0, len(lst)), lst)
        self.assertEqual(getslice(lst, 0, 100), lst)
        self.assertEqual(getslice(lst, -100, 100), lst)

        self.assertRaises(SystemError, getslice, (1, 2, 3), 0, 0)
        self.assertRaises(SystemError, getslice, 'abc', 0, 0)
        self.assertRaises(SystemError, getslice, 42, 0, 0)

        # CRASHES getslice(NULL, 0, 0)

    def test_list_setslice(self):
        # Test PyList_SetSlice()
        list_setslice = _testlimitedcapi.list_setslice
        def set_slice(lst, low, high, value):
            lst = lst.copy()
            self.assertEqual(list_setslice(lst, low, high, value), 0)
            return lst

        # insert items
        self.assertEqual(set_slice([], 0, 0, list("abc")), list("abc"))
        self.assertEqual(set_slice([], PY_SSIZE_T_MIN, PY_SSIZE_T_MIN, list("abc")), list("abc"))
        self.assertEqual(set_slice([], PY_SSIZE_T_MAX, PY_SSIZE_T_MAX, list("abc")), list("abc"))
        lst = list("abc")
        self.assertEqual(set_slice(lst, 0, 0, ["X"]), list("Xabc"))
        self.assertEqual(set_slice(lst, 1, 1, list("XY")), list("aXYbc"))
        self.assertEqual(set_slice(lst, len(lst), len(lst), ["X"]), list("abcX"))
        # self.assertEqual(set_slice(lst, PY_SSIZE_T_MAX, PY_SSIZE_T_MAX, ["X"]), list("abcX"))

        # replace items
        lst = list("abc")
        self.assertEqual(set_slice(lst, -100, 1, list("X")), list("Xbc"))
        self.assertEqual(set_slice(lst, 1, 2, list("X")), list("aXc"))
        self.assertEqual(set_slice(lst, 1, 3, list("XY")), list("aXY"))
        self.assertEqual(set_slice(lst, 0, 3, list("XYZ")), list("XYZ"))

        # delete items
        lst = list("abcdef")
        self.assertEqual(set_slice(lst, 0, len(lst), []), [])
        self.assertEqual(set_slice(lst, -100, 100, []), [])
        self.assertEqual(set_slice(lst, 1, 5, []), list("af"))
        self.assertEqual(set_slice(lst, 3, len(lst), []), list("abc"))

        # delete items with NULL
        lst = list("abcdef")
        self.assertEqual(set_slice(lst, 0, len(lst), NULL), [])
        self.assertEqual(set_slice(lst, 3, len(lst), NULL), list("abc"))

        self.assertRaises(SystemError, list_setslice, (), 0, 0, [])
        self.assertRaises(SystemError, list_setslice, 42, 0, 0, [])

        # Item finalizer modify the list (clear the list)
        lst = []
        lst.append(DelAppend(lst, 'zombie'))
        self.assertEqual(list_setslice(lst, 0, len(lst), NULL), 0)
        self.assertEqual(lst, ['zombie'])

        # Item finalizer modify the list (remove an list item)
        lst = []
        lst.append(DelAppend(lst, 'zombie'))
        lst.extend("abc")
        self.assertEqual(list_setslice(lst, 0, 1, NULL), 0)
        self.assertEqual(lst, ['a', 'b', 'c', 'zombie'])

        # CRASHES setslice(NULL, 0, 0, [])

    def test_list_sort(self):
        # Test PyList_Sort()
        sort = _testlimitedcapi.list_sort
        lst = [4, 6, 7, 3, 1, 5, 9, 2, 0, 8]
        sort(lst)
        self.assertEqual(lst, list(range(10)))

        lst2 = ListSubclass([4, 6, 7, 3, 1, 5, 9, 2, 0, 8])
        sort(lst2)
        self.assertEqual(lst2, list(range(10)))

        self.assertRaises(SystemError, sort, ())
        self.assertRaises(SystemError, sort, object())
        self.assertRaises(SystemError, sort, NULL)


    def test_list_reverse(self):
        # Test PyList_Reverse()
        reverse = _testlimitedcapi.list_reverse
        def list_reverse(lst):
            self.assertEqual(reverse(lst), 0)
            return lst

        self.assertEqual(list_reverse([]), [])
        self.assertEqual(list_reverse([2, 5, 10]), [10, 5, 2])
        self.assertEqual(list_reverse(list_reverse([2, 5, 10])), [2, 5, 10])

        self.assertRaises(SystemError, reverse, ())
        self.assertRaises(SystemError, reverse, object())
        self.assertRaises(SystemError, reverse, NULL)

    def test_list_astuple(self):
        # Test PyList_AsTuple()
        astuple = _testlimitedcapi.list_astuple
        self.assertEqual(astuple([]), ())
        self.assertEqual(astuple([[]]), ([],))
        self.assertEqual(astuple([2, 5, 10]), (2, 5, 10))

        self.assertRaises(SystemError, astuple, ())
        self.assertRaises(SystemError, astuple, object())
        self.assertRaises(SystemError, astuple, NULL)

    def test_list_clear(self):
        # Test PyList_Clear()
        list_clear = _testcapi.list_clear

        lst = [1, 2, 3]
        self.assertEqual(list_clear(lst), 0)
        self.assertEqual(lst, [])

        lst = []
        self.assertEqual(list_clear(lst), 0)
        self.assertEqual(lst, [])

        self.assertRaises(SystemError, list_clear, ())
        self.assertRaises(SystemError, list_clear, object())

        # Item finalizer modify the list
        lst = []
        lst.append(DelAppend(lst, 'zombie'))
        list_clear(lst)
        self.assertEqual(lst, ['zombie'])

        # CRASHES list_clear(NULL)

    def test_list_extend(self):
        # Test PyList_Extend()
        list_extend = _testcapi.list_extend

        for other_type in (list, tuple, str, iter):
            lst = list("ab")
            arg = other_type("def")
            self.assertEqual(list_extend(lst, arg), 0)
            self.assertEqual(lst, list("abdef"))

        # PyList_Extend(lst, lst)
        lst = list("abc")
        self.assertEqual(list_extend(lst, lst), 0)
        self.assertEqual(lst, list("abcabc"))

        self.assertRaises(TypeError, list_extend, [], object())
        self.assertRaises(SystemError, list_extend, (), list("abc"))

        # CRASHES list_extend(NULL, [])
        # CRASHES list_extend([], NULL)


if __name__ == "__main__":
    unittest.main()
