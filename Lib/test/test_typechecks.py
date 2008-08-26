"""Unit tests for __instancecheck__ and __subclasscheck__."""

import unittest
from test import test_support


class ABC(type):

    def __instancecheck__(cls, inst):
        """Implement isinstance(inst, cls)."""
        return any(cls.__subclasscheck__(c)
                   for c in set([type(inst), inst.__class__]))

    def __subclasscheck__(cls, sub):
        """Implement issubclass(sub, cls)."""
        candidates = cls.__dict__.get("__subclass__", set()) | set([cls])
        return any(c in candidates for c in sub.mro())


class Integer:

    __metaclass__ = ABC

    __subclass__ = set([int])


class SubInt(Integer):

    pass


class Evil:
    def __instancecheck__(self, inst): return False


class TypeChecksTest(unittest.TestCase):

    def testIsSubclassInternal(self):
        self.assertEqual(Integer.__subclasscheck__(int), True)
        self.assertEqual(Integer.__subclasscheck__(float), False)

    def testIsSubclassBuiltin(self):
        self.assertEqual(issubclass(int, Integer), True)
        self.assertEqual(issubclass(int, (Integer,)), True)
        self.assertEqual(issubclass(float, Integer), False)
        self.assertEqual(issubclass(float, (Integer,)), False)

    def testIsInstanceBuiltin(self):
        self.assertEqual(isinstance(42, Integer), True)
        self.assertEqual(isinstance(42, (Integer,)), True)
        self.assertEqual(isinstance(3.14, Integer), False)
        self.assertEqual(isinstance(3.14, (Integer,)), False)

    def testIsInstanceActual(self):
        self.assertEqual(isinstance(Integer(), Integer), True)
        self.assertEqual(isinstance(Integer(), (Integer,)), True)

    def testIsSubclassActual(self):
        self.assertEqual(issubclass(Integer, Integer), True)
        self.assertEqual(issubclass(Integer, (Integer,)), True)

    def testSubclassBehavior(self):
        self.assertEqual(issubclass(SubInt, Integer), True)
        self.assertEqual(issubclass(SubInt, (Integer,)), True)
        self.assertEqual(issubclass(SubInt, SubInt), True)
        self.assertEqual(issubclass(SubInt, (SubInt,)), True)
        self.assertEqual(issubclass(Integer, SubInt), False)
        self.assertEqual(issubclass(Integer, (SubInt,)), False)
        self.assertEqual(issubclass(int, SubInt), False)
        self.assertEqual(issubclass(int, (SubInt,)), False)
        self.assertEqual(isinstance(SubInt(), Integer), True)
        self.assertEqual(isinstance(SubInt(), (Integer,)), True)
        self.assertEqual(isinstance(SubInt(), SubInt), True)
        self.assertEqual(isinstance(SubInt(), (SubInt,)), True)
        self.assertEqual(isinstance(42, SubInt), False)
        self.assertEqual(isinstance(42, (SubInt,)), False)

    def testInfiniteRecursionCaughtProperly(self):
        e = Evil()
        # This invokes isinstance() recursively, until the stack is exhausted.
        self.assertRaises(RuntimeError, isinstance, e, Evil)
        # XXX How to check the same situation for issubclass()?


def test_main():
    test_support.run_unittest(TypeChecksTest)


if __name__ == "__main__":
    unittest.main()
