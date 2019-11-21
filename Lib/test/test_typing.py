import contextlib
import collections
import pickle
import re
import sys
from unittest import TestCase, main, skipUnless, SkipTest, skip
from copy import copy, deepcopy

from typing import Any, NoReturn
from typing import TypeVar, AnyStr
from typing import T, KT, VT  # Not in __all__.
from typing import Union, Optional
from typing import Tuple, List, MutableMapping
from typing import Callable
from typing import Generic, ClassVar
from typing import cast
from typing import get_type_hints
from typing import no_type_check, no_type_check_decorator
from typing import Type
from typing import NewType
from typing import NamedTuple
from typing import IO, TextIO, BinaryIO
from typing import Pattern, Match
import abc
import typing
import weakref

from test import mod_generics_cache


class BaseTestCase(TestCase):

    def assertIsSubclass(self, cls, class_or_tuple, msg=None):
        if not issubclass(cls, class_or_tuple):
            message = '%r is not a subclass of %r' % (cls, class_or_tuple)
            if msg is not None:
                message += ' : %s' % msg
            raise self.failureException(message)

    def assertNotIsSubclass(self, cls, class_or_tuple, msg=None):
        if issubclass(cls, class_or_tuple):
            message = '%r is a subclass of %r' % (cls, class_or_tuple)
            if msg is not None:
                message += ' : %s' % msg
            raise self.failureException(message)

    def clear_caches(self):
        for f in typing._cleanups:
            f()


class Employee:
    pass


class Manager(Employee):
    pass


class Founder(Employee):
    pass


class ManagingFounder(Manager, Founder):
    pass


class AnyTests(BaseTestCase):

    def test_any_instance_type_error(self):
        with self.assertRaises(TypeError):
            isinstance(42, Any)

    def test_any_subclass_type_error(self):
        with self.assertRaises(TypeError):
            issubclass(Employee, Any)
        with self.assertRaises(TypeError):
            issubclass(Any, Employee)

    def test_repr(self):
        self.assertEqual(repr(Any), 'typing.Any')

    def test_errors(self):
        with self.assertRaises(TypeError):
            issubclass(42, Any)
        with self.assertRaises(TypeError):
            Any[int]  # Any is not a generic type.

    def test_cannot_subclass(self):
        with self.assertRaises(TypeError):
            class A(Any):
                pass
        with self.assertRaises(TypeError):
            class A(type(Any)):
                pass

    def test_cannot_instantiate(self):
        with self.assertRaises(TypeError):
            Any()
        with self.assertRaises(TypeError):
            type(Any)()

    def test_any_works_with_alias(self):
        # These expressions must simply not fail.
        typing.Match[Any]
        typing.Pattern[Any]
        typing.IO[Any]


class NoReturnTests(BaseTestCase):

    def test_noreturn_instance_type_error(self):
        with self.assertRaises(TypeError):
            isinstance(42, NoReturn)

    def test_noreturn_subclass_type_error(self):
        with self.assertRaises(TypeError):
            issubclass(Employee, NoReturn)
        with self.assertRaises(TypeError):
            issubclass(NoReturn, Employee)

    def test_repr(self):
        self.assertEqual(repr(NoReturn), 'typing.NoReturn')

    def test_not_generic(self):
        with self.assertRaises(TypeError):
            NoReturn[int]

    def test_cannot_subclass(self):
        with self.assertRaises(TypeError):
            class A(NoReturn):
                pass
        with self.assertRaises(TypeError):
            class A(type(NoReturn)):
                pass

    def test_cannot_instantiate(self):
        with self.assertRaises(TypeError):
            NoReturn()
        with self.assertRaises(TypeError):
            type(NoReturn)()


class TypeVarTests(BaseTestCase):

    def test_basic_plain(self):
        T = TypeVar('T')
        # T equals itself.
        self.assertEqual(T, T)
        # T is an instance of TypeVar
        self.assertIsInstance(T, TypeVar)

    def test_typevar_instance_type_error(self):
        T = TypeVar('T')
        with self.assertRaises(TypeError):
            isinstance(42, T)

    def test_typevar_subclass_type_error(self):
        T = TypeVar('T')
        with self.assertRaises(TypeError):
            issubclass(int, T)
        with self.assertRaises(TypeError):
            issubclass(T, int)

    def test_constrained_error(self):
        with self.assertRaises(TypeError):
            X = TypeVar('X', int)
            X

    def test_union_unique(self):
        X = TypeVar('X')
        Y = TypeVar('Y')
        self.assertNotEqual(X, Y)
        self.assertEqual(Union[X], X)
        self.assertNotEqual(Union[X], Union[X, Y])
        self.assertEqual(Union[X, X], X)
        self.assertNotEqual(Union[X, int], Union[X])
        self.assertNotEqual(Union[X, int], Union[int])
        self.assertEqual(Union[X, int].__args__, (X, int))
        self.assertEqual(Union[X, int].__parameters__, (X,))
        self.assertIs(Union[X, int].__origin__, Union)

    def test_union_constrained(self):
        A = TypeVar('A', str, bytes)
        self.assertNotEqual(Union[A, str], Union[A])

    def test_repr(self):
        self.assertEqual(repr(T), '~T')
        self.assertEqual(repr(KT), '~KT')
        self.assertEqual(repr(VT), '~VT')
        self.assertEqual(repr(AnyStr), '~AnyStr')
        T_co = TypeVar('T_co', covariant=True)
        self.assertEqual(repr(T_co), '+T_co')
        T_contra = TypeVar('T_contra', contravariant=True)
        self.assertEqual(repr(T_contra), '-T_contra')

    def test_no_redefinition(self):
        self.assertNotEqual(TypeVar('T'), TypeVar('T'))
        self.assertNotEqual(TypeVar('T', int, str), TypeVar('T', int, str))

    def test_cannot_subclass_vars(self):
        with self.assertRaises(TypeError):
            class V(TypeVar('T')):
                pass

    def test_cannot_subclass_var_itself(self):
        with self.assertRaises(TypeError):
            class V(TypeVar):
                pass

    def test_cannot_instantiate_vars(self):
        with self.assertRaises(TypeError):
            TypeVar('A')()

    def test_bound_errors(self):
        with self.assertRaises(TypeError):
            TypeVar('X', bound=42)
        with self.assertRaises(TypeError):
            TypeVar('X', str, float, bound=Employee)

    def test_no_bivariant(self):
        with self.assertRaises(ValueError):
            TypeVar('T', covariant=True, contravariant=True)


class UnionTests(BaseTestCase):

    def test_basics(self):
        u = Union[int, float]
        self.assertNotEqual(u, Union)

    def test_subclass_error(self):
        with self.assertRaises(TypeError):
            issubclass(int, Union)
        with self.assertRaises(TypeError):
            issubclass(Union, int)
        with self.assertRaises(TypeError):
            issubclass(int, Union[int, str])
        with self.assertRaises(TypeError):
            issubclass(Union[int, str], int)

    def test_union_any(self):
        u = Union[Any]
        self.assertEqual(u, Any)
        u1 = Union[int, Any]
        u2 = Union[Any, int]
        u3 = Union[Any, object]
        self.assertEqual(u1, u2)
        self.assertNotEqual(u1, Any)
        self.assertNotEqual(u2, Any)
        self.assertNotEqual(u3, Any)

    def test_union_object(self):
        u = Union[object]
        self.assertEqual(u, object)
        u1 = Union[int, object]
        u2 = Union[object, int]
        self.assertEqual(u1, u2)
        self.assertNotEqual(u1, object)
        self.assertNotEqual(u2, object)

    def test_unordered(self):
        u1 = Union[int, float]
        u2 = Union[float, int]
        self.assertEqual(u1, u2)

    def test_single_class_disappears(self):
        t = Union[Employee]
        self.assertIs(t, Employee)

    def test_base_class_kept(self):
        u = Union[Employee, Manager]
        self.assertNotEqual(u, Employee)
        self.assertIn(Employee, u.__args__)
        self.assertIn(Manager, u.__args__)

    def test_union_union(self):
        u = Union[int, float]
        v = Union[u, Employee]
        self.assertEqual(v, Union[int, float, Employee])

    def test_repr(self):
        self.assertEqual(repr(Union), 'typing.Union')
        u = Union[Employee, int]
        self.assertEqual(repr(u), 'typing.Union[%s.Employee, int]' % __name__)
        u = Union[int, Employee]
        self.assertEqual(repr(u), 'typing.Union[int, %s.Employee]' % __name__)
        T = TypeVar('T')
        u = Union[T, int][int]
        self.assertEqual(repr(u), repr(int))
        u = Union[List[int], int]
        self.assertEqual(repr(u), 'typing.Union[typing.List[int], int]')

    def test_cannot_subclass(self):
        with self.assertRaises(TypeError):
            class C(Union):
                pass
        with self.assertRaises(TypeError):
            class C(type(Union)):
                pass
        with self.assertRaises(TypeError):
            class C(Union[int, str]):
                pass

    def test_cannot_instantiate(self):
        with self.assertRaises(TypeError):
            Union()
        with self.assertRaises(TypeError):
            type(Union)()
        u = Union[int, float]
        with self.assertRaises(TypeError):
            u()
        with self.assertRaises(TypeError):
            type(u)()

    def test_union_generalization(self):
        self.assertFalse(Union[str, typing.Iterable[int]] == str)
        self.assertFalse(Union[str, typing.Iterable[int]] == typing.Iterable[int])
        self.assertIn(str, Union[str, typing.Iterable[int]].__args__)
        self.assertIn(typing.Iterable[int], Union[str, typing.Iterable[int]].__args__)

    def test_union_compare_other(self):
        self.assertNotEqual(Union, object)
        self.assertNotEqual(Union, Any)
        self.assertNotEqual(ClassVar, Union)
        self.assertNotEqual(Optional, Union)
        self.assertNotEqual([None], Optional)
        self.assertNotEqual(Optional, typing.Mapping)
        self.assertNotEqual(Optional[typing.MutableMapping], Union)

    def test_optional(self):
        o = Optional[int]
        u = Union[int, None]
        self.assertEqual(o, u)

    def test_empty(self):
        with self.assertRaises(TypeError):
            Union[()]

    def test_union_instance_type_error(self):
        with self.assertRaises(TypeError):
            isinstance(42, Union[int, str])

    def test_no_eval_union(self):
        u = Union[int, str]
        def f(x: u): ...
        self.assertIs(get_type_hints(f)['x'], u)

    def test_function_repr_union(self):
        def fun() -> int: ...
        self.assertEqual(repr(Union[fun, int]), 'typing.Union[fun, int]')

    def test_union_str_pattern(self):
        # Shouldn't crash; see http://bugs.python.org/issue25390
        A = Union[str, Pattern]
        A

    def test_etree(self):
        # See https://github.com/python/typing/issues/229
        # (Only relevant for Python 2.)
        try:
            from xml.etree.cElementTree import Element
        except ImportError:
            raise SkipTest("cElementTree not found")
        Union[Element, str]  # Shouldn't crash

        def Elem(*args):
            return Element(*args)

        Union[Elem, str]  # Nor should this


