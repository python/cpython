import pickle
import types
import unittest
from test.support import check_syntax_error, run_code
from test.typinganndata import mod_generics_cache

from typing import (
    Callable, TypeAliasType, TypeVar, TypeVarTuple, ParamSpec, Unpack, get_args,
)


class TypeParamsInvalidTest(unittest.TestCase):
    def test_name_collisions(self):
        check_syntax_error(self, 'type TA1[A, **A] = None', "duplicate type parameter 'A'")
        check_syntax_error(self, 'type T[A, *A] = None', "duplicate type parameter 'A'")
        check_syntax_error(self, 'type T[*A, **A] = None', "duplicate type parameter 'A'")

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
        type VeryGeneric[T, *Ts, **P] = Callable[P, tuple[T, *Ts]]

        self.assertEqual(repr(Simple), "Simple")
        self.assertEqual(repr(VeryGeneric), "VeryGeneric")
        self.assertEqual(repr(VeryGeneric[int, bytes, str, [float, object]]),
                         "VeryGeneric[int, bytes, str, [float, object]]")
        self.assertEqual(repr(VeryGeneric[int, []]),
                         "VeryGeneric[int, []]")
        self.assertEqual(repr(VeryGeneric[int, [VeryGeneric[int], list[str]]]),
                         "VeryGeneric[int, [VeryGeneric[int], list[str]]]")

    def test_recursive_repr(self):
        type Recursive = Recursive
        self.assertEqual(repr(Recursive), "Recursive")

        type X = list[Y]
        type Y = list[X]
        self.assertEqual(repr(X), "X")
        self.assertEqual(repr(Y), "Y")

        type GenericRecursive[X] = list[X | GenericRecursive[X]]
        self.assertEqual(repr(GenericRecursive), "GenericRecursive")
        self.assertEqual(repr(GenericRecursive[int]), "GenericRecursive[int]")
        self.assertEqual(repr(GenericRecursive[GenericRecursive[int]]),
                         "GenericRecursive[GenericRecursive[int]]")

    def test_raising(self):
        type MissingName = list[_My_X]
        with self.assertRaisesRegex(
            NameError,
            "cannot access free variable '_My_X' where it is not associated with a value",
        ):
            MissingName.__value__
        _My_X = int
        self.assertEqual(MissingName.__value__, list[int])
        del _My_X
        # Cache should still work:
        self.assertEqual(MissingName.__value__, list[int])

        # Explicit exception:
        type ExprException = 1 / 0
        with self.assertRaises(ZeroDivisionError):
            ExprException.__value__


class TypeAliasConstructorTest(unittest.TestCase):
    def test_basic(self):
        TA = TypeAliasType("TA", int)
        self.assertEqual(TA.__name__, "TA")
        self.assertIs(TA.__value__, int)
        self.assertEqual(TA.__type_params__, ())
        self.assertEqual(TA.__module__, __name__)

    def test_attributes_with_exec(self):
        ns = {}
        exec("type TA = int", ns, ns)
        TA = ns["TA"]
        self.assertEqual(TA.__name__, "TA")
        self.assertIs(TA.__value__, int)
        self.assertEqual(TA.__type_params__, ())
        self.assertIs(TA.__module__, None)

    def test_generic(self):
        T = TypeVar("T")
        TA = TypeAliasType("TA", list[T], type_params=(T,))
        self.assertEqual(TA.__name__, "TA")
        self.assertEqual(TA.__value__, list[T])
        self.assertEqual(TA.__type_params__, (T,))
        self.assertEqual(TA.__module__, __name__)
        self.assertIs(type(TA[int]), types.GenericAlias)

    def test_not_generic(self):
        TA = TypeAliasType("TA", list[int], type_params=())
        self.assertEqual(TA.__name__, "TA")
        self.assertEqual(TA.__value__, list[int])
        self.assertEqual(TA.__type_params__, ())
        self.assertEqual(TA.__module__, __name__)
        with self.assertRaisesRegex(
            TypeError,
            "Only generic type aliases are subscriptable",
        ):
            TA[int]

    def test_type_params_order_with_defaults(self):
        HasNoDefaultT = TypeVar("HasNoDefaultT")
        WithDefaultT = TypeVar("WithDefaultT", default=int)

        HasNoDefaultP = ParamSpec("HasNoDefaultP")
        WithDefaultP = ParamSpec("WithDefaultP", default=HasNoDefaultP)

        HasNoDefaultTT = TypeVarTuple("HasNoDefaultTT")
        WithDefaultTT = TypeVarTuple("WithDefaultTT", default=HasNoDefaultTT)

        for type_params in [
            (HasNoDefaultT, WithDefaultT),
            (HasNoDefaultP, WithDefaultP),
            (HasNoDefaultTT, WithDefaultTT),
        ]:
            with self.subTest(type_params=type_params):
                TypeAliasType("A", int, type_params=type_params)  # ok

        msg = "follows default type parameter"
        for type_params in [
            (WithDefaultT, HasNoDefaultT),
            (WithDefaultP, HasNoDefaultP),
            (WithDefaultTT, HasNoDefaultTT),
            (WithDefaultT, HasNoDefaultP),  # different types
        ]:
            with self.subTest(type_params=type_params):
                with self.assertRaisesRegex(TypeError, msg):
                    TypeAliasType("A", int, type_params=type_params)

    def test_expects_type_like(self):
        T = TypeVar("T")

        msg = "Expected a type param"
        with self.assertRaisesRegex(TypeError, msg):
            TypeAliasType("A", int, type_params=(1,))
        with self.assertRaisesRegex(TypeError, msg):
            TypeAliasType("A", int, type_params=(1, 2))
        with self.assertRaisesRegex(TypeError, msg):
            TypeAliasType("A", int, type_params=(T, 2))

    def test_keywords(self):
        TA = TypeAliasType(name="TA", value=int)
        self.assertEqual(TA.__name__, "TA")
        self.assertIs(TA.__value__, int)
        self.assertEqual(TA.__type_params__, ())
        self.assertEqual(TA.__module__, __name__)

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

    def test_module(self):
        self.assertEqual(TypeAliasType.__module__, "typing")
        type Alias = int
        self.assertEqual(Alias.__module__, __name__)
        self.assertEqual(mod_generics_cache.Alias.__module__,
                         mod_generics_cache.__name__)
        self.assertEqual(mod_generics_cache.OldStyle.__module__,
                         mod_generics_cache.__name__)

    def test_unpack(self):
        type Alias = tuple[int, int]
        unpacked = (*Alias,)[0]
        self.assertEqual(unpacked, Unpack[Alias])

        class Foo[*Ts]:
            pass

        x = Foo[str, *Alias]
        self.assertEqual(x.__args__, (str, Unpack[Alias]))


