import contextlib
import collections
import pickle
import re
import sys
from unittest import TestCase, main, skipUnless, SkipTest

from typing import Any
from typing import TypeVar, AnyStr
from typing import T, KT, VT  # Not in __all__.
from typing import Union, Optional
from typing import Tuple
from typing import Callable
from typing import Generic
from typing import cast
from typing import get_type_hints
from typing import no_type_check, no_type_check_decorator
from typing import Type
from typing import NewType
from typing import NamedTuple
from typing import IO, TextIO, BinaryIO
from typing import Pattern, Match
import typing


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

    def test_any_subclass(self):
        self.assertTrue(issubclass(Employee, Any))
        self.assertTrue(issubclass(int, Any))
        self.assertTrue(issubclass(type(None), Any))
        self.assertTrue(issubclass(object, Any))

    def test_others_any(self):
        self.assertFalse(issubclass(Any, Employee))
        self.assertFalse(issubclass(Any, int))
        self.assertFalse(issubclass(Any, type(None)))
        # However, Any is a subclass of object (this can't be helped).
        self.assertTrue(issubclass(Any, object))

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

    def test_cannot_instantiate(self):
        with self.assertRaises(TypeError):
            Any()

    def test_cannot_subscript(self):
        with self.assertRaises(TypeError):
            Any[int]

    def test_any_is_subclass(self):
        # Any should be considered a subclass of everything.
        self.assertIsSubclass(Any, Any)
        self.assertIsSubclass(Any, typing.List)
        self.assertIsSubclass(Any, typing.List[int])
        self.assertIsSubclass(Any, typing.List[T])
        self.assertIsSubclass(Any, typing.Mapping)
        self.assertIsSubclass(Any, typing.Mapping[str, int])
        self.assertIsSubclass(Any, typing.Mapping[KT, VT])
        self.assertIsSubclass(Any, Generic)
        self.assertIsSubclass(Any, Generic[T])
        self.assertIsSubclass(Any, Generic[KT, VT])
        self.assertIsSubclass(Any, AnyStr)
        self.assertIsSubclass(Any, Union)
        self.assertIsSubclass(Any, Union[int, str])
        self.assertIsSubclass(Any, typing.Match)
        self.assertIsSubclass(Any, typing.Match[str])
        # These expressions must simply not fail.
        typing.Match[Any]
        typing.Pattern[Any]
        typing.IO[Any]


class TypeVarTests(BaseTestCase):

    def test_basic_plain(self):
        T = TypeVar('T')
        # Every class is a subclass of T.
        self.assertIsSubclass(int, T)
        self.assertIsSubclass(str, T)
        # T equals itself.
        self.assertEqual(T, T)
        # T is a subclass of itself.
        self.assertIsSubclass(T, T)
        # T is an instance of TypeVar
        self.assertIsInstance(T, TypeVar)

    def test_typevar_instance_type_error(self):
        T = TypeVar('T')
        with self.assertRaises(TypeError):
            isinstance(42, T)

    def test_basic_constrained(self):
        A = TypeVar('A', str, bytes)
        # Only str and bytes are subclasses of A.
        self.assertIsSubclass(str, A)
        self.assertIsSubclass(bytes, A)
        self.assertNotIsSubclass(int, A)
        # A equals itself.
        self.assertEqual(A, A)
        # A is a subclass of itself.
        self.assertIsSubclass(A, A)

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
        self.assertEqual(Union[X, int].__union_params__, (X, int))
        self.assertEqual(Union[X, int].__union_set_params__, {X, int})

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

    def test_subclass_as_unions(self):
        # None of these are true -- each type var is its own world.
        self.assertFalse(issubclass(TypeVar('T', int, str),
                                    TypeVar('T', int, str)))
        self.assertFalse(issubclass(TypeVar('T', int, float),
                                    TypeVar('T', int, float, str)))
        self.assertFalse(issubclass(TypeVar('T', int, str),
                                    TypeVar('T', str, int)))
        A = TypeVar('A', int, str)
        B = TypeVar('B', int, str, float)
        self.assertFalse(issubclass(A, B))
        self.assertFalse(issubclass(B, A))

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

    def test_bound(self):
        X = TypeVar('X', bound=Employee)
        self.assertIsSubclass(Employee, X)
        self.assertIsSubclass(Manager, X)
        self.assertNotIsSubclass(int, X)

    def test_bound_errors(self):
        with self.assertRaises(TypeError):
            TypeVar('X', bound=42)
        with self.assertRaises(TypeError):
            TypeVar('X', str, float, bound=Employee)


