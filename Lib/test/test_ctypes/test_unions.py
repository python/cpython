import ctypes
import sys
import unittest
import warnings
from ctypes import Union
from .support import _CData, UnionType


class ArrayTestCase(unittest.TestCase):
    def test_inheritance_hierarchy(self):
        self.assertEqual(Union.mro(), [Union, _CData, object])

        self.assertEqual(UnionType.__name__, "UnionType")
        self.assertEqual(type(UnionType), type)

    def test_instantiation(self):
        with self.assertRaisesRegex(TypeError, "abstract class"):
            Union()

        UnionType("Foo", (Union,), {})

    def test_immutability(self):
        Union.foo = "bar"

        msg = "cannot set 'foo' attribute of immutable type"
        with self.assertRaisesRegex(TypeError, msg):
            UnionType.foo = "bar"