class TupleTests(BaseTestCase):

    def test_basics(self):
        with self.assertRaises(TypeError):
            issubclass(Tuple, Tuple[int, str])
        with self.assertRaises(TypeError):
            issubclass(tuple, Tuple[int, str])

        class TP(tuple): ...
        self.assertTrue(issubclass(tuple, Tuple))
        self.assertTrue(issubclass(TP, Tuple))

    def test_equality(self):
        self.assertEqual(Tuple[int], Tuple[int])
        self.assertEqual(Tuple[int, ...], Tuple[int, ...])
        self.assertNotEqual(Tuple[int], Tuple[int, int])
        self.assertNotEqual(Tuple[int], Tuple[int, ...])

    def test_tuple_subclass(self):
        class MyTuple(tuple):
            pass
        self.assertTrue(issubclass(MyTuple, Tuple))

    def test_tuple_instance_type_error(self):
        with self.assertRaises(TypeError):
            isinstance((0, 0), Tuple[int, int])
        self.assertIsInstance((0, 0), Tuple)

    def test_repr(self):
        self.assertEqual(repr(Tuple), 'typing.Tuple')
        self.assertEqual(repr(Tuple[()]), 'typing.Tuple[()]')
        self.assertEqual(repr(Tuple[int, float]), 'typing.Tuple[int, float]')
        self.assertEqual(repr(Tuple[int, ...]), 'typing.Tuple[int, ...]')

    def test_errors(self):
        with self.assertRaises(TypeError):
            issubclass(42, Tuple)
        with self.assertRaises(TypeError):
            issubclass(42, Tuple[int])


class CallableTests(BaseTestCase):

    def test_self_subclass(self):
        with self.assertRaises(TypeError):
            self.assertTrue(issubclass(type(lambda x: x), Callable[[int], int]))
        self.assertTrue(issubclass(type(lambda x: x), Callable))

    def test_eq_hash(self):
        self.assertEqual(Callable[[int], int], Callable[[int], int])
        self.assertEqual(len({Callable[[int], int], Callable[[int], int]}), 1)
        self.assertNotEqual(Callable[[int], int], Callable[[int], str])
        self.assertNotEqual(Callable[[int], int], Callable[[str], int])
        self.assertNotEqual(Callable[[int], int], Callable[[int, int], int])
        self.assertNotEqual(Callable[[int], int], Callable[[], int])
        self.assertNotEqual(Callable[[int], int], Callable)

    def test_cannot_instantiate(self):
        with self.assertRaises(TypeError):
            Callable()
        with self.assertRaises(TypeError):
            type(Callable)()
        c = Callable[[int], str]
        with self.assertRaises(TypeError):
            c()
        with self.assertRaises(TypeError):
            type(c)()

    def test_callable_wrong_forms(self):
        with self.assertRaises(TypeError):
            Callable[[...], int]
        with self.assertRaises(TypeError):
            Callable[(), int]
        with self.assertRaises(TypeError):
            Callable[[()], int]
        with self.assertRaises(TypeError):
            Callable[[int, 1], 2]
        with self.assertRaises(TypeError):
            Callable[int]

    def test_callable_instance_works(self):
        def f():
            pass
        self.assertIsInstance(f, Callable)
        self.assertNotIsInstance(None, Callable)

    def test_callable_instance_type_error(self):
        def f():
            pass
        with self.assertRaises(TypeError):
            self.assertIsInstance(f, Callable[[], None])
        with self.assertRaises(TypeError):
            self.assertIsInstance(f, Callable[[], Any])
        with self.assertRaises(TypeError):
            self.assertNotIsInstance(None, Callable[[], None])
        with self.assertRaises(TypeError):
            self.assertNotIsInstance(None, Callable[[], Any])

    def test_repr(self):
        ct0 = Callable[[], bool]
        self.assertEqual(repr(ct0), 'typing.Callable[[], bool]')
        ct2 = Callable[[str, float], int]
        self.assertEqual(repr(ct2), 'typing.Callable[[str, float], int]')
        ctv = Callable[..., str]
        self.assertEqual(repr(ctv), 'typing.Callable[..., str]')

    def test_callable_with_ellipsis(self):

        def foo(a: Callable[..., T]):
            pass

        self.assertEqual(get_type_hints(foo, globals(), locals()),
                         {'a': Callable[..., T]})

    def test_ellipsis_in_generic(self):
        # Shouldn't crash; see https://github.com/python/typing/issues/259
        typing.List[Callable[..., str]]


XK = TypeVar('XK', str, bytes)
XV = TypeVar('XV')


class SimpleMapping(Generic[XK, XV]):

    def __getitem__(self, key: XK) -> XV:
        ...

    def __setitem__(self, key: XK, value: XV):
        ...

    def get(self, key: XK, default: XV = None) -> XV:
        ...


class MySimpleMapping(SimpleMapping[XK, XV]):

    def __init__(self):
        self.store = {}

    def __getitem__(self, key: str):
        return self.store[key]

    def __setitem__(self, key: str, value):
        self.store[key] = value

    def get(self, key: str, default=None):
        try:
            return self.store[key]
        except KeyError:
            return default


class ProtocolTests(BaseTestCase):

    def test_supports_int(self):
        self.assertIsSubclass(int, typing.SupportsInt)
        self.assertNotIsSubclass(str, typing.SupportsInt)

    def test_supports_float(self):
        self.assertIsSubclass(float, typing.SupportsFloat)
        self.assertNotIsSubclass(str, typing.SupportsFloat)

    def test_supports_complex(self):

        # Note: complex itself doesn't have __complex__.
        class C:
            def __complex__(self):
                return 0j

        self.assertIsSubclass(C, typing.SupportsComplex)
        self.assertNotIsSubclass(str, typing.SupportsComplex)

    def test_supports_bytes(self):

        # Note: bytes itself doesn't have __bytes__.
        class B:
            def __bytes__(self):
                return b''

        self.assertIsSubclass(B, typing.SupportsBytes)
        self.assertNotIsSubclass(str, typing.SupportsBytes)

    def test_supports_abs(self):
        self.assertIsSubclass(float, typing.SupportsAbs)
        self.assertIsSubclass(int, typing.SupportsAbs)
        self.assertNotIsSubclass(str, typing.SupportsAbs)

    def test_supports_round(self):
        issubclass(float, typing.SupportsRound)
        self.assertIsSubclass(float, typing.SupportsRound)
        self.assertIsSubclass(int, typing.SupportsRound)
        self.assertNotIsSubclass(str, typing.SupportsRound)

    def test_reversible(self):
        self.assertIsSubclass(list, typing.Reversible)
        self.assertNotIsSubclass(int, typing.Reversible)

    def test_protocol_instance_type_error(self):
        with self.assertRaises(TypeError):
            isinstance(0, typing.SupportsAbs)
        class C1(typing.SupportsInt):
            def __int__(self) -> int:
                return 42
        class C2(C1):
            pass
        c = C2()
        self.assertIsInstance(c, C1)


