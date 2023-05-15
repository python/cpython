import types
import unittest
from test.support import check_syntax_error, run_code

from typing import Callable, TypeAliasType, TypeVar, get_args


class TypeParamsInvalidTest(unittest.TestCase):
    def test_name_collision_01(self):
        check_syntax_error(self, """type TA1[A, **A] = None""", "duplicate type parameter 'A'")

    def test_name_non_collision_02(self):
        ns = run_code("""type TA1[A] = lambda A: A""")
        self.assertIsInstance(ns["TA1"], TypeAliasType)
        self.assertTrue(callable(ns["TA1"].__value__))
        self.assertEqual("arg", ns["TA1"].__value__("arg"))

    def test_name_non_collision_03(self):
        ns = run_code("""
            class Outer[A]:
                type TA1[A] = None
            """
        )
        outer_A, = ns["Outer"].__type_params__
        inner_A, = ns["Outer"].TA1.__type_params__
        self.assertIsNot(outer_A, inner_A)


class TypeParamsAccessTest(unittest.TestCase):
    def test_alias_access_01(self):
        ns = run_code("type TA1[A, B] = dict[A, B]")
        alias = ns["TA1"]
        self.assertIsInstance(alias, TypeAliasType)
        self.assertEqual(alias.__type_params__, get_args(alias.__value__))

    def test_alias_access_02(self):
        ns = run_code("""
            type TA1[A, B] = TA1[A, B] | int
            """
        )
        alias = ns["TA1"]
        self.assertIsInstance(alias, TypeAliasType)
        A, B = alias.__type_params__
        self.assertEqual(alias.__value__, alias[A, B] | int)

    def test_alias_access_03(self):
        ns = run_code("""
            class Outer[A]:
                def inner[B](self):
                    type TA1[C] = TA1[A, B] | int
                    return TA1
            """
        )
        cls = ns["Outer"]
        A, = cls.__type_params__
        B, = cls.inner.__type_params__
        alias = cls.inner(None)
        self.assertIsInstance(alias, TypeAliasType)
        alias2 = cls.inner(None)
        self.assertIsNot(alias, alias2)
        self.assertEqual(len(alias.__type_params__), 1)

        self.assertEqual(alias.__value__, alias[A, B] | int)


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

    def test_alias_value_02(self):
        class Parent[A]:
            type TA1[B] = dict[A, B]

        self.assertIsInstance(Parent.TA1, TypeAliasType)
        self.assertEqual(len(Parent.TA1.__parameters__), 1)
        self.assertEqual(len(Parent.__parameters__), 1)
        a, = Parent.__parameters__
        b, = Parent.TA1.__parameters__
        self.assertEqual(Parent.__type_params__, (a,))
        self.assertEqual(Parent.TA1.__type_params__, (b,))
        self.assertEqual(Parent.TA1.__value__, dict[a, b])

    def test_alias_value_03(self):
        def outer[A]():
            type TA1[B] = dict[A, B]
            return TA1

        o = outer()
        self.assertIsInstance(o, TypeAliasType)
        self.assertEqual(len(o.__parameters__), 1)
        self.assertEqual(len(outer.__type_params__), 1)
        b = o.__parameters__[0]
        self.assertEqual(o.__type_params__, (b,))

    def test_alias_value_04(self):
        def more_generic[T, *Ts, **P]():
            type TA[T2, *Ts2, **P2] = tuple[Callable[P, tuple[T, *Ts]], Callable[P2, tuple[T2, *Ts2]]]
            return TA

        alias = more_generic()
        self.assertIsInstance(alias, TypeAliasType)
        T2, Ts2, P2 = alias.__type_params__
        self.assertEqual(alias.__parameters__, (T2, *Ts2, P2))
        T, Ts, P = more_generic.__type_params__
        self.assertEqual(alias.__value__, tuple[Callable[P, tuple[T, *Ts]], Callable[P2, tuple[T2, *Ts2]]])

    def test_subscripting(self):
        type NonGeneric = int
        type Generic[A] = dict[A, A]
        type VeryGeneric[T, *Ts, **P] = Callable[P, tuple[T, *Ts]]

        with self.assertRaises(TypeError):
            NonGeneric[int]

        specialized = Generic[int]
        self.assertIsInstance(specialized, types.GenericAlias)
        self.assertIs(specialized.__origin__, Generic)
        self.assertEqual(specialized.__args__, (int,))

        specialized2 = VeryGeneric[int, str, float, [bool, range]]
        self.assertIsInstance(specialized2, types.GenericAlias)
        self.assertIs(specialized2.__origin__, VeryGeneric)
        self.assertEqual(specialized2.__args__, (int, str, float, [bool, range]))

    def test_repr(self):
        type Simple = int
        self.assertEqual(repr(Simple), "Simple")

    def test_recursive_repr(self):
        type Recursive = Recursive
        self.assertEqual(repr(Recursive), "Recursive")

        type X = list[Y]
        type Y = list[X]
        self.assertEqual(repr(X), "X")


class TypeAliasConstructorTest(unittest.TestCase):
    def test_basic(self):
        TA = TypeAliasType("TA", int)
        self.assertEqual(TA.__name__, "TA")
        self.assertIs(TA.__value__, int)
        self.assertEqual(TA.__type_params__, ())

    def test_generic(self):
        T = TypeVar("T")
        TA = TypeAliasType("TA", list[T], type_params=(T,))
        self.assertEqual(TA.__name__, "TA")
        self.assertEqual(TA.__value__, list[T])
        self.assertEqual(TA.__type_params__, (T,))

    def test_keywords(self):
        TA = TypeAliasType(name="TA", value=int)
        self.assertEqual(TA.__name__, "TA")
        self.assertIs(TA.__value__, int)
        self.assertEqual(TA.__type_params__, ())

    def test_errors(self):
        with self.assertRaises(TypeError):
            TypeAliasType()
        with self.assertRaises(TypeError):
            TypeAliasType("TA")
        with self.assertRaises(TypeError):
            TypeAliasType("TA", list, ())
        with self.assertRaises(TypeError):
            TypeAliasType("TA", list, type_params=42)


class TypeAliasTypeTest(unittest.TestCase):
    def test_immutable(self):
        with self.assertRaises(TypeError):
            TypeAliasType.whatever = "not allowed"

    def test_no_subclassing(self):
        with self.assertRaisesRegex(TypeError, "not an acceptable base type"):
            class MyAlias(TypeAliasType):
                pass

    def test_union(self):
        type Alias1 = int
        type Alias2 = str
        union = Alias1 | Alias2
        self.assertIsInstance(union, types.UnionType)
        self.assertEqual(get_args(union), (Alias1, Alias2))
        union2 = Alias1 | list[float]
        self.assertIsInstance(union2, types.UnionType)
        self.assertEqual(get_args(union2), (Alias1, list[float]))
        union3 = list[range] | Alias1
        self.assertIsInstance(union3, types.UnionType)
        self.assertEqual(get_args(union3), (list[range], Alias1))
