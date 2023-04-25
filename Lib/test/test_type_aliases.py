import textwrap
import types
import unittest

from typing import TypeAliasType

class TypeParamsInvalidTest(unittest.TestCase):
    def test_name_collision_01(self):
        code = """type TA1[A, **A] = None"""

        with self.assertRaisesRegex(SyntaxError, "duplicate type parameter 'A'"):
            exec(code, {}, {})

    def test_name_non_collision_02(self):
        code = """type TA1[A] = lambda A: None"""
        exec(code, {}, {})

    def test_name_non_collision_03(self):
        code = textwrap.dedent("""\
            class Outer[A]:
                type TA1[A] = None
            """
        )
        exec(code, {}, {})


class TypeParamsAccessTest(unittest.TestCase):
    def test_alias_access_01(self):
        code = textwrap.dedent("""\
            type TA1[A, B] = dict[A, B]
            """
        )
        exec(code, {}, {})

    def test_alias_access_02(self):
        code = textwrap.dedent("""\
            type TA1[A, B] = TA1[A, B] | int
            """
        )
        exec(code, {}, {})

    def test_alias_access_03(self):
        code = textwrap.dedent("""\
            class Outer[A]:
                def inner[B](self):
                    type TA1[C] = TA1[A, B] | int
            """
        )
        exec(code, {}, {})


class TypeParamsAliasValueTest(unittest.TestCase):
    def test_alias_value_01(self):
        type TA1 = int

        self.assertIsInstance(TA1, TypeAliasType)
        self.assertEqual(TA1.__value__, int)
        self.assertEqual(TA1.__parameters__, ())
        self.assertEqual(TA1.__type_params__, ())

        type TA2 = TA1 | str

        self.assertIsInstance(TA2, TypeAliasType)
        a, b = TA2.__value__.__args__
        self.assertEqual(a, TA1)
        self.assertEqual(b, str)
        self.assertEqual(TA2.__parameters__, ())
        self.assertEqual(TA2.__type_params__, ())

    def test_alias_access_02(self):
        class Parent[A]:
            type TA1[B] = dict[A, B]

        self.assertIsInstance(Parent.TA1, TypeAliasType)
        self.assertEqual(len(Parent.TA1.__parameters__), 1)
        self.assertEqual(len(Parent.__parameters__), 1)
        a = Parent.__parameters__[0]
        b = Parent.TA1.__parameters__[0]
        self.assertEqual(Parent.TA1.__type_params__, (a, b))

    def test_alias_access_02(self):
        def outer[A]():
            type TA1[B] = dict[A, B]
            return TA1

        o = outer()
        self.assertIsInstance(o, TypeAliasType)
        self.assertEqual(len(o.__parameters__), 1)
        self.assertEqual(len(outer.__type_variables__), 1)
        b = o.__parameters__[0]
        self.assertEqual(o.__type_params__, (b,))

    def test_subscripting(self):
        type NonGeneric = int
        type Generic[A] = dict[A, A]

        with self.assertRaises(TypeError):
            NonGeneric[int]
        self.assertIsInstance(Generic[int], types.GenericAlias)