class GenericTests(BaseTestCase):

    def test_basics(self):
        X = SimpleMapping[str, Any]
        self.assertEqual(X.__parameters__, ())
        with self.assertRaises(TypeError):
            X[str]
        with self.assertRaises(TypeError):
            X[str, str]
        Y = SimpleMapping[XK, str]
        self.assertEqual(Y.__parameters__, (XK,))
        Y[str]
        with self.assertRaises(TypeError):
            Y[str, str]
        SM1 = SimpleMapping[str, int]
        with self.assertRaises(TypeError):
            issubclass(SM1, SimpleMapping)
        self.assertIsInstance(SM1(), SimpleMapping)

    def test_generic_errors(self):
        T = TypeVar('T')
        S = TypeVar('S')
        with self.assertRaises(TypeError):
            Generic[T]()
        with self.assertRaises(TypeError):
            Generic[T][T]
        with self.assertRaises(TypeError):
            Generic[T][S]
        with self.assertRaises(TypeError):
            class C(Generic[T], Generic[T]): ...
        with self.assertRaises(TypeError):
            isinstance([], List[int])
        with self.assertRaises(TypeError):
            issubclass(list, List[int])
        with self.assertRaises(TypeError):
            class NewGeneric(Generic): ...
        with self.assertRaises(TypeError):
            class MyGeneric(Generic[T], Generic[S]): ...
        with self.assertRaises(TypeError):
            class MyGeneric(List[T], Generic[S]): ...

    def test_init(self):
        T = TypeVar('T')
        S = TypeVar('S')
        with self.assertRaises(TypeError):
            Generic[T, T]
        with self.assertRaises(TypeError):
            Generic[T, S, T]

    def test_init_subclass(self):
        class X(typing.Generic[T]):
            def __init_subclass__(cls, **kwargs):
                super().__init_subclass__(**kwargs)
                cls.attr = 42
        class Y(X):
            pass
        self.assertEqual(Y.attr, 42)
        with self.assertRaises(AttributeError):
            X.attr
        X.attr = 1
        Y.attr = 2
        class Z(Y):
            pass
        class W(X[int]):
            pass
        self.assertEqual(Y.attr, 2)
        self.assertEqual(Z.attr, 42)
        self.assertEqual(W.attr, 42)

    def test_repr(self):
        self.assertEqual(repr(SimpleMapping),
                         f"<class '{__name__}.SimpleMapping'>")
        self.assertEqual(repr(MySimpleMapping),
                         f"<class '{__name__}.MySimpleMapping'>")

    def test_chain_repr(self):
        T = TypeVar('T')
        S = TypeVar('S')

        class C(Generic[T]):
            pass

        X = C[Tuple[S, T]]
        self.assertEqual(X, C[Tuple[S, T]])
        self.assertNotEqual(X, C[Tuple[T, S]])

        Y = X[T, int]
        self.assertEqual(Y, X[T, int])
        self.assertNotEqual(Y, X[S, int])
        self.assertNotEqual(Y, X[T, str])

        Z = Y[str]
        self.assertEqual(Z, Y[str])
        self.assertNotEqual(Z, Y[int])
        self.assertNotEqual(Z, Y[T])

        self.assertTrue(str(Z).endswith(
            '.C[typing.Tuple[str, int]]'))

    def test_new_repr(self):
        T = TypeVar('T')
        U = TypeVar('U', covariant=True)
        S = TypeVar('S')

        self.assertEqual(repr(List), 'typing.List')
        self.assertEqual(repr(List[T]), 'typing.List[~T]')
        self.assertEqual(repr(List[U]), 'typing.List[+U]')
        self.assertEqual(repr(List[S][T][int]), 'typing.List[int]')
        self.assertEqual(repr(List[int]), 'typing.List[int]')

    def test_new_repr_complex(self):
        T = TypeVar('T')
        TS = TypeVar('TS')

        self.assertEqual(repr(typing.Mapping[T, TS][TS, T]), 'typing.Mapping[~TS, ~T]')
        self.assertEqual(repr(List[Tuple[T, TS]][int, T]),
                         'typing.List[typing.Tuple[int, ~T]]')
        self.assertEqual(
            repr(List[Tuple[T, T]][List[int]]),
            'typing.List[typing.Tuple[typing.List[int], typing.List[int]]]'
        )

    def test_new_repr_bare(self):
        T = TypeVar('T')
        self.assertEqual(repr(Generic[T]), 'typing.Generic[~T]')
        self.assertEqual(repr(typing._Protocol[T]), 'typing._Protocol[~T]')
        class C(typing.Dict[Any, Any]): ...
        # this line should just work
        repr(C.__mro__)

    def test_dict(self):
        T = TypeVar('T')

        class B(Generic[T]):
            pass

        b = B()
        b.foo = 42
        self.assertEqual(b.__dict__, {'foo': 42})

        class C(B[int]):
            pass

        c = C()
        c.bar = 'abc'
        self.assertEqual(c.__dict__, {'bar': 'abc'})

    def test_subscripted_generics_as_proxies(self):
        T = TypeVar('T')
        class C(Generic[T]):
            x = 'def'
        self.assertEqual(C[int].x, 'def')
        self.assertEqual(C[C[int]].x, 'def')
        C[C[int]].x = 'changed'
        self.assertEqual(C.x, 'changed')
        self.assertEqual(C[str].x, 'changed')
        C[List[str]].z = 'new'
        self.assertEqual(C.z, 'new')
        self.assertEqual(C[Tuple[int]].z, 'new')

        self.assertEqual(C().x, 'changed')
        self.assertEqual(C[Tuple[str]]().z, 'new')

        class D(C[T]):
            pass
        self.assertEqual(D[int].x, 'changed')
        self.assertEqual(D.z, 'new')
        D.z = 'from derived z'
        D[int].x = 'from derived x'
        self.assertEqual(C.x, 'changed')
        self.assertEqual(C[int].z, 'new')
        self.assertEqual(D.x, 'from derived x')
        self.assertEqual(D[str].z, 'from derived z')

    def test_abc_registry_kept(self):
        T = TypeVar('T')
        class C(collections.abc.Mapping, Generic[T]): ...
        C.register(int)
        self.assertIsInstance(1, C)
        C[int]
        self.assertIsInstance(1, C)
        C._abc_registry_clear()
        C._abc_caches_clear()  # To keep refleak hunting mode clean

    def test_false_subclasses(self):
        class MyMapping(MutableMapping[str, str]): pass
        self.assertNotIsInstance({}, MyMapping)
        self.assertNotIsSubclass(dict, MyMapping)

    def test_abc_bases(self):
        class MM(MutableMapping[str, str]):
            def __getitem__(self, k):
                return None
            def __setitem__(self, k, v):
                pass
            def __delitem__(self, k):
                pass
            def __iter__(self):
                return iter(())
            def __len__(self):
                return 0
        # this should just work
        MM().update()
        self.assertIsInstance(MM(), collections.abc.MutableMapping)
        self.assertIsInstance(MM(), MutableMapping)
        self.assertNotIsInstance(MM(), List)
        self.assertNotIsInstance({}, MM)

    def test_multiple_bases(self):
        class MM1(MutableMapping[str, str], collections.abc.MutableMapping):
            pass
        class MM2(collections.abc.MutableMapping, MutableMapping[str, str]):
            pass
        self.assertEqual(MM2.__bases__, (collections.abc.MutableMapping, Generic))

    def test_orig_bases(self):
        T = TypeVar('T')
        class C(typing.Dict[str, T]): ...
        self.assertEqual(C.__orig_bases__, (typing.Dict[str, T],))

    def test_naive_runtime_checks(self):
        def naive_dict_check(obj, tp):
            # Check if a dictionary conforms to Dict type
            if len(tp.__parameters__) > 0:
                raise NotImplementedError
            if tp.__args__:
                KT, VT = tp.__args__
                return all(
                    isinstance(k, KT) and isinstance(v, VT)
                    for k, v in obj.items()
                )
        self.assertTrue(naive_dict_check({'x': 1}, typing.Dict[str, int]))
        self.assertFalse(naive_dict_check({1: 'x'}, typing.Dict[str, int]))
        with self.assertRaises(NotImplementedError):
            naive_dict_check({1: 'x'}, typing.Dict[str, T])

        def naive_generic_check(obj, tp):
            # Check if an instance conforms to the generic class
            if not hasattr(obj, '__orig_class__'):
                raise NotImplementedError
            return obj.__orig_class__ == tp
        class Node(Generic[T]): ...
        self.assertTrue(naive_generic_check(Node[int](), Node[int]))
        self.assertFalse(naive_generic_check(Node[str](), Node[int]))
        self.assertFalse(naive_generic_check(Node[str](), List))
        with self.assertRaises(NotImplementedError):
            naive_generic_check([1, 2, 3], Node[int])

        def naive_list_base_check(obj, tp):
            # Check if list conforms to a List subclass
            return all(isinstance(x, tp.__orig_bases__[0].__args__[0])
                       for x in obj)
        class C(List[int]): ...
        self.assertTrue(naive_list_base_check([1, 2, 3], C))
        self.assertFalse(naive_list_base_check(['a', 'b'], C))

    def test_multi_subscr_base(self):
        T = TypeVar('T')
        U = TypeVar('U')
        V = TypeVar('V')
        class C(List[T][U][V]): ...
        class D(C, List[T][U][V]): ...
        self.assertEqual(C.__parameters__, (V,))
        self.assertEqual(D.__parameters__, (V,))
        self.assertEqual(C[int].__parameters__, ())
        self.assertEqual(D[int].__parameters__, ())
        self.assertEqual(C[int].__args__, (int,))
        self.assertEqual(D[int].__args__, (int,))
        self.assertEqual(C.__bases__, (list, Generic))
        self.assertEqual(D.__bases__, (C, list, Generic))
        self.assertEqual(C.__orig_bases__, (List[T][U][V],))
        self.assertEqual(D.__orig_bases__, (C, List[T][U][V]))

    def test_subscript_meta(self):
        T = TypeVar('T')
        class Meta(type): ...
        self.assertEqual(Type[Meta], Type[Meta])
        self.assertEqual(Union[T, int][Meta], Union[Meta, int])
        self.assertEqual(Callable[..., Meta].__args__, (Ellipsis, Meta))

    def test_generic_hashes(self):
        class A(Generic[T]):
            ...

        class B(Generic[T]):
            class A(Generic[T]):
                ...

        self.assertEqual(A, A)
        self.assertEqual(mod_generics_cache.A[str], mod_generics_cache.A[str])
        self.assertEqual(B.A, B.A)
        self.assertEqual(mod_generics_cache.B.A[B.A[str]],
                         mod_generics_cache.B.A[B.A[str]])

        self.assertNotEqual(A, B.A)
        self.assertNotEqual(A, mod_generics_cache.A)
        self.assertNotEqual(A, mod_generics_cache.B.A)
        self.assertNotEqual(B.A, mod_generics_cache.A)
        self.assertNotEqual(B.A, mod_generics_cache.B.A)

        self.assertNotEqual(A[str], B.A[str])
        self.assertNotEqual(A[List[Any]], B.A[List[Any]])
        self.assertNotEqual(A[str], mod_generics_cache.A[str])
        self.assertNotEqual(A[str], mod_generics_cache.B.A[str])
        self.assertNotEqual(B.A[int], mod_generics_cache.A[int])
        self.assertNotEqual(B.A[List[Any]], mod_generics_cache.B.A[List[Any]])

        self.assertNotEqual(Tuple[A[str]], Tuple[B.A[str]])
        self.assertNotEqual(Tuple[A[List[Any]]], Tuple[B.A[List[Any]]])
        self.assertNotEqual(Union[str, A[str]], Union[str, mod_generics_cache.A[str]])
        self.assertNotEqual(Union[A[str], A[str]],
                            Union[A[str], mod_generics_cache.A[str]])
        self.assertNotEqual(typing.FrozenSet[A[str]],
                            typing.FrozenSet[mod_generics_cache.B.A[str]])

        if sys.version_info[:2] > (3, 2):
            self.assertTrue(repr(Tuple[A[str]]).endswith('<locals>.A[str]]'))
            self.assertTrue(repr(Tuple[B.A[str]]).endswith('<locals>.B.A[str]]'))
            self.assertTrue(repr(Tuple[mod_generics_cache.A[str]])
                            .endswith('mod_generics_cache.A[str]]'))
            self.assertTrue(repr(Tuple[mod_generics_cache.B.A[str]])
                            .endswith('mod_generics_cache.B.A[str]]'))

    def test_extended_generic_rules_eq(self):
        T = TypeVar('T')
        U = TypeVar('U')
        self.assertEqual(Tuple[T, T][int], Tuple[int, int])
        self.assertEqual(typing.Iterable[Tuple[T, T]][T], typing.Iterable[Tuple[T, T]])
        with self.assertRaises(TypeError):
            Tuple[T, int][()]
        with self.assertRaises(TypeError):
            Tuple[T, U][T, ...]

        self.assertEqual(Union[T, int][int], int)
        self.assertEqual(Union[T, U][int, Union[int, str]], Union[int, str])
        class Base: ...
        class Derived(Base): ...
        self.assertEqual(Union[T, Base][Union[Base, Derived]], Union[Base, Derived])
        with self.assertRaises(TypeError):
            Union[T, int][1]

        self.assertEqual(Callable[[T], T][KT], Callable[[KT], KT])
        self.assertEqual(Callable[..., List[T]][int], Callable[..., List[int]])
        with self.assertRaises(TypeError):
            Callable[[T], U][..., int]
        with self.assertRaises(TypeError):
            Callable[[T], U][[], int]

    def test_extended_generic_rules_repr(self):
        T = TypeVar('T')
        self.assertEqual(repr(Union[Tuple, Callable]).replace('typing.', ''),
                         'Union[Tuple, Callable]')
        self.assertEqual(repr(Union[Tuple, Tuple[int]]).replace('typing.', ''),
                         'Union[Tuple, Tuple[int]]')
        self.assertEqual(repr(Callable[..., Optional[T]][int]).replace('typing.', ''),
                         'Callable[..., Union[int, NoneType]]')
        self.assertEqual(repr(Callable[[], List[T]][int]).replace('typing.', ''),
                         'Callable[[], List[int]]')

    def test_generic_forward_ref(self):
        def foobar(x: List[List['CC']]): ...
        class CC: ...
        self.assertEqual(
            get_type_hints(foobar, globals(), locals()),
            {'x': List[List[CC]]}
        )
        T = TypeVar('T')
        AT = Tuple[T, ...]
        def barfoo(x: AT): ...
        self.assertIs(get_type_hints(barfoo, globals(), locals())['x'], AT)
        CT = Callable[..., List[T]]
        def barfoo2(x: CT): ...
        self.assertIs(get_type_hints(barfoo2, globals(), locals())['x'], CT)

    def test_extended_generic_rules_subclassing(self):
        class T1(Tuple[T, KT]): ...
        class T2(Tuple[T, ...]): ...
        class C1(Callable[[T], T]): ...
        class C2(Callable[..., int]):
            def __call__(self):
                return None

        self.assertEqual(T1.__parameters__, (T, KT))
        self.assertEqual(T1[int, str].__args__, (int, str))
        self.assertEqual(T1[int, T].__origin__, T1)

        self.assertEqual(T2.__parameters__, (T,))
        with self.assertRaises(TypeError):
            T1[int]
        with self.assertRaises(TypeError):
            T2[int, str]

        self.assertEqual(repr(C1[int]).split('.')[-1], 'C1[int]')
        self.assertEqual(C2.__parameters__, ())
        self.assertIsInstance(C2(), collections.abc.Callable)
        self.assertIsSubclass(C2, collections.abc.Callable)
        self.assertIsSubclass(C1, collections.abc.Callable)
        self.assertIsInstance(T1(), tuple)
        self.assertIsSubclass(T2, tuple)
        with self.assertRaises(TypeError):
            issubclass(Tuple[int, ...], typing.Sequence)
        with self.assertRaises(TypeError):
            issubclass(Tuple[int, ...], typing.Iterable)

    def test_fail_with_bare_union(self):
        with self.assertRaises(TypeError):
            List[Union]
        with self.assertRaises(TypeError):
            Tuple[Optional]
        with self.assertRaises(TypeError):
            ClassVar[ClassVar]
        with self.assertRaises(TypeError):
            List[ClassVar[int]]

    def test_fail_with_bare_generic(self):
        T = TypeVar('T')
        with self.assertRaises(TypeError):
            List[Generic]
        with self.assertRaises(TypeError):
            Tuple[Generic[T]]
        with self.assertRaises(TypeError):
            List[typing._Protocol]

    def test_type_erasure_special(self):
        T = TypeVar('T')
        # this is the only test that checks type caching
        self.clear_caches()
        class MyTup(Tuple[T, T]): ...
        self.assertIs(MyTup[int]().__class__, MyTup)
        self.assertIs(MyTup[int]().__orig_class__, MyTup[int])
        class MyCall(Callable[..., T]):
            def __call__(self): return None
        self.assertIs(MyCall[T]().__class__, MyCall)
        self.assertIs(MyCall[T]().__orig_class__, MyCall[T])
        class MyDict(typing.Dict[T, T]): ...
        self.assertIs(MyDict[int]().__class__, MyDict)
        self.assertIs(MyDict[int]().__orig_class__, MyDict[int])
        class MyDef(typing.DefaultDict[str, T]): ...
        self.assertIs(MyDef[int]().__class__, MyDef)
        self.assertIs(MyDef[int]().__orig_class__, MyDef[int])
        # ChainMap was added in 3.3
        if sys.version_info >= (3, 3):
            class MyChain(typing.ChainMap[str, T]): ...
            self.assertIs(MyChain[int]().__class__, MyChain)
            self.assertIs(MyChain[int]().__orig_class__, MyChain[int])

    def test_all_repr_eq_any(self):
        objs = (getattr(typing, el) for el in typing.__all__)
        for obj in objs:
            self.assertNotEqual(repr(obj), '')
            self.assertEqual(obj, obj)
            if getattr(obj, '__parameters__', None) and len(obj.__parameters__) == 1:
                self.assertEqual(obj[Any].__args__, (Any,))
            if isinstance(obj, type):
                for base in obj.__mro__:
                    self.assertNotEqual(repr(base), '')
                    self.assertEqual(base, base)

    def test_pickle(self):
        global C  # pickle wants to reference the class by name
        T = TypeVar('T')

        class B(Generic[T]):
            pass

        class C(B[int]):
            pass

        c = C()
        c.foo = 42
        c.bar = 'abc'
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            z = pickle.dumps(c, proto)
            x = pickle.loads(z)
            self.assertEqual(x.foo, 42)
            self.assertEqual(x.bar, 'abc')
            self.assertEqual(x.__dict__, {'foo': 42, 'bar': 'abc'})
        samples = [Any, Union, Tuple, Callable, ClassVar,
                   Union[int, str], ClassVar[List], Tuple[int, ...], Callable[[str], bytes],
                   typing.DefaultDict, typing.FrozenSet[int]]
        for s in samples:
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                z = pickle.dumps(s, proto)
                x = pickle.loads(z)
                self.assertEqual(s, x)
        more_samples = [List, typing.Iterable, typing.Type, List[int],
                        typing.Type[typing.Mapping], typing.AbstractSet[Tuple[int, str]]]
        for s in more_samples:
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                z = pickle.dumps(s, proto)
                x = pickle.loads(z)
                self.assertEqual(s, x)

    def test_copy_and_deepcopy(self):
        T = TypeVar('T')
        class Node(Generic[T]): ...
        things = [Union[T, int], Tuple[T, int], Callable[..., T], Callable[[int], int],
                  Tuple[Any, Any], Node[T], Node[int], Node[Any], typing.Iterable[T],
                  typing.Iterable[Any], typing.Iterable[int], typing.Dict[int, str],
                  typing.Dict[T, Any], ClassVar[int], ClassVar[List[T]], Tuple['T', 'T'],
                  Union['T', int], List['T'], typing.Mapping['T', int]]
        for t in things + [Any]:
            self.assertEqual(t, copy(t))
            self.assertEqual(t, deepcopy(t))

    def test_immutability_by_copy_and_pickle(self):
        # Special forms like Union, Any, etc., generic aliases to containers like List,
        # Mapping, etc., and type variabcles are considered immutable by copy and pickle.
        global TP, TPB, TPV  # for pickle
        TP = TypeVar('TP')
        TPB = TypeVar('TPB', bound=int)
        TPV = TypeVar('TPV', bytes, str)
        for X in [TP, TPB, TPV, List, typing.Mapping, ClassVar, typing.Iterable,
                  Union, Any, Tuple, Callable]:
            self.assertIs(copy(X), X)
            self.assertIs(deepcopy(X), X)
            self.assertIs(pickle.loads(pickle.dumps(X)), X)
        # Check that local type variables are copyable.
        TL = TypeVar('TL')
        TLB = TypeVar('TLB', bound=int)
        TLV = TypeVar('TLV', bytes, str)
        for X in [TL, TLB, TLV]:
            self.assertIs(copy(X), X)
            self.assertIs(deepcopy(X), X)

    def test_copy_generic_instances(self):
        T = TypeVar('T')
        class C(Generic[T]):
            def __init__(self, attr: T) -> None:
                self.attr = attr

        c = C(42)
        self.assertEqual(copy(c).attr, 42)
        self.assertEqual(deepcopy(c).attr, 42)
        self.assertIsNot(copy(c), c)
        self.assertIsNot(deepcopy(c), c)
        c.attr = 1
        self.assertEqual(copy(c).attr, 1)
        self.assertEqual(deepcopy(c).attr, 1)
        ci = C[int](42)
        self.assertEqual(copy(ci).attr, 42)
        self.assertEqual(deepcopy(ci).attr, 42)
        self.assertIsNot(copy(ci), ci)
        self.assertIsNot(deepcopy(ci), ci)
        ci.attr = 1
        self.assertEqual(copy(ci).attr, 1)
        self.assertEqual(deepcopy(ci).attr, 1)
        self.assertEqual(ci.__orig_class__, C[int])

    def test_weakref_all(self):
        T = TypeVar('T')
        things = [Any, Union[T, int], Callable[..., T], Tuple[Any, Any],
                  Optional[List[int]], typing.Mapping[int, str],
                  typing.re.Match[bytes], typing.Iterable['whatever']]
        for t in things:
            self.assertEqual(weakref.ref(t)(), t)

    def test_parameterized_slots(self):
        T = TypeVar('T')
        class C(Generic[T]):
            __slots__ = ('potato',)

        c = C()
        c_int = C[int]()

        c.potato = 0
        c_int.potato = 0
        with self.assertRaises(AttributeError):
            c.tomato = 0
        with self.assertRaises(AttributeError):
            c_int.tomato = 0

        def foo(x: C['C']): ...
        self.assertEqual(get_type_hints(foo, globals(), locals())['x'], C[C])
        self.assertEqual(copy(C[int]), deepcopy(C[int]))

    def test_parameterized_slots_dict(self):
        T = TypeVar('T')
        class D(Generic[T]):
            __slots__ = {'banana': 42}

        d = D()
        d_int = D[int]()

        d.banana = 'yes'
        d_int.banana = 'yes'
        with self.assertRaises(AttributeError):
            d.foobar = 'no'
        with self.assertRaises(AttributeError):
            d_int.foobar = 'no'

    def test_errors(self):
        with self.assertRaises(TypeError):
            B = SimpleMapping[XK, Any]

            class C(Generic[B]):
                pass

    def test_repr_2(self):
        class C(Generic[T]):
            pass

        self.assertEqual(C.__module__, __name__)
        self.assertEqual(C.__qualname__,
                         'GenericTests.test_repr_2.<locals>.C')
        X = C[int]
        self.assertEqual(X.__module__, __name__)
        self.assertEqual(repr(X).split('.')[-1], 'C[int]')

        class Y(C[int]):
            pass

        self.assertEqual(Y.__module__, __name__)
        self.assertEqual(Y.__qualname__,
                         'GenericTests.test_repr_2.<locals>.Y')

    def test_eq_1(self):
        self.assertEqual(Generic, Generic)
        self.assertEqual(Generic[T], Generic[T])
        self.assertNotEqual(Generic[KT], Generic[VT])

    def test_eq_2(self):

        class A(Generic[T]):
            pass

        class B(Generic[T]):
            pass

        self.assertEqual(A, A)
        self.assertNotEqual(A, B)
        self.assertEqual(A[T], A[T])
        self.assertNotEqual(A[T], B[T])

    def test_multiple_inheritance(self):

        class A(Generic[T, VT]):
            pass

        class B(Generic[KT, T]):
            pass

        class C(A[T, VT], Generic[VT, T, KT], B[KT, T]):
            pass

        self.assertEqual(C.__parameters__, (VT, T, KT))

    def test_multiple_inheritance_special(self):
        S = TypeVar('S')
        class B(Generic[S]): ...
        class C(List[int], B): ...
        self.assertEqual(C.__mro__, (C, list, B, Generic, object))

    def test_init_subclass_super_called(self):
        class FinalException(Exception):
            pass

        class Final:
            def __init_subclass__(cls, **kwargs) -> None:
                for base in cls.__bases__:
                    if base is not Final and issubclass(base, Final):
                        raise FinalException(base)
                super().__init_subclass__(**kwargs)
        class Test(Generic[T], Final):
            pass
        with self.assertRaises(FinalException):
            class Subclass(Test):
                pass
        with self.assertRaises(FinalException):
            class Subclass(Test[int]):
                pass

    def test_nested(self):

        G = Generic

        class Visitor(G[T]):

            a = None

            def set(self, a: T):
                self.a = a

            def get(self):
                return self.a

            def visit(self) -> T:
                return self.a

        V = Visitor[typing.List[int]]

        class IntListVisitor(V):

            def append(self, x: int):
                self.a.append(x)

        a = IntListVisitor()
        a.set([])
        a.append(1)
        a.append(42)
        self.assertEqual(a.get(), [1, 42])

    def test_type_erasure(self):
        T = TypeVar('T')

        class Node(Generic[T]):
            def __init__(self, label: T,
                         left: 'Node[T]' = None,
                         right: 'Node[T]' = None):
                self.label = label  # type: T
                self.left = left  # type: Optional[Node[T]]
                self.right = right  # type: Optional[Node[T]]

        def foo(x: T):
            a = Node(x)
            b = Node[T](x)
            c = Node[Any](x)
            self.assertIs(type(a), Node)
            self.assertIs(type(b), Node)
            self.assertIs(type(c), Node)
            self.assertEqual(a.label, x)
            self.assertEqual(b.label, x)
            self.assertEqual(c.label, x)

        foo(42)

    def test_implicit_any(self):
        T = TypeVar('T')

        class C(Generic[T]):
            pass

        class D(C):
            pass

        self.assertEqual(D.__parameters__, ())

        with self.assertRaises(Exception):
            D[int]
        with self.assertRaises(Exception):
            D[Any]
        with self.assertRaises(Exception):
            D[T]

    def test_new_with_args(self):

        class A(Generic[T]):
            pass

        class B:
            def __new__(cls, arg):
                # call object
                obj = super().__new__(cls)
                obj.arg = arg
                return obj

        # mro: C, A, Generic, B, object
        class C(A, B):
            pass

        c = C('foo')
        self.assertEqual(c.arg, 'foo')

    def test_new_with_args2(self):

        class A:
            def __init__(self, arg):
                self.from_a = arg
                # call object
                super().__init__()

        # mro: C, Generic, A, object
        class C(Generic[T], A):
            def __init__(self, arg):
                self.from_c = arg
                # call Generic
                super().__init__(arg)

        c = C('foo')
        self.assertEqual(c.from_a, 'foo')
        self.assertEqual(c.from_c, 'foo')

    def test_new_no_args(self):

        class A(Generic[T]):
            pass

        with self.assertRaises(TypeError):
            A('foo')

        class B:
            def __new__(cls):
                # call object
                obj = super().__new__(cls)
                obj.from_b = 'b'
                return obj

        # mro: C, A, Generic, B, object
        class C(A, B):
            def __init__(self, arg):
                self.arg = arg

            def __new__(cls, arg):
                # call A
                obj = super().__new__(cls)
                obj.from_c = 'c'
                return obj

        c = C('foo')
        self.assertEqual(c.arg, 'foo')
        self.assertEqual(c.from_b, 'b')
        self.assertEqual(c.from_c, 'c')


