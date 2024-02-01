import unittest
from ctypes import Union
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
