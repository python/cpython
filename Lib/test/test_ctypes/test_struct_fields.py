import unittest
from ctypes import Structure, Union, sizeof, c_char, c_int
from ._support import (CField, Py_TPFLAGS_DISALLOW_INSTANTIATION,
                       Py_TPFLAGS_IMMUTABLETYPE)


class StructFieldsTestCase(unittest.TestCase):
    # Structure/Union classes must get 'finalized' sooner or
    # later, when one of these things happen:
    #
    # 1. _fields_ is set.
    # 2. An instance is created.
    # 3. The type is used as field of another Structure/Union.
    # 4. The type is subclassed
    #
    # When they are finalized, assigning _fields_ is no longer allowed.
    def test_1_A(self):
        class X(Structure):
            pass
        self.assertEqual(sizeof(X), 0) # not finalized
        X._fields_ = [] # finalized
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

    def test_1_B(self):
        class X(Structure):
            _fields_ = [] # finalized
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

    def test_2(self):
        class X(Structure):
            pass
        X()
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

    def test_3(self):
        class X(Structure):
            pass
        class Y(Structure):
            _fields_ = [("x", X)] # finalizes X
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

    def test_4(self):
        class X(Structure):
            pass
        class Y(X):
            pass
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])
        Y._fields_ = []
        self.assertRaises(AttributeError, setattr, X, "_fields_", [])

    def test_5(self):
        class X(Structure):
            _fields_ = (("char", c_char * 5),)

        x = X(b'#' * 5)
        x.char = b'a\0b\0'
        self.assertEqual(bytes(x), b'a\x00###')

    def test_6(self):
        self.assertRaises(TypeError, CField)

    def test_cfield_type_flags(self):
        self.assertTrue(CField.__flags__ & Py_TPFLAGS_DISALLOW_INSTANTIATION)
        self.assertTrue(CField.__flags__ & Py_TPFLAGS_IMMUTABLETYPE)

    def test_cfield_inheritance_hierarchy(self):
        self.assertEqual(CField.mro(), [CField, object])

    def test_gh99275(self):
        class BrokenStructure(Structure):
            def __init_subclass__(cls, **kwargs):
                cls._fields_ = []  # This line will fail, `stginfo` is not ready

        with self.assertRaisesRegex(TypeError,
                                    'ctypes state is not initialized'):
            class Subclass(BrokenStructure): ...

    # __set__ and __get__ should raise a TypeError in case their self
    # argument is not a ctype instance.
    def test___set__(self):
        class MyCStruct(Structure):
            _fields_ = (("field", c_int),)
        self.assertRaises(TypeError,
                          MyCStruct.field.__set__, 'wrong type self', 42)

        class MyCUnion(Union):
            _fields_ = (("field", c_int),)
        self.assertRaises(TypeError,
                          MyCUnion.field.__set__, 'wrong type self', 42)

    def test___get__(self):
        class MyCStruct(Structure):
            _fields_ = (("field", c_int),)
        self.assertRaises(TypeError,
                          MyCStruct.field.__get__, 'wrong type self', 42)

        class MyCUnion(Union):
            _fields_ = (("field", c_int),)
        self.assertRaises(TypeError,
                          MyCUnion.field.__get__, 'wrong type self', 42)


if __name__ == "__main__":
    unittest.main()