class ClassVarTests(BaseTestCase):

    def test_basics(self):
        with self.assertRaises(TypeError):
            ClassVar[1]
        with self.assertRaises(TypeError):
            ClassVar[int, str]
        with self.assertRaises(TypeError):
            ClassVar[int][str]

    def test_repr(self):
        self.assertEqual(repr(ClassVar), 'typing.ClassVar')
        cv = ClassVar[int]
        self.assertEqual(repr(cv), 'typing.ClassVar[int]')
        cv = ClassVar[Employee]
        self.assertEqual(repr(cv), 'typing.ClassVar[%s.Employee]' % __name__)

    def test_cannot_subclass(self):
        with self.assertRaises(TypeError):
            class C(type(ClassVar)):
                pass
        with self.assertRaises(TypeError):
            class C(type(ClassVar[int])):
                pass

    def test_cannot_init(self):
        with self.assertRaises(TypeError):
            ClassVar()
        with self.assertRaises(TypeError):
            type(ClassVar)()
        with self.assertRaises(TypeError):
            type(ClassVar[Optional[int]])()

    def test_no_isinstance(self):
        with self.assertRaises(TypeError):
            isinstance(1, ClassVar[int])
        with self.assertRaises(TypeError):
            issubclass(int, ClassVar)


class CastTests(BaseTestCase):

    def test_basics(self):
        self.assertEqual(cast(int, 42), 42)
        self.assertEqual(cast(float, 42), 42)
        self.assertIs(type(cast(float, 42)), int)
        self.assertEqual(cast(Any, 42), 42)
        self.assertEqual(cast(list, 42), 42)
        self.assertEqual(cast(Union[str, float], 42), 42)
        self.assertEqual(cast(AnyStr, 42), 42)
        self.assertEqual(cast(None, 42), 42)

    def test_errors(self):
        # Bogus calls are not expected to fail.
        cast(42, 42)
        cast('hello', 42)