class UnionTests(BaseTestCase):

    def test_basics(self):
        u = Union[int, float]
        self.assertNotEqual(u, Union)
        self.assertTrue(issubclass(int, u))
        self.assertTrue(issubclass(float, u))

    def test_union_any(self):
        u = Union[Any]
        self.assertEqual(u, Any)
        u = Union[int, Any]
        self.assertEqual(u, Any)
        u = Union[Any, int]
        self.assertEqual(u, Any)

    def test_union_object(self):
        u = Union[object]
        self.assertEqual(u, object)
        u = Union[int, object]
        self.assertEqual(u, object)
        u = Union[object, int]
        self.assertEqual(u, object)

    def test_union_any_object(self):
        u = Union[object, Any]
        self.assertEqual(u, Any)
        u = Union[Any, object]
        self.assertEqual(u, Any)

    def test_unordered(self):
        u1 = Union[int, float]
        u2 = Union[float, int]
        self.assertEqual(u1, u2)

    def test_subclass(self):
        u = Union[int, Employee]
        self.assertTrue(issubclass(Manager, u))

    def test_self_subclass(self):
        self.assertTrue(issubclass(Union[KT, VT], Union))
        self.assertFalse(issubclass(Union, Union[KT, VT]))

    def test_multiple_inheritance(self):
        u = Union[int, Employee]
        self.assertTrue(issubclass(ManagingFounder, u))

    def test_single_class_disappears(self):
        t = Union[Employee]
        self.assertIs(t, Employee)

    def test_base_class_disappears(self):
        u = Union[Employee, Manager, int]
        self.assertEqual(u, Union[int, Employee])
        u = Union[Manager, int, Employee]
        self.assertEqual(u, Union[int, Employee])
        u = Union[Employee, Manager]
        self.assertIs(u, Employee)

    def test_weird_subclasses(self):
        u = Union[Employee, int, float]
        v = Union[int, float]
        self.assertTrue(issubclass(v, u))
        w = Union[int, Manager]
        self.assertTrue(issubclass(w, u))

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

    def test_cannot_subclass(self):
        with self.assertRaises(TypeError):
            class C(Union):
                pass
        with self.assertRaises(TypeError):
            class C(Union[int, str]):
                pass

    def test_cannot_instantiate(self):
        with self.assertRaises(TypeError):
            Union()
        u = Union[int, float]
        with self.assertRaises(TypeError):
            u()

    def test_optional(self):
        o = Optional[int]
        u = Union[int, None]
        self.assertEqual(o, u)

    def test_empty(self):
        with self.assertRaises(TypeError):
            Union[()]

    def test_issubclass_union(self):
        self.assertIsSubclass(Union[int, str], Union)
        self.assertNotIsSubclass(int, Union)

    def test_union_instance_type_error(self):
        with self.assertRaises(TypeError):
            isinstance(42, Union[int, str])

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


class TypeVarUnionTests(BaseTestCase):

    def test_simpler(self):
        A = TypeVar('A', int, str, float)
        B = TypeVar('B', int, str)
        self.assertIsSubclass(A, A)
        self.assertIsSubclass(B, B)
        self.assertNotIsSubclass(B, A)
        self.assertIsSubclass(A, Union[int, str, float])
        self.assertNotIsSubclass(Union[int, str, float], A)
        self.assertNotIsSubclass(Union[int, str], B)
        self.assertIsSubclass(B, Union[int, str])
        self.assertNotIsSubclass(A, B)
        self.assertNotIsSubclass(Union[int, str, float], B)
        self.assertNotIsSubclass(A, Union[int, str])

    def test_var_union_subclass(self):
        self.assertTrue(issubclass(T, Union[int, T]))
        self.assertTrue(issubclass(KT, Union[KT, VT]))

    def test_var_union(self):
        TU = TypeVar('TU', Union[int, float], None)
        self.assertIsSubclass(int, TU)
        self.assertIsSubclass(float, TU)


