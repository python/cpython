import unittest
from ctypes import Union
from .support import (_CData, UnionType, Py_TPFLAGS_DISALLOW_INSTANTIATION,
                      Py_TPFLAGS_IMMUTABLETYPE)


class ArrayTestCase(unittest.TestCase):
    def test_inheritance_hierarchy(self):
        self.assertEqual(Union.mro(), [Union, _CData, object])

        self.assertEqual(UnionType.__name__, "UnionType")
        self.assertEqual(type(UnionType), type)

    def test_type_flags(self):
        self.assertTrue(Union.__flags__ & Py_TPFLAGS_IMMUTABLETYPE)
        self.assertFalse(Union.__flags__ & Py_TPFLAGS_DISALLOW_INSTANTIATION)

        self.assertTrue(UnionType.__flags__ & Py_TPFLAGS_IMMUTABLETYPE)
        self.assertFalse(UnionType.__flags__ & Py_TPFLAGS_DISALLOW_INSTANTIATION)
