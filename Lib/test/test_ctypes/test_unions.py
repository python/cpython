import unittest
from ctypes import Union, c_char
from ._support import (_CData, UnionType, Py_TPFLAGS_DISALLOW_INSTANTIATION,
                       Py_TPFLAGS_IMMUTABLETYPE)


class ArrayTestCase(unittest.TestCase):
    def test_inheritance_hierarchy(self):
        self.assertEqual(Union.mro(), [Union, _CData, object])

        self.assertEqual(UnionType.__name__, "UnionType")
        self.assertEqual(type(UnionType), type)

    def test_type_flags(self):
        for cls in Union, UnionType:
            with self.subTest(cls=Union):
                self.assertTrue(Union.__flags__ & Py_TPFLAGS_IMMUTABLETYPE)
                self.assertFalse(Union.__flags__ & Py_TPFLAGS_DISALLOW_INSTANTIATION)

    def test_metaclass_details(self):
        # Abstract classes (whose metaclass __init__ was not called) can't be
        # instantiated directly
        NewUnion = UnionType.__new__(UnionType, 'NewUnion',
                                     (Union,), {})
        for cls in Union, NewUnion:
            with self.subTest(cls=cls):
                with self.assertRaisesRegex(TypeError, "abstract class"):
                    obj = cls()

        # Cannot call the metaclass __init__ more than once
        class T(Union):
            _fields_ = [("x", c_char),
                        ("y", c_char)]
        with self.assertRaisesRegex(SystemError, "already initialized"):
            UnionType.__init__(T, 'ptr', (), {})