class TupleTests(BaseTestCase):

    def test_basics(self):
        self.assertTrue(issubclass(Tuple[int, str], Tuple))
        self.assertTrue(issubclass(Tuple[int, str], Tuple[int, str]))
        self.assertFalse(issubclass(int, Tuple))
        self.assertFalse(issubclass(Tuple[float, str], Tuple[int, str]))
        self.assertFalse(issubclass(Tuple[int, str, int], Tuple[int, str]))
        self.assertFalse(issubclass(Tuple[int, str], Tuple[int, str, int]))
        self.assertTrue(issubclass(tuple, Tuple))
        self.assertFalse(issubclass(Tuple, tuple))  # Can't have it both ways.

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
        with self.assertRaises(TypeError):
            isinstance((0, 0), Tuple)

    def test_tuple_ellipsis_subclass(self):

        class B:
            pass

        class C(B):
            pass

        self.assertNotIsSubclass(Tuple[B], Tuple[B, ...])
        self.assertIsSubclass(Tuple[C, ...], Tuple[B, ...])
        self.assertNotIsSubclass(Tuple[C, ...], Tuple[B])
        self.assertNotIsSubclass(Tuple[C], Tuple[B, ...])

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
        self.assertTrue(issubclass(Callable[[int], int], Callable))
        self.assertFalse(issubclass(Callable, Callable[[int], int]))
        self.assertTrue(issubclass(Callable[[int], int], Callable[[int], int]))
        self.assertFalse(issubclass(Callable[[Employee], int],
                                    Callable[[Manager], int]))
        self.assertFalse(issubclass(Callable[[Manager], int],
                                    Callable[[Employee], int]))
        self.assertFalse(issubclass(Callable[[int], Employee],
                                    Callable[[int], Manager]))
        self.assertFalse(issubclass(Callable[[int], Manager],
                                    Callable[[int], Employee]))

    def test_eq_hash(self):
        self.assertEqual(Callable[[int], int], Callable[[int], int])
        self.assertEqual(len({Callable[[int], int], Callable[[int], int]}), 1)
        self.assertNotEqual(Callable[[int], int], Callable[[int], str])
        self.assertNotEqual(Callable[[int], int], Callable[[str], int])
        self.assertNotEqual(Callable[[int], int], Callable[[int, int], int])
        self.assertNotEqual(Callable[[int], int], Callable[[], int])
        self.assertNotEqual(Callable[[int], int], Callable)

    def test_cannot_subclass(self):
        with self.assertRaises(TypeError):

            class C(Callable):
                pass

        with self.assertRaises(TypeError):

            class C(Callable[[int], int]):
                pass

    def test_cannot_instantiate(self):
        with self.assertRaises(TypeError):
            Callable()
        c = Callable[[int], str]
        with self.assertRaises(TypeError):
            c()

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

    def test_init(self):
        T = TypeVar('T')
        S = TypeVar('S')
        with self.assertRaises(TypeError):
            Generic[T, T]
        with self.assertRaises(TypeError):
            Generic[T, S, T]

    def test_repr(self):
        self.assertEqual(repr(SimpleMapping),
                         __name__ + '.' + 'SimpleMapping<~XK, ~XV>')
        self.assertEqual(repr(MySimpleMapping),
                         __name__ + '.' + 'MySimpleMapping<~XK, ~XV>')

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
            '.C<~T>[typing.Tuple[~S, ~T]]<~S, ~T>[~T, int]<~T>[str]'))

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

    def test_errors(self):
        with self.assertRaises(TypeError):
            B = SimpleMapping[XK, Any]

            class C(Generic[B]):
                pass

    def test_repr_2(self):
        PY32 = sys.version_info[:2] < (3, 3)

        class C(Generic[T]):
            pass

        self.assertEqual(C.__module__, __name__)
        if not PY32:
            self.assertEqual(C.__qualname__,
                             'GenericTests.test_repr_2.<locals>.C')
        self.assertEqual(repr(C).split('.')[-1], 'C<~T>')
        X = C[int]
        self.assertEqual(X.__module__, __name__)
        if not PY32:
            self.assertEqual(X.__qualname__, 'C')
        self.assertEqual(repr(X).split('.')[-1], 'C<~T>[int]')

        class Y(C[int]):
            pass

        self.assertEqual(Y.__module__, __name__)
        if not PY32:
            self.assertEqual(Y.__qualname__,
                             'GenericTests.test_repr_2.<locals>.Y')
        self.assertEqual(repr(Y).split('.')[-1], 'Y')

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


