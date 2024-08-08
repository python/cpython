import unittest
from test.support import import_helper
_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')


NULL = None
PY_SSIZE_T_MIN = _testcapi.PY_SSIZE_T_MIN
PY_SSIZE_T_MAX = _testcapi.PY_SSIZE_T_MAX

class TupleSubclass(tuple):
    pass


class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyTuple_Check()
        check = _testlimitedcapi.tuple_check
        self.assertTrue(check((1, 2)))
        self.assertTrue(check(()))
        self.assertTrue(check(TupleSubclass([1, 2])))
        self.assertFalse(check({1: 2}))
        self.assertFalse(check([1, 2]))
        self.assertFalse(check(42))
        self.assertFalse(check(object()))
        # CRASHES check(NULL)

    def test_tuple_check_exact(self):
        # Test PyTuple_CheckExact()
        check = _testlimitedcapi.tuple_check_exact
        self.assertTrue(check((1,)))
        self.assertTrue(check(()))
        self.assertFalse(check(TupleSubclass([1])))
        self.assertFalse(check({1: 2}))
        self.assertFalse(check([1, 2]))
        self.assertFalse(check(42))
        self.assertFalse(check(object()))
        # CRASHES check(NULL)

    def test_tuple_size(self):
        # Test PyTuple_Size()
        size = _testlimitedcapi.tuple_size
        self.assertEqual(size((1, 2, 3)), 3)
        self.assertEqual(size(TupleSubclass((1, 2))), 2)
        self.assertRaises(SystemError, size, [])
        self.assertRaises(SystemError, size, 23)
        self.assertRaises(SystemError, size, object())
        # CRASHES size(NULL)

    def test_tuple_get_size(self):
        # Test PyTuple_GET_SIZE()
        size = _testcapi.tuple_get_size
        self.assertEqual(size((1, 2, 3)), 3)
        self.assertEqual(size(TupleSubclass((1, 2))), 2)
        # CRASHES size(object())
        # CRASHES size(23)
        # CRASHES size([])
        # CRASHES size(NULL)

    def test_tuple_getitem(self):
        # Test PyTuple_GetItem()
        getitem = _testlimitedcapi.tuple_getitem
        tpl = (1, 2, 3)
        self.assertEqual(getitem(tpl, 0), 1)
        self.assertEqual(getitem(tpl, 2), 3)

        self.assertRaises(IndexError, getitem, tpl, 3)
        self.assertRaises(IndexError, getitem, tpl, -1)
        self.assertRaises(IndexError, getitem, tpl, PY_SSIZE_T_MIN)
        self.assertRaises(IndexError, getitem, tpl, PY_SSIZE_T_MAX)

        self.assertRaises(SystemError, getitem, 42, 1)
        self.assertRaises(SystemError, getitem, [1, 2, 3], 1)
        self.assertRaises(SystemError, getitem, {1: 2}, 1)
        # CRASHES getitem(NULL, 1)

    def test_tuple_get_item(self):
        # Test PyTuple_GET_ITEM()
        get_item = _testcapi.tuple_get_item
        tpl = (1, 2, (1, 2, 3))
        self.assertEqual(get_item(tpl, 0), 1)
        self.assertEqual(get_item(tpl, 2), (1, 2, 3))
        # CRASHES for out of index: get_item(tpl, 3)
        # CRASHES for get_item(tpl, PY_SSIZE_T_MIN)
        # CRASHES for get_item(tpl, PY_SSIZE_T_MAX)
        # CRASHES get_item(21, 2)
        # CRASHES get_item(NULL, 1)

    def test_tuple_getslice(self):
        # Test PyTuple_GetSlice()
        getslice = _testlimitedcapi.tuple_getslice
        tpl = (1, 2, 3)

        # empty
        self.assertEqual(getslice(tpl, PY_SSIZE_T_MIN, 0), ())
        self.assertEqual(getslice(tpl, -1, 0), ())
        self.assertEqual(getslice(tpl, 3, PY_SSIZE_T_MAX), ())

        # slice
        self.assertEqual(getslice(tpl, 1, 3), (2, 3))

        # whole
        self.assertEqual(getslice(tpl, 0, len(tpl)), tpl)
        self.assertEqual(getslice(tpl, 0, 100), tpl)
        self.assertEqual(getslice(tpl, -100, 100), tpl)

        self.assertRaises(SystemError, getslice, [1, 2, 3], 0, 0)
        self.assertRaises(SystemError, getslice, 'abc', 0, 0)
        self.assertRaises(SystemError, getslice, 42, 0, 0)

        # CRASHES getslice(NULL, 0, 0)
