import enum
import unittest
from test.support import import_helper

_testlimitedcapi = import_helper.import_module('_testlimitedcapi')


class Constant(enum.IntEnum):
    Py_CONSTANT_NONE = 0
    Py_CONSTANT_FALSE = 1
    Py_CONSTANT_TRUE = 2
    Py_CONSTANT_ELLIPSIS = 3
    Py_CONSTANT_NOT_IMPLEMENTED = 4
    Py_CONSTANT_ZERO = 5
    Py_CONSTANT_ONE = 6
    Py_CONSTANT_EMPTY_STR = 7
    Py_CONSTANT_EMPTY_BYTES = 8
    Py_CONSTANT_EMPTY_TUPLE = 9

    INVALID_CONSTANT = Py_CONSTANT_EMPTY_TUPLE + 1


class CAPITest(unittest.TestCase):
    def check_get_constant(self, get_constant):
        self.assertIs(get_constant(Constant.Py_CONSTANT_NONE), None)
        self.assertIs(get_constant(Constant.Py_CONSTANT_FALSE), False)
        self.assertIs(get_constant(Constant.Py_CONSTANT_TRUE), True)
        self.assertIs(get_constant(Constant.Py_CONSTANT_ELLIPSIS), Ellipsis)
        self.assertIs(get_constant(Constant.Py_CONSTANT_NOT_IMPLEMENTED), NotImplemented)

        for constant_id, constant_type, value in (
            (Constant.Py_CONSTANT_ZERO, int, 0),
            (Constant.Py_CONSTANT_ONE, int, 1),
            (Constant.Py_CONSTANT_EMPTY_STR, str, ""),
            (Constant.Py_CONSTANT_EMPTY_BYTES, bytes, b""),
            (Constant.Py_CONSTANT_EMPTY_TUPLE, tuple, ()),
        ):
            with self.subTest(constant_id=constant_id):
                obj = get_constant(constant_id)
                self.assertEqual(type(obj), constant_type, obj)
                self.assertEqual(obj, value)

        with self.assertRaises(SystemError):
            get_constant(Constant.INVALID_CONSTANT)

    def test_get_constant(self):
        self.check_get_constant(_testlimitedcapi.get_constant)

    def test_get_constant_borrowed(self):
        self.check_get_constant(_testlimitedcapi.get_constant_borrowed)


if __name__ == "__main__":
    unittest.main()