class ForwardRefTests(BaseTestCase):

    def test_basics(self):

        class Node(Generic[T]):

            def __init__(self, label: T):
                self.label = label
                self.left = self.right = None

            def add_both(self,
                         left: 'Optional[Node[T]]',
                         right: 'Node[T]' = None,
                         stuff: int = None,
                         blah=None):
                self.left = left
                self.right = right

            def add_left(self, node: Optional['Node[T]']):
                self.add_both(node, None)

            def add_right(self, node: 'Node[T]' = None):
                self.add_both(None, node)

        t = Node[int]
        both_hints = get_type_hints(t.add_both, globals(), locals())
        self.assertEqual(both_hints['left'], Optional[Node[T]])
        self.assertEqual(both_hints['right'], Optional[Node[T]])
        self.assertEqual(both_hints['left'], both_hints['right'])
        self.assertEqual(both_hints['stuff'], Optional[int])
        self.assertNotIn('blah', both_hints)

        left_hints = get_type_hints(t.add_left, globals(), locals())
        self.assertEqual(left_hints['node'], Optional[Node[T]])

        right_hints = get_type_hints(t.add_right, globals(), locals())
        self.assertEqual(right_hints['node'], Optional[Node[T]])

    def test_forwardref_instance_type_error(self):
        fr = typing.ForwardRef('int')
        with self.assertRaises(TypeError):
            isinstance(42, fr)

    def test_forwardref_subclass_type_error(self):
        fr = typing.ForwardRef('int')
        with self.assertRaises(TypeError):
            issubclass(int, fr)

    def test_forward_equality(self):
        fr = typing.ForwardRef('int')
        self.assertEqual(fr, typing.ForwardRef('int'))
        self.assertNotEqual(List['int'], List[int])

    def test_forward_repr(self):
        self.assertEqual(repr(List['int']), "typing.List[ForwardRef('int')]")

    def test_union_forward(self):

        def foo(a: Union['T']):
            pass

        self.assertEqual(get_type_hints(foo, globals(), locals()),
                         {'a': Union[T]})

    def test_tuple_forward(self):

        def foo(a: Tuple['T']):
            pass

        self.assertEqual(get_type_hints(foo, globals(), locals()),
                         {'a': Tuple[T]})

    def test_callable_forward(self):

        def foo(a: Callable[['T'], 'T']):
            pass

        self.assertEqual(get_type_hints(foo, globals(), locals()),
                         {'a': Callable[[T], T]})

    def test_callable_with_ellipsis_forward(self):

        def foo(a: 'Callable[..., T]'):
            pass

        self.assertEqual(get_type_hints(foo, globals(), locals()),
                         {'a': Callable[..., T]})

    def test_syntax_error(self):

        with self.assertRaises(SyntaxError):
            Generic['/T']

    def test_delayed_syntax_error(self):

        def foo(a: 'Node[T'):
            pass

        with self.assertRaises(SyntaxError):
            get_type_hints(foo)

    def test_type_error(self):

        def foo(a: Tuple['42']):
            pass

        with self.assertRaises(TypeError):
            get_type_hints(foo)

    def test_name_error(self):

        def foo(a: 'Noode[T]'):
            pass

        with self.assertRaises(NameError):
            get_type_hints(foo, locals())

    def test_no_type_check(self):

        @no_type_check
        def foo(a: 'whatevers') -> {}:
            pass

        th = get_type_hints(foo)
        self.assertEqual(th, {})

    def test_no_type_check_class(self):

        @no_type_check
        class C:
            def foo(a: 'whatevers') -> {}:
                pass

        cth = get_type_hints(C.foo)
        self.assertEqual(cth, {})
        ith = get_type_hints(C().foo)
        self.assertEqual(ith, {})

    def test_no_type_check_no_bases(self):
        class C:
            def meth(self, x: int): ...
        @no_type_check
        class D(C):
            c = C
        # verify that @no_type_check never affects bases
        self.assertEqual(get_type_hints(C.meth), {'x': int})

    def test_no_type_check_forward_ref_as_string(self):
        class C:
            foo: typing.ClassVar[int] = 7
        class D:
            foo: ClassVar[int] = 7
        class E:
            foo: 'typing.ClassVar[int]' = 7
        class F:
            foo: 'ClassVar[int]' = 7

        expected_result = {'foo': typing.ClassVar[int]}
        for clazz in [C, D, E, F]:
            self.assertEqual(get_type_hints(clazz), expected_result)

    def test_nested_classvar_fails_forward_ref_check(self):
        class E:
            foo: 'typing.ClassVar[typing.ClassVar[int]]' = 7
        class F:
            foo: ClassVar['ClassVar[int]'] = 7

        for clazz in [E, F]:
            with self.assertRaises(TypeError):
                get_type_hints(clazz)

    def test_meta_no_type_check(self):

        @no_type_check_decorator
        def magic_decorator(func):
            return func

        self.assertEqual(magic_decorator.__name__, 'magic_decorator')

        @magic_decorator
        def foo(a: 'whatevers') -> {}:
            pass

        @magic_decorator
        class C:
            def foo(a: 'whatevers') -> {}:
                pass

        self.assertEqual(foo.__name__, 'foo')
        th = get_type_hints(foo)
        self.assertEqual(th, {})
        cth = get_type_hints(C.foo)
        self.assertEqual(cth, {})
        ith = get_type_hints(C().foo)
        self.assertEqual(ith, {})

    def test_default_globals(self):
        code = ("class C:\n"
                "    def foo(self, a: 'C') -> 'D': pass\n"
                "class D:\n"
                "    def bar(self, b: 'D') -> C: pass\n"
                )
        ns = {}
        exec(code, ns)
        hints = get_type_hints(ns['C'].foo)
        self.assertEqual(hints, {'a': ns['C'], 'return': ns['D']})


class OverloadTests(BaseTestCase):

    def test_overload_fails(self):
        from typing import overload

        with self.assertRaises(RuntimeError):

            @overload
            def blah():
                pass

            blah()

    def test_overload_succeeds(self):
        from typing import overload

        @overload
        def blah():
            pass

        def blah():
            pass

        blah()


ASYNCIO_TESTS = """
import asyncio

T_a = TypeVar('T_a')

class AwaitableWrapper(typing.Awaitable[T_a]):

    def __init__(self, value):
        self.value = value

    def __await__(self) -> typing.Iterator[T_a]:
        yield
        return self.value

class AsyncIteratorWrapper(typing.AsyncIterator[T_a]):

    def __init__(self, value: typing.Iterable[T_a]):
        self.value = value

    def __aiter__(self) -> typing.AsyncIterator[T_a]:
        return self

    @asyncio.coroutine
    def __anext__(self) -> T_a:
        data = yield from self.value
        if data:
            return data
        else:
            raise StopAsyncIteration

class ACM:
    async def __aenter__(self) -> int:
        return 42
    async def __aexit__(self, etype, eval, tb):
        return None
"""

try:
    exec(ASYNCIO_TESTS)
except ImportError:
    ASYNCIO = False  # multithreading is not enabled
else:
    ASYNCIO = True

# Definitions needed for features introduced in Python 3.6

from test import ann_module, ann_module2, ann_module3
from typing import AsyncContextManager

class A:
    y: float
class B(A):
    x: ClassVar[Optional['B']] = None
    y: int
    b: int
class CSub(B):
    z: ClassVar['CSub'] = B()
class G(Generic[T]):
    lst: ClassVar[List[T]] = []

class NoneAndForward:
    parent: 'NoneAndForward'
    meaning: None

class CoolEmployee(NamedTuple):
    name: str
    cool: int

class CoolEmployeeWithDefault(NamedTuple):
    name: str
    cool: int = 0

class XMeth(NamedTuple):
    x: int
    def double(self):
        return 2 * self.x

class XRepr(NamedTuple):
    x: int
    y: int = 1
    def __str__(self):
        return f'{self.x} -> {self.y}'
    def __add__(self, other):
        return 0

class HasForeignBaseClass(mod_generics_cache.A):
    some_xrepr: 'XRepr'
    other_a: 'mod_generics_cache.A'

async def g_with(am: AsyncContextManager[int]):
    x: int
    async with am as x:
        return x

try:
    g_with(ACM()).send(None)
except StopIteration as e:
    assert e.args[0] == 42

gth = get_type_hints

class ForRefExample:
    @ann_module.dec
    def func(self: 'ForRefExample'):
        pass

    @ann_module.dec
    @ann_module.dec
    def nested(self: 'ForRefExample'):
        pass


