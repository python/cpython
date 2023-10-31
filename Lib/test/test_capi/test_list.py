import unittest
from collections import UserList
import _testcapi


NULL = None
PY_SSIZE_T_MIN = _testcapi.PY_SSIZE_T_MIN
PY_SSIZE_T_MAX = _testcapi.PY_SSIZE_T_MAX

PyList_Check = _testcapi.list_check
PyList_CheckExact = _testcapi.list_checkexact
PyList_Size = _testcapi.list_size
PyList_GetItem = _testcapi.list_getitem
PyList_SetItem = _testcapi.list_setitem
PyList_Insert = _testcapi.list_insert
PyList_Append = _testcapi.list_append
PyList_Sort = _testcapi.list_sort
PyList_AsTuple = _testcapi.list_astuple
PyList_Reverse = _testcapi.list_reverse
PyList_GetSlice = _testcapi.list_getslice
PyList_SetSlice = _testcapi.list_setslice


class ListSubclass(list):
    pass


class CAPITest(unittest.TestCase):

    def test_list_check(self):
        self.assertTrue(PyList_Check([2, 3, 5]))
        # CRASHES self.assertFalse(PyList_Check(NULL))

    def test_list_checkexact(self):
        self.assertTrue(PyList_CheckExact([2, 3, 5]))
        # CRASHES self.assertFalse(PyList_CheckExact(NULL))

    def test_list_new(self):
        expected = [None, None, None]

        PyList_New = _testcapi.list_new
        lst = PyList_New(3)
        self.assertEqual(lst, expected)
        self.assertIs(type(lst), list)
        lst2 = PyList_New(3)
        self.assertIsNot(lst2, lst)

        with self.assertRaises(SystemError):
            PyList_New(-1)

    def test_list_size(self):
        self.assertEqual(PyList_Size([]), 0)
        self.assertEqual(PyList_Size([2, 3, 5]), 3)

    def test_list_getitem(self):
        lst = list("abc")
        self.assertEqual(PyList_GetItem(lst, 0), "a")
        self.assertEqual(PyList_GetItem(lst, 1), "b")
        self.assertEqual(PyList_GetItem(lst, 2), "c")

        for invalid_index in (PY_SSIZE_T_MIN, -1, len(lst), 10, PY_SSIZE_T_MAX):
            with self.subTest(invalid_index=invalid_index):
                self.assertRaises(IndexError, PyList_GetItem, lst, invalid_index)

        lst2 = ListSubclass(lst)
        self.assertEqual(PyList_GetItem(lst2, 1), "b")
        self.assertRaises(IndexError, PyList_GetItem, lst2, len(lst2))

        # CRASHES PyList_GetItem(NULL, 0)

    def test_list_setitem(self):
        lst = list("abc")
        PyList_SetItem(lst, 1, "X")
        self.assertEqual(lst, ["a", "X", "c"])
        self.assertRaises(IndexError, PyList_SetItem, lst, 3, "Y")

        lst2 = ListSubclass(list("abc"))
        PyList_SetItem(lst2, 1, "X")
        self.assertEqual(lst2, ["a", "X", "c"])

        # CRASHES PyList_SetItem(list("abc"), 0, NULL)
        # CRASHES PyList_SetItem(NULL, 0, 'x')

    def test_list_insert(self):
        lst = list("abc")
        PyList_Insert(lst, 1, "X")
        self.assertEqual(lst, ["a", "X", "b", "c"])
        PyList_Insert(lst, -1, "Y")
        self.assertEqual(lst, ["a", "X", "b", "Y", "c"])
        PyList_Insert(lst, 100, "Z")
        self.assertEqual(lst, ["a", "X", "b", "Y", "c", "Z"])
        PyList_Insert(lst, -100, "0")
        self.assertEqual(lst, ["0", "a", "X", "b", "Y", "c", "Z"])

        lst2 = ListSubclass(list("abc"))
        PyList_Insert(lst2, 1, "X")
        self.assertEqual(lst2, ["a", "X", "b", "c"])

        with self.assertRaises(SystemError):
            PyList_Insert(list("abc"), 0, NULL)

        # CRASHES PyList_Insert(NULL, 0, 'x')

    def test_list_append(self):
        lst = []
        PyList_Append(lst, "a")
        self.assertEqual(lst, ["a"])
        PyList_Append(lst, "b")
        self.assertEqual(lst, ["a", "b"])

        lst2 = ListSubclass()
        PyList_Append(lst2, "X")
        self.assertEqual(lst2, ["X"])

        self.assertRaises(SystemError, PyList_Append, [], NULL)

        # CRASHES PyList_Append(NULL, 'a')

    def test_list_sort(self):
        lst = [4, 6, 7, 3, 1, 5, 9, 2, 0, 8]
        PyList_Sort(lst)
        self.assertEqual(lst, list(range(10)))

        lst2 = ListSubclass([4, 6, 7, 3, 1, 5, 9, 2, 0, 8])
        PyList_Sort(lst2)
        self.assertEqual(lst2, list(range(10)))

        with self.assertRaises(SystemError):
            PyList_Sort(NULL)

    def test_list_astuple(self):
        self.assertEqual(PyList_AsTuple([]), ())
        self.assertEqual(PyList_AsTuple([2, 5, 10]), (2, 5, 10))

        with self.assertRaises(SystemError):
            PyList_AsTuple(NULL)

    def test_list_reverse(self):
        def list_reverse(lst):
            self.assertEqual(PyList_Reverse(lst), 0)
            return lst

        self.assertEqual(list_reverse([]), [])
        self.assertEqual(list_reverse([2, 5, 10]), [10, 5, 2])

        with self.assertRaises(SystemError):
            PyList_Reverse(NULL)

    def test_list_getslice(self):
        lst = list("abcdef")

        # empty
        self.assertEqual(PyList_GetSlice(lst, -100, 0), [])
        self.assertEqual(PyList_GetSlice(lst, 0, 0), [])
        self.assertEqual(PyList_GetSlice(lst, 3, 0), [])

        # slice
        self.assertEqual(PyList_GetSlice(lst, 1, 3), list("bc"))

        # whole
        self.assertEqual(PyList_GetSlice(lst, 0, len(lst)), lst)
        self.assertEqual(PyList_GetSlice(lst, 0, 100), lst)
        self.assertEqual(PyList_GetSlice(lst, -100, 100), lst)

        # CRASHES PyList_GetSlice(NULL, 0, 0)

    def test_list_setslice(self):
        def set_slice(lst, low, high, value):
            lst = lst.copy()
            self.assertEqual(PyList_SetSlice(lst, low, high, value), 0)
            return lst

        # insert items
        self.assertEqual(set_slice([], 0, 0, list("abc")), list("abc"))
        lst = list("abc")
        self.assertEqual(set_slice(lst, 0, 0, ["X"]), list("Xabc"))
        self.assertEqual(set_slice(lst, 1, 1, list("XY")), list("aXYbc"))
        self.assertEqual(set_slice(lst, len(lst), len(lst), ["X"]), list("abcX"))
        self.assertEqual(set_slice(lst, 100, 100, ["X"]), list("abcX"))

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

        # CRASHES PyList_SetSlice(NULL, 0, 0, ["x"])

    def test_not_list_objects(self):
        for obj in (
            123,
            UserList([2, 3, 5]),
            (2, 3, 5),  # tuple
            object(),
        ):
            with self.subTest(obj=obj):
                self.assertFalse(PyList_Check(obj))
                self.assertFalse(PyList_CheckExact(obj))
                with self.assertRaises(SystemError):
                    PyList_Size(obj)
                with self.assertRaises(SystemError):
                    PyList_SetItem(obj, 0, "x")
                with self.assertRaises(SystemError):
                    PyList_Insert(obj, 0, "x")
                with self.assertRaises(SystemError):
                    PyList_Append(obj, "x")
                with self.assertRaises(SystemError):
                    PyList_Sort(obj)
                with self.assertRaises(SystemError):
                    PyList_AsTuple(obj)
                with self.assertRaises(SystemError):
                    PyList_Reverse(obj)
                with self.assertRaises(SystemError):
                    PyList_GetSlice(obj, 0, 1)
                with self.assertRaises(SystemError):
                    PyList_SetSlice(obj, 0, 1, ["x"])


if __name__ == "__main__":
    unittest.main()