class VarianceTests(BaseTestCase):

    def test_invariance(self):
        # Because of invariance, List[subclass of X] is not a subclass
        # of List[X], and ditto for MutableSequence.
        self.assertNotIsSubclass(typing.List[Manager], typing.List[Employee])
        self.assertNotIsSubclass(typing.MutableSequence[Manager],
                              typing.MutableSequence[Employee])
        # It's still reflexive.
        self.assertIsSubclass(typing.List[Employee], typing.List[Employee])
        self.assertIsSubclass(typing.MutableSequence[Employee],
                          typing.MutableSequence[Employee])

    def test_covariance_tuple(self):
        # Check covariace for Tuple (which are really special cases).
        self.assertIsSubclass(Tuple[Manager], Tuple[Employee])
        self.assertNotIsSubclass(Tuple[Employee], Tuple[Manager])
        # And pairwise.
        self.assertIsSubclass(Tuple[Manager, Manager],
                              Tuple[Employee, Employee])
        self.assertNotIsSubclass(Tuple[Employee, Employee],
                              Tuple[Manager, Employee])
        # And using ellipsis.
        self.assertIsSubclass(Tuple[Manager, ...], Tuple[Employee, ...])
        self.assertNotIsSubclass(Tuple[Employee, ...], Tuple[Manager, ...])

    def test_covariance_sequence(self):
        # Check covariance for Sequence (which is just a generic class
        # for this purpose, but using a covariant type variable).
        self.assertIsSubclass(typing.Sequence[Manager],
                              typing.Sequence[Employee])
        self.assertNotIsSubclass(typing.Sequence[Employee],
                              typing.Sequence[Manager])

    def test_covariance_mapping(self):
        # Ditto for Mapping (covariant in the value, invariant in the key).
        self.assertIsSubclass(typing.Mapping[Employee, Manager],
                          typing.Mapping[Employee, Employee])
        self.assertNotIsSubclass(typing.Mapping[Manager, Employee],
                              typing.Mapping[Employee, Employee])
        self.assertNotIsSubclass(typing.Mapping[Employee, Manager],
                              typing.Mapping[Manager, Manager])
        self.assertNotIsSubclass(typing.Mapping[Manager, Employee],
                              typing.Mapping[Manager, Manager])


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
        fr = typing._ForwardRef('int')
        with self.assertRaises(TypeError):
            isinstance(42, fr)

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

    def test_meta_no_type_check(self):

        @no_type_check_decorator
        def magic_decorator(deco):
            return deco

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

    def test_overload_exists(self):
        from typing import overload

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


PY35 = sys.version_info[:2] >= (3, 5)

PY35_TESTS = """
import asyncio

T_a = TypeVar('T')

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
"""

if PY35:
    exec(PY35_TESTS)


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
        self.assertIsInstance([], typing.Iterable[int])
        self.assertNotIsInstance(42, typing.Iterable)
        # Just in case, also test issubclass() a few times.
        self.assertIsSubclass(list, typing.Iterable)
        self.assertIsSubclass(list, typing.Iterable)

    def test_iterator(self):
        it = iter([])
        self.assertIsInstance(it, typing.Iterator)
        self.assertIsInstance(it, typing.Iterator[int])
        self.assertNotIsInstance(42, typing.Iterator)

    @skipUnless(PY35, 'Python 3.5 required')
    def test_awaitable(self):
        ns = {}
        exec(
            "async def foo() -> typing.Awaitable[int]:\n"
            "    return await AwaitableWrapper(42)\n",
            globals(), ns)
        foo = ns['foo']
        g = foo()
        self.assertIsSubclass(type(g), typing.Awaitable[int])
        self.assertIsInstance(g, typing.Awaitable)
        self.assertNotIsInstance(foo, typing.Awaitable)
        self.assertIsSubclass(typing.Awaitable[Manager],
                          typing.Awaitable[Employee])
        self.assertNotIsSubclass(typing.Awaitable[Employee],
                              typing.Awaitable[Manager])
        g.send(None)  # Run foo() till completion, to avoid warning.

    @skipUnless(PY35, 'Python 3.5 required')
    def test_async_iterable(self):
        base_it = range(10)  # type: Iterator[int]
        it = AsyncIteratorWrapper(base_it)
        self.assertIsInstance(it, typing.AsyncIterable)
        self.assertIsInstance(it, typing.AsyncIterable)
        self.assertIsSubclass(typing.AsyncIterable[Manager],
                          typing.AsyncIterable[Employee])
        self.assertNotIsInstance(42, typing.AsyncIterable)

    @skipUnless(PY35, 'Python 3.5 required')
    def test_async_iterator(self):
        base_it = range(10)  # type: Iterator[int]
        it = AsyncIteratorWrapper(base_it)
        self.assertIsInstance(it, typing.AsyncIterator)
        self.assertIsSubclass(typing.AsyncIterator[Manager],
                          typing.AsyncIterator[Employee])
        self.assertNotIsInstance(42, typing.AsyncIterator)

    def test_sized(self):
        self.assertIsInstance([], typing.Sized)
        self.assertNotIsInstance(42, typing.Sized)

    def test_container(self):
        self.assertIsInstance([], typing.Container)
        self.assertNotIsInstance(42, typing.Container)

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

    def test_no_defaultdict_instantiation(self):
        with self.assertRaises(TypeError):
            typing.DefaultDict()
        with self.assertRaises(TypeError):
            typing.DefaultDict[KT, VT]()
        with self.assertRaises(TypeError):
            typing.DefaultDict[str, int]()

    def test_defaultdict_subclass(self):

        class MyDefDict(typing.DefaultDict[str, int]):
            pass

        dd = MyDefDict()
        self.assertIsInstance(dd, MyDefDict)

        self.assertIsSubclass(MyDefDict, collections.defaultdict)
        self.assertNotIsSubclass(collections.defaultdict, MyDefDict)

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
        self.assertIsSubclass(typing.Generator[Manager, Employee, Manager],
                          typing.Generator[Employee, Manager, Employee])
        self.assertNotIsSubclass(typing.Generator[Manager, Manager, Manager],
                              typing.Generator[Employee, Employee, Employee])

    def test_no_generator_instantiation(self):
        with self.assertRaises(TypeError):
            typing.Generator()
        with self.assertRaises(TypeError):
            typing.Generator[T, T, T]()
        with self.assertRaises(TypeError):
            typing.Generator[int, int, int]()

    def test_subclassing(self):

        class MMA(typing.MutableMapping):
            pass

        with self.assertRaises(TypeError):  # It's abstract
            MMA()

        class MMC(MMA):
            def __len__(self):
                return 0

        self.assertEqual(len(MMC()), 0)

        class MMB(typing.MutableMapping[KT, VT]):
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