class GetTypeHintTests(BaseTestCase):
    def test_get_type_hints_from_various_objects(self):
        # For invalid objects should fail with TypeError (not AttributeError etc).
        with self.assertRaises(TypeError):
            gth(123)
        with self.assertRaises(TypeError):
            gth('abc')
        with self.assertRaises(TypeError):
            gth(None)

    def test_get_type_hints_modules(self):
        ann_module_type_hints = {1: 2, 'f': Tuple[int, int], 'x': int, 'y': str}
        self.assertEqual(gth(ann_module), ann_module_type_hints)
        self.assertEqual(gth(ann_module2), {})
        self.assertEqual(gth(ann_module3), {})

    @skip("known bug")
    def test_get_type_hints_modules_forwardref(self):
        # FIXME: This currently exposes a bug in typing. Cached forward references
        # don't account for the case where there are multiple types of the same
        # name coming from different modules in the same program.
        mgc_hints = {'default_a': Optional[mod_generics_cache.A],
                     'default_b': Optional[mod_generics_cache.B]}
        self.assertEqual(gth(mod_generics_cache), mgc_hints)

    def test_get_type_hints_classes(self):
        self.assertEqual(gth(ann_module.C),  # gth will find the right globalns
                         {'y': Optional[ann_module.C]})
        self.assertIsInstance(gth(ann_module.j_class), dict)
        self.assertEqual(gth(ann_module.M), {'123': 123, 'o': type})
        self.assertEqual(gth(ann_module.D),
                         {'j': str, 'k': str, 'y': Optional[ann_module.C]})
        self.assertEqual(gth(ann_module.Y), {'z': int})
        self.assertEqual(gth(ann_module.h_class),
                         {'y': Optional[ann_module.C]})
        self.assertEqual(gth(ann_module.S), {'x': str, 'y': str})
        self.assertEqual(gth(ann_module.foo), {'x': int})
        self.assertEqual(gth(NoneAndForward),
                         {'parent': NoneAndForward, 'meaning': type(None)})
        self.assertEqual(gth(HasForeignBaseClass),
                         {'some_xrepr': XRepr, 'other_a': mod_generics_cache.A,
                          'some_b': mod_generics_cache.B})
        self.assertEqual(gth(XRepr.__new__),
                         {'x': int, 'y': int})
        self.assertEqual(gth(mod_generics_cache.B),
                         {'my_inner_a1': mod_generics_cache.B.A,
                          'my_inner_a2': mod_generics_cache.B.A,
                          'my_outer_a': mod_generics_cache.A})

    def test_respect_no_type_check(self):
        @no_type_check
        class NoTpCheck:
            class Inn:
                def __init__(self, x: 'not a type'): ...
        self.assertTrue(NoTpCheck.__no_type_check__)
        self.assertTrue(NoTpCheck.Inn.__init__.__no_type_check__)
        self.assertEqual(gth(ann_module2.NTC.meth), {})
        class ABase(Generic[T]):
            def meth(x: int): ...
        @no_type_check
        class Der(ABase): ...
        self.assertEqual(gth(ABase.meth), {'x': int})

    def test_get_type_hints_for_builtins(self):
        # Should not fail for built-in classes and functions.
        self.assertEqual(gth(int), {})
        self.assertEqual(gth(type), {})
        self.assertEqual(gth(dir), {})
        self.assertEqual(gth(len), {})
        self.assertEqual(gth(object.__str__), {})
        self.assertEqual(gth(object().__str__), {})
        self.assertEqual(gth(str.join), {})

    def test_previous_behavior(self):
        def testf(x, y): ...
        testf.__annotations__['x'] = 'int'
        self.assertEqual(gth(testf), {'x': int})
        def testg(x: None): ...
        self.assertEqual(gth(testg), {'x': type(None)})

    def test_get_type_hints_for_object_with_annotations(self):
        class A: ...
        class B: ...
        b = B()
        b.__annotations__ = {'x': 'A'}
        self.assertEqual(gth(b, locals()), {'x': A})

    def test_get_type_hints_ClassVar(self):
        self.assertEqual(gth(ann_module2.CV, ann_module2.__dict__),
                         {'var': typing.ClassVar[ann_module2.CV]})
        self.assertEqual(gth(B, globals()),
                         {'y': int, 'x': ClassVar[Optional[B]], 'b': int})
        self.assertEqual(gth(CSub, globals()),
                         {'z': ClassVar[CSub], 'y': int, 'b': int,
                          'x': ClassVar[Optional[B]]})
        self.assertEqual(gth(G), {'lst': ClassVar[List[T]]})

    def test_get_type_hints_wrapped_decoratored_func(self):
        expects = {'self': ForRefExample}
        self.assertEqual(gth(ForRefExample.func), expects)
        self.assertEqual(gth(ForRefExample.nested), expects)