# All these type aliases are used for pickling tests:
T = TypeVar('T')
type SimpleAlias = int
type RecursiveAlias = dict[str, RecursiveAlias]
type GenericAlias[X] = list[X]
type GenericAliasMultipleTypes[X, Y] = dict[X, Y]
type RecursiveGenericAlias[X] = dict[str, RecursiveAlias[X]]
type BoundGenericAlias[X: int] = set[X]
type ConstrainedGenericAlias[LongName: (str, bytes)] = list[LongName]
type AllTypesAlias[A, *B, **C] = Callable[C, A] | tuple[*B]


class TypeAliasPickleTest(unittest.TestCase):
    def test_pickling(self):
        things_to_test = [
            SimpleAlias,
            RecursiveAlias,

            GenericAlias,
            GenericAlias[T],
            GenericAlias[int],

            GenericAliasMultipleTypes,
            GenericAliasMultipleTypes[str, T],
            GenericAliasMultipleTypes[T, str],
            GenericAliasMultipleTypes[int, str],

            RecursiveGenericAlias,
            RecursiveGenericAlias[T],
            RecursiveGenericAlias[int],

            BoundGenericAlias,
            BoundGenericAlias[int],
            BoundGenericAlias[T],

            ConstrainedGenericAlias,
            ConstrainedGenericAlias[str],
            ConstrainedGenericAlias[T],

            AllTypesAlias,
            AllTypesAlias[int, str, T, [T, object]],

            # Other modules:
            mod_generics_cache.Alias,
            mod_generics_cache.OldStyle,
        ]
        for thing in things_to_test:
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(thing=thing, proto=proto):
                    pickled = pickle.dumps(thing, protocol=proto)
                    self.assertEqual(pickle.loads(pickled), thing)

    type ClassLevel = str

    def test_pickling_local(self):
        type A = int
        things_to_test = [
            self.ClassLevel,
            A,
        ]
        for thing in things_to_test:
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(thing=thing, proto=proto):
                    with self.assertRaises(pickle.PickleError):
                        pickle.dumps(thing, protocol=proto)


class TypeParamsExoticGlobalsTest(unittest.TestCase):
    def test_exec_with_unusual_globals(self):
        class customdict(dict):
            def __missing__(self, key):
                return key

        code = compile("type Alias = undefined", "test", "exec")
        ns = customdict()
        exec(code, ns)
        Alias = ns["Alias"]
        self.assertEqual(Alias.__value__, "undefined")

        code = compile("class A: type Alias = undefined", "test", "exec")
        ns = customdict()
        exec(code, ns)
        Alias = ns["A"].Alias
        self.assertEqual(Alias.__value__, "undefined")