class OtherABCTests(BaseTestCase):

    @skipUnless(hasattr(typing, 'ContextManager'),
                'requires typing.ContextManager')
    def test_contextmanager(self):
        @contextlib.contextmanager
        def manager():
            yield 42

        cm = manager()
        self.assertIsInstance(cm, typing.ContextManager)
        self.assertIsInstance(cm, typing.ContextManager[int])
        self.assertNotIsInstance(42, typing.ContextManager)


class TypeTests(BaseTestCase):

    def test_type_basic(self):

        class User: pass
        class BasicUser(User): pass
        class ProUser(User): pass

        def new_user(user_class: Type[User]) -> User:
            return user_class()

        joe = new_user(BasicUser)

    def test_type_typevar(self):

        class User: pass
        class BasicUser(User): pass
        class ProUser(User): pass

        U = TypeVar('U', bound=User)

        def new_user(user_class: Type[U]) -> U:
            return user_class()

        joe = new_user(BasicUser)


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
        self.assertEqual(Emp._field_types, dict(name=str, id=int))

    def test_pickle(self):
        global Emp  # pickle wants to reference the class by name
        Emp = NamedTuple('Emp', [('name', str), ('id', int)])
        jane = Emp('jane', 37)
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            z = pickle.dumps(jane, proto)
            jane2 = pickle.loads(z)
            self.assertEqual(jane2, jane)


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
        self.assertIsSubclass(type(pat), Pattern[str])

        mat = pat.search('12345abcde.....')
        self.assertIsSubclass(mat.__class__, Match)
        self.assertIsSubclass(mat.__class__, Match[str])
        self.assertIsSubclass(mat.__class__, Match[bytes])  # Sad but true.
        self.assertIsSubclass(type(mat), Match)
        self.assertIsSubclass(type(mat), Match[str])

        p = Pattern[Union[str, bytes]]
        self.assertIsSubclass(Pattern[str], Pattern)
        self.assertIsSubclass(Pattern[str], p)

        m = Match[Union[bytes, str]]
        self.assertIsSubclass(Match[bytes], Match)
        self.assertIsSubclass(Match[bytes], m)

    def test_errors(self):
        with self.assertRaises(TypeError):
            # Doesn't fit AnyStr.
            Pattern[int]
        with self.assertRaises(TypeError):
            # Can't change type vars?
            Match[T]
        m = Match[Union[str, bytes]]
        with self.assertRaises(TypeError):
            # Too complicated?
            m[str]
        with self.assertRaises(TypeError):
            # We don't support isinstance().
            isinstance(42, Pattern)
        with self.assertRaises(TypeError):
            # We don't support isinstance().
            isinstance(42, Pattern[str])

    def test_repr(self):
        self.assertEqual(repr(Pattern), 'Pattern[~AnyStr]')
        self.assertEqual(repr(Pattern[str]), 'Pattern[str]')
        self.assertEqual(repr(Pattern[bytes]), 'Pattern[bytes]')
        self.assertEqual(repr(Match), 'Match[~AnyStr]')
        self.assertEqual(repr(Match[str]), 'Match[str]')
        self.assertEqual(repr(Match[bytes]), 'Match[bytes]')

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
                         "A type alias cannot be subclassed")


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


if __name__ == '__main__':
    main()