class CollectionsAbcTests(BaseTestCase):

    def test_hashable(self):
        self.assertIsInstance(42, typing.Hashable)
        self.assertNotIsInstance([], typing.Hashable)

    def test_iterable(self):
        self.assertIsInstance([], typing.Iterable)
        # Due to ABC caching, the second time takes a separate code
        # path and could fail.  So call this a few times.
        self.assertIsInstance([], typing.Iterable)
        self.assertIsInstance([], typing.Iterable)
        self.assertNotIsInstance(42, typing.Iterable)
        # Just in case, also test issubclass() a few times.
        self.assertIsSubclass(list, typing.Iterable)
        self.assertIsSubclass(list, typing.Iterable)

    def test_iterator(self):
        it = iter([])
        self.assertIsInstance(it, typing.Iterator)
        self.assertNotIsInstance(42, typing.Iterator)

    @skipUnless(ASYNCIO, 'Python 3.5 and multithreading required')
    def test_awaitable(self):
        ns = {}
        exec(
            "async def foo() -> typing.Awaitable[int]:\n"
            "    return await AwaitableWrapper(42)\n",
            globals(), ns)
        foo = ns['foo']
        g = foo()
        self.assertIsInstance(g, typing.Awaitable)
        self.assertNotIsInstance(foo, typing.Awaitable)
        g.send(None)  # Run foo() till completion, to avoid warning.

    @skipUnless(ASYNCIO, 'Python 3.5 and multithreading required')
    def test_coroutine(self):
        ns = {}
        exec(
            "async def foo():\n"
            "    return\n",
            globals(), ns)
        foo = ns['foo']
        g = foo()
        self.assertIsInstance(g, typing.Coroutine)
        with self.assertRaises(TypeError):
            isinstance(g, typing.Coroutine[int])
        self.assertNotIsInstance(foo, typing.Coroutine)
        try:
            g.send(None)
        except StopIteration:
            pass

    @skipUnless(ASYNCIO, 'Python 3.5 and multithreading required')
    def test_async_iterable(self):
        base_it = range(10)  # type: Iterator[int]
        it = AsyncIteratorWrapper(base_it)
        self.assertIsInstance(it, typing.AsyncIterable)
        self.assertIsInstance(it, typing.AsyncIterable)
        self.assertNotIsInstance(42, typing.AsyncIterable)

    @skipUnless(ASYNCIO, 'Python 3.5 and multithreading required')
    def test_async_iterator(self):
        base_it = range(10)  # type: Iterator[int]
        it = AsyncIteratorWrapper(base_it)
        self.assertIsInstance(it, typing.AsyncIterator)
        self.assertNotIsInstance(42, typing.AsyncIterator)

    def test_sized(self):
        self.assertIsInstance([], typing.Sized)
        self.assertNotIsInstance(42, typing.Sized)

    def test_container(self):
        self.assertIsInstance([], typing.Container)
        self.assertNotIsInstance(42, typing.Container)

    def test_collection(self):
        if hasattr(typing, 'Collection'):
            self.assertIsInstance(tuple(), typing.Collection)
            self.assertIsInstance(frozenset(), typing.Collection)
            self.assertIsSubclass(dict, typing.Collection)
            self.assertNotIsInstance(42, typing.Collection)

    def test_abstractset(self):
        self.assertIsInstance(set(), typing.AbstractSet)
        self.assertNotIsInstance(42, typing.AbstractSet)

    def test_mutableset(self):
        self.assertIsInstance(set(), typing.MutableSet)
        self.assertNotIsInstance(frozenset(), typing.MutableSet)

    def test_mapping(self):
        self.assertIsInstance({}, typing.Mapping)
        self.assertNotIsInstance(42, typing.Mapping)

    def test_mutablemapping(self):
        self.assertIsInstance({}, typing.MutableMapping)
        self.assertNotIsInstance(42, typing.MutableMapping)

    def test_sequence(self):
        self.assertIsInstance([], typing.Sequence)
        self.assertNotIsInstance(42, typing.Sequence)

    def test_mutablesequence(self):
        self.assertIsInstance([], typing.MutableSequence)
        self.assertNotIsInstance((), typing.MutableSequence)

    def test_bytestring(self):
        self.assertIsInstance(b'', typing.ByteString)
        self.assertIsInstance(bytearray(b''), typing.ByteString)

    def test_list(self):
        self.assertIsSubclass(list, typing.List)

    def test_deque(self):
        self.assertIsSubclass(collections.deque, typing.Deque)
        class MyDeque(typing.Deque[int]): ...
        self.assertIsInstance(MyDeque(), collections.deque)

    def test_counter(self):
        self.assertIsSubclass(collections.Counter, typing.Counter)

    def test_set(self):
        self.assertIsSubclass(set, typing.Set)
        self.assertNotIsSubclass(frozenset, typing.Set)

    def test_frozenset(self):
        self.assertIsSubclass(frozenset, typing.FrozenSet)
        self.assertNotIsSubclass(set, typing.FrozenSet)

    def test_dict(self):
        self.assertIsSubclass(dict, typing.Dict)

    def test_no_list_instantiation(self):
        with self.assertRaises(TypeError):
            typing.List()
        with self.assertRaises(TypeError):
            typing.List[T]()
        with self.assertRaises(TypeError):
            typing.List[int]()

    def test_list_subclass(self):

        class MyList(typing.List[int]):
            pass

        a = MyList()
        self.assertIsInstance(a, MyList)
        self.assertIsInstance(a, typing.Sequence)

        self.assertIsSubclass(MyList, list)
        self.assertNotIsSubclass(list, MyList)

    def test_no_dict_instantiation(self):
        with self.assertRaises(TypeError):
            typing.Dict()
        with self.assertRaises(TypeError):
            typing.Dict[KT, VT]()
        with self.assertRaises(TypeError):
            typing.Dict[str, int]()

    def test_dict_subclass(self):

        class MyDict(typing.Dict[str, int]):
            pass

        d = MyDict()
        self.assertIsInstance(d, MyDict)
        self.assertIsInstance(d, typing.MutableMapping)

        self.assertIsSubclass(MyDict, dict)
        self.assertNotIsSubclass(dict, MyDict)

    def test_defaultdict_instantiation(self):
        self.assertIs(type(typing.DefaultDict()), collections.defaultdict)
        self.assertIs(type(typing.DefaultDict[KT, VT]()), collections.defaultdict)
        self.assertIs(type(typing.DefaultDict[str, int]()), collections.defaultdict)

    def test_defaultdict_subclass(self):

        class MyDefDict(typing.DefaultDict[str, int]):
            pass

        dd = MyDefDict()
        self.assertIsInstance(dd, MyDefDict)

        self.assertIsSubclass(MyDefDict, collections.defaultdict)
        self.assertNotIsSubclass(collections.defaultdict, MyDefDict)

    def test_ordereddict_instantiation(self):
        self.assertIs(type(typing.OrderedDict()), collections.OrderedDict)
        self.assertIs(type(typing.OrderedDict[KT, VT]()), collections.OrderedDict)
        self.assertIs(type(typing.OrderedDict[str, int]()), collections.OrderedDict)

    def test_ordereddict_subclass(self):

        class MyOrdDict(typing.OrderedDict[str, int]):
            pass

        od = MyOrdDict()
        self.assertIsInstance(od, MyOrdDict)

        self.assertIsSubclass(MyOrdDict, collections.OrderedDict)
        self.assertNotIsSubclass(collections.OrderedDict, MyOrdDict)

    @skipUnless(sys.version_info >= (3, 3), 'ChainMap was added in 3.3')
    def test_chainmap_instantiation(self):
        self.assertIs(type(typing.ChainMap()), collections.ChainMap)
        self.assertIs(type(typing.ChainMap[KT, VT]()), collections.ChainMap)
        self.assertIs(type(typing.ChainMap[str, int]()), collections.ChainMap)
        class CM(typing.ChainMap[KT, VT]): ...
        self.assertIs(type(CM[int, str]()), CM)

    @skipUnless(sys.version_info >= (3, 3), 'ChainMap was added in 3.3')
    def test_chainmap_subclass(self):

        class MyChainMap(typing.ChainMap[str, int]):
            pass

        cm = MyChainMap()
        self.assertIsInstance(cm, MyChainMap)

        self.assertIsSubclass(MyChainMap, collections.ChainMap)
        self.assertNotIsSubclass(collections.ChainMap, MyChainMap)

    def test_deque_instantiation(self):
        self.assertIs(type(typing.Deque()), collections.deque)
        self.assertIs(type(typing.Deque[T]()), collections.deque)
        self.assertIs(type(typing.Deque[int]()), collections.deque)
        class D(typing.Deque[T]): ...
        self.assertIs(type(D[int]()), D)

    def test_counter_instantiation(self):
        self.assertIs(type(typing.Counter()), collections.Counter)
        self.assertIs(type(typing.Counter[T]()), collections.Counter)
        self.assertIs(type(typing.Counter[int]()), collections.Counter)
        class C(typing.Counter[T]): ...
        self.assertIs(type(C[int]()), C)

    def test_counter_subclass_instantiation(self):

        class MyCounter(typing.Counter[int]):
            pass

        d = MyCounter()
        self.assertIsInstance(d, MyCounter)
        self.assertIsInstance(d, typing.Counter)
        self.assertIsInstance(d, collections.Counter)

    def test_no_set_instantiation(self):
        with self.assertRaises(TypeError):
            typing.Set()
        with self.assertRaises(TypeError):
            typing.Set[T]()
        with self.assertRaises(TypeError):
            typing.Set[int]()

    def test_set_subclass_instantiation(self):

        class MySet(typing.Set[int]):
            pass

        d = MySet()
        self.assertIsInstance(d, MySet)

    def test_no_frozenset_instantiation(self):
        with self.assertRaises(TypeError):
            typing.FrozenSet()
        with self.assertRaises(TypeError):
            typing.FrozenSet[T]()
        with self.assertRaises(TypeError):
            typing.FrozenSet[int]()

    def test_frozenset_subclass_instantiation(self):

        class MyFrozenSet(typing.FrozenSet[int]):
            pass

        d = MyFrozenSet()
        self.assertIsInstance(d, MyFrozenSet)

    def test_no_tuple_instantiation(self):
        with self.assertRaises(TypeError):
            Tuple()
        with self.assertRaises(TypeError):
            Tuple[T]()
        with self.assertRaises(TypeError):
            Tuple[int]()

    def test_generator(self):
        def foo():
            yield 42
        g = foo()
        self.assertIsSubclass(type(g), typing.Generator)

    def test_no_generator_instantiation(self):
        with self.assertRaises(TypeError):
            typing.Generator()
        with self.assertRaises(TypeError):
            typing.Generator[T, T, T]()
        with self.assertRaises(TypeError):
            typing.Generator[int, int, int]()

    def test_async_generator(self):
        ns = {}
        exec("async def f():\n"
             "    yield 42\n", globals(), ns)
        g = ns['f']()
        self.assertIsSubclass(type(g), typing.AsyncGenerator)

    def test_no_async_generator_instantiation(self):
        with self.assertRaises(TypeError):
            typing.AsyncGenerator()
        with self.assertRaises(TypeError):
            typing.AsyncGenerator[T, T]()
        with self.assertRaises(TypeError):
            typing.AsyncGenerator[int, int]()

    def test_subclassing(self):

        class MMA(typing.MutableMapping):
            pass

        with self.assertRaises(TypeError):  # It's abstract
            MMA()

        class MMC(MMA):
            def __getitem__(self, k):
                return None
            def __setitem__(self, k, v):
                pass
            def __delitem__(self, k):
                pass
            def __iter__(self):
                return iter(())
            def __len__(self):
                return 0

        self.assertEqual(len(MMC()), 0)
        assert callable(MMC.update)
        self.assertIsInstance(MMC(), typing.Mapping)

        class MMB(typing.MutableMapping[KT, VT]):
            def __getitem__(self, k):
                return None
            def __setitem__(self, k, v):
                pass
            def __delitem__(self, k):
                pass
            def __iter__(self):
                return iter(())
            def __len__(self):
                return 0

        self.assertEqual(len(MMB()), 0)
        self.assertEqual(len(MMB[str, str]()), 0)
        self.assertEqual(len(MMB[KT, VT]()), 0)

        self.assertNotIsSubclass(dict, MMA)
        self.assertNotIsSubclass(dict, MMB)

        self.assertIsSubclass(MMA, typing.Mapping)
        self.assertIsSubclass(MMB, typing.Mapping)
        self.assertIsSubclass(MMC, typing.Mapping)

        self.assertIsInstance(MMB[KT, VT](), typing.Mapping)
        self.assertIsInstance(MMB[KT, VT](), collections.abc.Mapping)

        self.assertIsSubclass(MMA, collections.abc.Mapping)
        self.assertIsSubclass(MMB, collections.abc.Mapping)
        self.assertIsSubclass(MMC, collections.abc.Mapping)

        with self.assertRaises(TypeError):
            issubclass(MMB[str, str], typing.Mapping)
        self.assertIsSubclass(MMC, MMA)

        class I(typing.Iterable): ...
        self.assertNotIsSubclass(list, I)

        class G(typing.Generator[int, int, int]): ...
        def g(): yield 0
        self.assertIsSubclass(G, typing.Generator)
        self.assertIsSubclass(G, typing.Iterable)
        self.assertIsSubclass(G, collections.abc.Generator)
        self.assertIsSubclass(G, collections.abc.Iterable)
        self.assertNotIsSubclass(type(g), G)

    def test_subclassing_async_generator(self):
        class G(typing.AsyncGenerator[int, int]):
            def asend(self, value):
                pass
            def athrow(self, typ, val=None, tb=None):
                pass

        ns = {}
        exec('async def g(): yield 0', globals(), ns)
        g = ns['g']
        self.assertIsSubclass(G, typing.AsyncGenerator)
        self.assertIsSubclass(G, typing.AsyncIterable)
        self.assertIsSubclass(G, collections.abc.AsyncGenerator)
        self.assertIsSubclass(G, collections.abc.AsyncIterable)
        self.assertNotIsSubclass(type(g), G)

        instance = G()
        self.assertIsInstance(instance, typing.AsyncGenerator)
        self.assertIsInstance(instance, typing.AsyncIterable)
        self.assertIsInstance(instance, collections.abc.AsyncGenerator)
        self.assertIsInstance(instance, collections.abc.AsyncIterable)
        self.assertNotIsInstance(type(g), G)
        self.assertNotIsInstance(g, G)

    def test_subclassing_subclasshook(self):

        class Base(typing.Iterable):
            @classmethod
            def __subclasshook__(cls, other):
                if other.__name__ == 'Foo':
                    return True
                else:
                    return False

        class C(Base): ...
        class Foo: ...
        class Bar: ...
        self.assertIsSubclass(Foo, Base)
        self.assertIsSubclass(Foo, C)
        self.assertNotIsSubclass(Bar, C)

    def test_subclassing_register(self):

        class A(typing.Container): ...
        class B(A): ...

        class C: ...
        A.register(C)
        self.assertIsSubclass(C, A)
        self.assertNotIsSubclass(C, B)

        class D: ...
        B.register(D)
        self.assertIsSubclass(D, A)
        self.assertIsSubclass(D, B)

        class M(): ...
        collections.abc.MutableMapping.register(M)
        self.assertIsSubclass(M, typing.Mapping)

    def test_collections_as_base(self):

        class M(collections.abc.Mapping): ...
        self.assertIsSubclass(M, typing.Mapping)
        self.assertIsSubclass(M, typing.Iterable)

        class S(collections.abc.MutableSequence): ...
        self.assertIsSubclass(S, typing.MutableSequence)
        self.assertIsSubclass(S, typing.Iterable)

        class I(collections.abc.Iterable): ...
        self.assertIsSubclass(I, typing.Iterable)

        class A(collections.abc.Mapping, metaclass=abc.ABCMeta): ...
        class B: ...
        A.register(B)
        self.assertIsSubclass(B, typing.Mapping)


class OtherABCTests(BaseTestCase):

    def test_contextmanager(self):
        @contextlib.contextmanager
        def manager():
            yield 42

        cm = manager()
        self.assertIsInstance(cm, typing.ContextManager)
        self.assertNotIsInstance(42, typing.ContextManager)

    @skipUnless(ASYNCIO, 'Python 3.5 required')
    def test_async_contextmanager(self):
        class NotACM:
            pass
        self.assertIsInstance(ACM(), typing.AsyncContextManager)
        self.assertNotIsInstance(NotACM(), typing.AsyncContextManager)
        @contextlib.contextmanager
        def manager():
            yield 42

        cm = manager()
        self.assertNotIsInstance(cm, typing.AsyncContextManager)
        self.assertEqual(typing.AsyncContextManager[int].__args__, (int,))
        with self.assertRaises(TypeError):
            isinstance(42, typing.AsyncContextManager[int])
        with self.assertRaises(TypeError):
            typing.AsyncContextManager[int, str]


class TypeTests(BaseTestCase):

    def test_type_basic(self):

        class User: pass
        class BasicUser(User): pass
        class ProUser(User): pass

        def new_user(user_class: Type[User]) -> User:
            return user_class()

        new_user(BasicUser)

    def test_type_typevar(self):

        class User: pass
        class BasicUser(User): pass
        class ProUser(User): pass

        U = TypeVar('U', bound=User)

        def new_user(user_class: Type[U]) -> U:
            return user_class()

        new_user(BasicUser)

    def test_type_optional(self):
        A = Optional[Type[BaseException]]

        def foo(a: A) -> Optional[BaseException]:
            if a is None:
                return None
            else:
                return a()

        assert isinstance(foo(KeyboardInterrupt), KeyboardInterrupt)
        assert foo(None) is None


class NewTypeTests(BaseTestCase):

    def test_basic(self):
        UserId = NewType('UserId', int)
        UserName = NewType('UserName', str)
        self.assertIsInstance(UserId(5), int)
        self.assertIsInstance(UserName('Joe'), str)
        self.assertEqual(UserId(5) + 1, 6)

    def test_errors(self):
        UserId = NewType('UserId', int)
        UserName = NewType('UserName', str)
        with self.assertRaises(TypeError):
            issubclass(UserId, int)
        with self.assertRaises(TypeError):
            class D(UserName):
                pass


