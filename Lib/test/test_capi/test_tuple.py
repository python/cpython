import unittest
import sys
from collections import namedtuple
from test.support import import_helper
_testcapi = import_helper.import_module('_testcapi')

NULL = None
PY_SSIZE_T_MIN = _testcapi.PY_SSIZE_T_MIN
PY_SSIZE_T_MAX = _testcapi.PY_SSIZE_T_MAX

class TupleSubclass(tuple):
    pass

class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyTuple_Check()
        check = _testcapi.tuple_check
        self.assertTrue(check((1, 2)))
        self.assertTrue(check(()))
        self.assertFalse(check({1: 2}))
        self.assertFalse(check([1, 2]))
        self.assertFalse(check(42))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)


    def test_tuple_checkexact(self):
        # Test PyTuple_CheckExact()
        check = _testcapi.tuple_checkexact
        self.assertTrue(check((1, 2)))
        self.assertTrue(check(()))
        self.assertFalse(check(TupleSubclass((1, 2))))
        self.assertFalse(check({1: 2}))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)

    def test_tuple_new(self):
        # Test PyTuple_New()
        tuple_new = _testcapi.tuple_new
        tup = tuple_new(0)
        self.assertEqual(tup, ())
        self.assertIs(type(tup), tuple)
        tup2 = tuple_new(1)
        self.assertIsNot(tup2, tup)
        self.assertRaises(SystemError, tuple_new, NULL)
        self.assertRaises(SystemError, tuple_new, -1)


    def test_tuple_pack(self):
        # Test PyTuple_Pack()
        pass

    def test_tuple_size(self):
        # Test PyTuple_Size()
        size = _testcapi.tuple_size
        self.assertEqual(size((1, 2)), 2)
        self.assertEqual(size(TupleSubclass((1, 2))), 2)
        self.assertRaises(SystemError, size, {})
        self.assertRaises(SystemError, size, 23)
        self.assertRaises(SystemError, size, object())

        # CRASHES size(NULL)

    def test_tuple_get_size(self):
        # Test PyTuple_GET_SIZE()
        size = _testcapi.tuple_get_size
        self.assertEqual(size(()), 0)
        self.assertEqual(size((1, 2)), 2)
        self.assertEqual(size(TupleSubclass((1, 2))), 2)
        # CRASHES size(object())
        # CRASHES size(23)
        # CRASHES size({})
        # CRASHES size(UserList())
        # CRASHES size(NULL)


    def test_tuple_getitem(self):
        # Test PyTuple_GetItem()
        getitem = _testcapi.tuple_getitem
        tup = (1, 2, 3)
        self.assertEqual(getitem(tup, 0), 1)
        self.assertEqual(getitem(tup, len(tup)-1), 3)
        self.assertRaises(IndexError, getitem, tup, -1)
        self.assertRaises(IndexError, getitem, tup, PY_SSIZE_T_MIN)
        self.assertRaises(IndexError, getitem, tup, PY_SSIZE_T_MAX)
        self.assertRaises(IndexError, getitem, tup, len(tup))
        self.assertRaises(SystemError, getitem, 42, 1)

    #     # CRASHES getitem(NULL, 1)

    def test_tuple_get_item(self):
        # Test PyTuple_GET_ITEM()
        get_item = _testcapi.tuple_get_item
        tup = (1, 2, 3)
        self.assertEqual(get_item(tup, 0), 1)
        self.assertEqual(get_item(tup, 2), 3)

        # CRASHES for get_item(tup, -1)
        # CRASHES for get_item(tup, PY_SSIZE_T_MIN)
        # CRASHES for get_item(tup, PY_SSIZE_T_MAX)
        # CRASHES for out of index: get_item(tup, 3)
        # CRASHES get_item(21, 2)
        # CRASHES get_item(Null,1)

    def test_tuple_getslice(self):
        # Test PyTuple_GetSlice()
        getslice = _testcapi.tuple_getslice
        tup = (1,2,3)

        # empty
        self.assertEqual(getslice(tup, PY_SSIZE_T_MIN, 0), ())
        self.assertEqual(getslice(tup, -1, 0), ())
        self.assertEqual(getslice(tup, 3, PY_SSIZE_T_MAX), ())

        # slice
        self.assertEqual(getslice(tup, 1, 3), (2, 3))

        # whole
        self.assertEqual(getslice(tup, 0, len(tup)), tup)
        self.assertEqual(getslice(tup, 0, 100), tup)
        self.assertEqual(getslice(tup, -100, 100), tup)

        self.assertRaises(TypeError, tup, 'a', '2')

        # CRASHES getslice(NULL, 0, 0)


    def test_tuple_setitem(self):
        # Test PyTuple_SetItem()
        setitem = _testcapi.tuple_setitem
        tup = (1, 2, 3)
        self.assertRaises(SystemError, setitem, tup, 1, 0)
        self.assertRaises(SystemError, setitem, {}, 0, 5)
        self.assertRaises(SystemError, setitem, tup, PY_SSIZE_T_MIN, 5)
        self.assertRaises(SystemError, setitem, tup, PY_SSIZE_T_MAX, 5)
        self.assertRaises(SystemError, setitem, tup, -1, 5)
        self.assertRaises(SystemError, setitem, tup, len(tup) , 5)
        self.assertRaises(TypeError, setitem, 23, 'a', 5)
        self.assertRaises(TypeError, setitem, tup, 1.5, 10)

        # CRASHES setitem(NULL, 'a', 5)

    def test_tuple_set_item(self):
        # Test PyTuple_SET_ITEM()
        set_item = _testcapi.tuple_set_item
        tup = (1, 2, 3)
        set_item(tup, 1, (1, 2))
        self.assertEqual(tup, (1, (1, 2), 3))

        # CRASHES for set_item([1], PY_SSIZE_T_MIN, 5)
        # CRASHES for set_item([1], PY_SSIZE_T_MAX, 5)
        # CRASHES for set_item([1], -1, 5)
        # CRASHES for set_item([], 0, 1)
        # CRASHES for set_item(NULL, 0, 1)

    def test_tuple_resize(self):
        # Test PyTuple_Resize()
        pass