class NamedTupleTests(BaseTestCase):
    class NestedEmployee(NamedTuple):
        name: str
        cool: int

    def test_basics(self):
        Emp = NamedTuple('Emp', [('name', str), ('id', int)])
        self.assertIsSubclass(Emp, tuple)
        joe = Emp('Joe', 42)
        jim = Emp(name='Jim', id=1)
        self.assertIsInstance(joe, Emp)
        self.assertIsInstance(joe, tuple)
        self.assertEqual(joe.name, 'Joe')
        self.assertEqual(joe.id, 42)
        self.assertEqual(jim.name, 'Jim')
        self.assertEqual(jim.id, 1)
        self.assertEqual(Emp.__name__, 'Emp')
        self.assertEqual(Emp._fields, ('name', 'id'))
        self.assertEqual(Emp.__annotations__,
                         collections.OrderedDict([('name', str), ('id', int)]))
        self.assertIs(Emp._field_types, Emp.__annotations__)

    def test_namedtuple_pyversion(self):
        if sys.version_info[:2] < (3, 6):
            with self.assertRaises(TypeError):
                NamedTuple('Name', one=int, other=str)
            with self.assertRaises(TypeError):
                class NotYet(NamedTuple):
                    whatever = 0

    def test_annotation_usage(self):
        tim = CoolEmployee('Tim', 9000)
        self.assertIsInstance(tim, CoolEmployee)
        self.assertIsInstance(tim, tuple)
        self.assertEqual(tim.name, 'Tim')
        self.assertEqual(tim.cool, 9000)
        self.assertEqual(CoolEmployee.__name__, 'CoolEmployee')
        self.assertEqual(CoolEmployee._fields, ('name', 'cool'))
        self.assertEqual(CoolEmployee.__annotations__,
                         collections.OrderedDict(name=str, cool=int))
        self.assertIs(CoolEmployee._field_types, CoolEmployee.__annotations__)

    def test_annotation_usage_with_default(self):
        jelle = CoolEmployeeWithDefault('Jelle')
        self.assertIsInstance(jelle, CoolEmployeeWithDefault)
        self.assertIsInstance(jelle, tuple)
        self.assertEqual(jelle.name, 'Jelle')
        self.assertEqual(jelle.cool, 0)
        cooler_employee = CoolEmployeeWithDefault('Sjoerd', 1)
        self.assertEqual(cooler_employee.cool, 1)

        self.assertEqual(CoolEmployeeWithDefault.__name__, 'CoolEmployeeWithDefault')
        self.assertEqual(CoolEmployeeWithDefault._fields, ('name', 'cool'))
        self.assertEqual(CoolEmployeeWithDefault._field_types, dict(name=str, cool=int))
        self.assertEqual(CoolEmployeeWithDefault._field_defaults, dict(cool=0))

        with self.assertRaises(TypeError):
            exec("""
class NonDefaultAfterDefault(NamedTuple):
    x: int = 3
    y: int
""")

    def test_annotation_usage_with_methods(self):
        self.assertEqual(XMeth(1).double(), 2)
        self.assertEqual(XMeth(42).x, XMeth(42)[0])
        self.assertEqual(str(XRepr(42)), '42 -> 1')
        self.assertEqual(XRepr(1, 2) + XRepr(3), 0)

        with self.assertRaises(AttributeError):
            exec("""
class XMethBad(NamedTuple):
    x: int
    def _fields(self):
        return 'no chance for this'
""")

        with self.assertRaises(AttributeError):
            exec("""
class XMethBad2(NamedTuple):
    x: int
    def _source(self):
        return 'no chance for this as well'
""")

    def test_namedtuple_keyword_usage(self):
        LocalEmployee = NamedTuple("LocalEmployee", name=str, age=int)
        nick = LocalEmployee('Nick', 25)
        self.assertIsInstance(nick, tuple)
        self.assertEqual(nick.name, 'Nick')
        self.assertEqual(LocalEmployee.__name__, 'LocalEmployee')
        self.assertEqual(LocalEmployee._fields, ('name', 'age'))
        self.assertEqual(LocalEmployee.__annotations__, dict(name=str, age=int))
        self.assertIs(LocalEmployee._field_types, LocalEmployee.__annotations__)
        with self.assertRaises(TypeError):
            NamedTuple('Name', [('x', int)], y=str)
        with self.assertRaises(TypeError):
            NamedTuple('Name', x=1, y='a')

    def test_namedtuple_special_keyword_names(self):
        NT = NamedTuple("NT", cls=type, self=object, typename=str, fields=list)
        self.assertEqual(NT.__name__, 'NT')
        self.assertEqual(NT._fields, ('cls', 'self', 'typename', 'fields'))
        a = NT(cls=str, self=42, typename='foo', fields=[('bar', tuple)])
        self.assertEqual(a.cls, str)
        self.assertEqual(a.self, 42)
        self.assertEqual(a.typename, 'foo')
        self.assertEqual(a.fields, [('bar', tuple)])

    def test_namedtuple_errors(self):
        with self.assertRaises(TypeError):
            NamedTuple.__new__()
        with self.assertRaises(TypeError):
            NamedTuple()
        with self.assertRaises(TypeError):
            NamedTuple('Emp', [('name', str)], None)
        with self.assertRaises(ValueError):
            NamedTuple('Emp', [('_name', str)])

        Emp = NamedTuple(typename='Emp', name=str, id=int)
        self.assertEqual(Emp.__name__, 'Emp')
        self.assertEqual(Emp._fields, ('name', 'id'))

        Emp = NamedTuple('Emp', fields=[('name', str), ('id', int)])
        self.assertEqual(Emp.__name__, 'Emp')
        self.assertEqual(Emp._fields, ('name', 'id'))

    def test_copy_and_pickle(self):
        global Emp  # pickle wants to reference the class by name
        Emp = NamedTuple('Emp', [('name', str), ('cool', int)])
        for cls in Emp, CoolEmployee, self.NestedEmployee:
            with self.subTest(cls=cls):
                jane = cls('jane', 37)
                for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                    z = pickle.dumps(jane, proto)
                    jane2 = pickle.loads(z)
                    self.assertEqual(jane2, jane)
                    self.assertIsInstance(jane2, cls)

                jane2 = copy(jane)
                self.assertEqual(jane2, jane)
                self.assertIsInstance(jane2, cls)

                jane2 = deepcopy(jane)
                self.assertEqual(jane2, jane)
                self.assertIsInstance(jane2, cls)


class IOTests(BaseTestCase):

    def test_io(self):

        def stuff(a: IO) -> AnyStr:
            return a.readline()

        a = stuff.__annotations__['a']
        self.assertEqual(a.__parameters__, (AnyStr,))

    def test_textio(self):

        def stuff(a: TextIO) -> str:
            return a.readline()

        a = stuff.__annotations__['a']
        self.assertEqual(a.__parameters__, ())

    def test_binaryio(self):

        def stuff(a: BinaryIO) -> bytes:
            return a.readline()

        a = stuff.__annotations__['a']
        self.assertEqual(a.__parameters__, ())

    def test_io_submodule(self):
        from typing.io import IO, TextIO, BinaryIO, __all__, __name__
        self.assertIs(IO, typing.IO)
        self.assertIs(TextIO, typing.TextIO)
        self.assertIs(BinaryIO, typing.BinaryIO)
        self.assertEqual(set(__all__), set(['IO', 'TextIO', 'BinaryIO']))
        self.assertEqual(__name__, 'typing.io')


class RETests(BaseTestCase):
    # Much of this is really testing _TypeAlias.

    def test_basics(self):
        pat = re.compile('[a-z]+', re.I)
        self.assertIsSubclass(pat.__class__, Pattern)
        self.assertIsSubclass(type(pat), Pattern)
        self.assertIsInstance(pat, Pattern)

        mat = pat.search('12345abcde.....')
        self.assertIsSubclass(mat.__class__, Match)
        self.assertIsSubclass(type(mat), Match)
        self.assertIsInstance(mat, Match)

        # these should just work
        Pattern[Union[str, bytes]]
        Match[Union[bytes, str]]

    def test_alias_equality(self):
        self.assertEqual(Pattern[str], Pattern[str])
        self.assertNotEqual(Pattern[str], Pattern[bytes])
        self.assertNotEqual(Pattern[str], Match[str])
        self.assertNotEqual(Pattern[str], str)

    def test_errors(self):
        m = Match[Union[str, bytes]]
        with self.assertRaises(TypeError):
            m[str]
        with self.assertRaises(TypeError):
            # We don't support isinstance().
            isinstance(42, Pattern[str])
        with self.assertRaises(TypeError):
            # We don't support issubclass().
            issubclass(Pattern[bytes], Pattern[str])

    def test_repr(self):
        self.assertEqual(repr(Pattern), 'typing.Pattern')
        self.assertEqual(repr(Pattern[str]), 'typing.Pattern[str]')
        self.assertEqual(repr(Pattern[bytes]), 'typing.Pattern[bytes]')
        self.assertEqual(repr(Match), 'typing.Match')
        self.assertEqual(repr(Match[str]), 'typing.Match[str]')
        self.assertEqual(repr(Match[bytes]), 'typing.Match[bytes]')

    def test_re_submodule(self):
        from typing.re import Match, Pattern, __all__, __name__
        self.assertIs(Match, typing.Match)
        self.assertIs(Pattern, typing.Pattern)
        self.assertEqual(set(__all__), set(['Match', 'Pattern']))
        self.assertEqual(__name__, 'typing.re')

    def test_cannot_subclass(self):
        with self.assertRaises(TypeError) as ex:

            class A(typing.Match):
                pass

        self.assertEqual(str(ex.exception),
                         "type 're.Match' is not an acceptable base type")


class AllTests(BaseTestCase):
    """Tests for __all__."""

    def test_all(self):
        from typing import __all__ as a
        # Just spot-check the first and last of every category.
        self.assertIn('AbstractSet', a)
        self.assertIn('ValuesView', a)
        self.assertIn('cast', a)
        self.assertIn('overload', a)
        if hasattr(contextlib, 'AbstractContextManager'):
            self.assertIn('ContextManager', a)
        # Check that io and re are not exported.
        self.assertNotIn('io', a)
        self.assertNotIn('re', a)
        # Spot-check that stdlib modules aren't exported.
        self.assertNotIn('os', a)
        self.assertNotIn('sys', a)
        # Check that Text is defined.
        self.assertIn('Text', a)
        # Check previously missing classes.
        self.assertIn('SupportsBytes', a)
        self.assertIn('SupportsComplex', a)

    def test_all_exported_names(self):
        import typing

        actual_all = set(typing.__all__)
        computed_all = {
            k for k, v in vars(typing).items()
            # explicitly exported, not a thing with __module__
            if k in actual_all or (
                # avoid private names
                not k.startswith('_') and
                # avoid things in the io / re typing submodules
                k not in typing.io.__all__ and
                k not in typing.re.__all__ and
                k not in {'io', 're'} and
                # there's a few types and metaclasses that aren't exported
                not k.endswith(('Meta', '_contra', '_co')) and
                not k.upper() == k and
                # but export all things that have __module__ == 'typing'
                getattr(v, '__module__', None) == typing.__name__
            )
        }
        self.assertSetEqual(computed_all, actual_all)



if __name__ == '__main__':
    main()
