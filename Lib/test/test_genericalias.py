"""Tests for C-implemented GenericAlias."""

import unittest
import pickle
import copy
from collections import (
    defaultdict, deque, OrderedDict, Counter, UserDict, UserList
)
from collections.abc import *
from concurrent.futures import Future
from concurrent.futures.thread import _WorkItem
from contextlib import AbstractContextManager, AbstractAsyncContextManager
from contextvars import ContextVar, Token
from dataclasses import Field
from functools import partial, partialmethod, cached_property
from graphlib import TopologicalSorter
from logging import LoggerAdapter, StreamHandler
from mailbox import Mailbox, _PartialFile
try:
    import ctypes
except ImportError:
    ctypes = None
from difflib import SequenceMatcher
from filecmp import dircmp
from fileinput import FileInput
from itertools import chain
from http.cookies import Morsel
try:
    from multiprocessing.managers import ValueProxy
    from multiprocessing.pool import ApplyResult
    from multiprocessing.queues import SimpleQueue as MPSimpleQueue
except ImportError:
    # _multiprocessing module is optional
    ValueProxy = None
    ApplyResult = None
    MPSimpleQueue = None
try:
    from multiprocessing.shared_memory import ShareableList
except ImportError:
    # multiprocessing.shared_memory is not available on e.g. Android
    ShareableList = None
from os import DirEntry
from re import Pattern, Match
from types import GenericAlias, MappingProxyType, AsyncGeneratorType
from tempfile import TemporaryDirectory, SpooledTemporaryFile
from urllib.parse import SplitResult, ParseResult
from unittest.case import _AssertRaisesContext
from queue import Queue, SimpleQueue
from weakref import WeakSet, ReferenceType, ref
import typing
from typing import Unpack

from typing import TypeVar
T = TypeVar('T')
K = TypeVar('K')
V = TypeVar('V')

_UNPACKED_TUPLES = [
    # Unpacked tuple using `*`
    (*tuple[int],)[0],
    (*tuple[T],)[0],
    (*tuple[int, str],)[0],
    (*tuple[int, ...],)[0],
    (*tuple[T, ...],)[0],
    tuple[*tuple[int, ...]],
    tuple[*tuple[T, ...]],
    tuple[str, *tuple[int, ...]],
    tuple[*tuple[int, ...], str],
    tuple[float, *tuple[int, ...], str],
    tuple[*tuple[*tuple[int, ...]]],
    # Unpacked tuple using `Unpack`
    Unpack[tuple[int]],
    Unpack[tuple[T]],
    Unpack[tuple[int, str]],
    Unpack[tuple[int, ...]],
    Unpack[tuple[T, ...]],
    tuple[Unpack[tuple[int, ...]]],
    tuple[Unpack[tuple[T, ...]]],
    tuple[str, Unpack[tuple[int, ...]]],
    tuple[Unpack[tuple[int, ...]], str],
    tuple[float, Unpack[tuple[int, ...]], str],
    tuple[Unpack[tuple[Unpack[tuple[int, ...]]]]],
    # Unpacked tuple using `*` AND `Unpack`
    tuple[Unpack[tuple[*tuple[int, ...]]]],
    tuple[*tuple[Unpack[tuple[int, ...]]]],
]


class BaseTest(unittest.TestCase):
    """Test basics."""
    generic_types = [type, tuple, list, dict, set, frozenset, enumerate,
                     defaultdict, deque,
                     SequenceMatcher,
                     dircmp,
                     FileInput,
                     OrderedDict, Counter, UserDict, UserList,
                     Pattern, Match,
                     partial, partialmethod, cached_property,
                     TopologicalSorter,
                     AbstractContextManager, AbstractAsyncContextManager,
                     Awaitable, Coroutine,
                     AsyncIterable, AsyncIterator,
                     AsyncGenerator, Generator,
                     Iterable, Iterator,
                     Reversible,
                     Container, Collection,
                     Mailbox, _PartialFile,
                     ContextVar, Token,
                     Field,
                     Set, MutableSet,
                     Mapping, MutableMapping, MappingView,
                     KeysView, ItemsView, ValuesView,
                     Sequence, MutableSequence,
                     MappingProxyType, AsyncGeneratorType,
                     DirEntry,
                     chain,
                     LoggerAdapter, StreamHandler,
                     TemporaryDirectory, SpooledTemporaryFile,
                     Queue, SimpleQueue,
                     _AssertRaisesContext,
                     SplitResult, ParseResult,
                     WeakSet, ReferenceType, ref,
                     ShareableList,
                     Future, _WorkItem,
                     Morsel]
    if ctypes is not None:
        generic_types.extend((ctypes.Array, ctypes.LibraryLoader))
    if ValueProxy is not None:
        generic_types.extend((ValueProxy, ApplyResult, MPSimpleQueue))

    def test_subscriptable(self):
        for t in self.generic_types:
            if t is None:
                continue
            tname = t.__name__
            with self.subTest(f"Testing {tname}"):
                alias = t[int]
                self.assertIs(alias.__origin__, t)
                self.assertEqual(alias.__args__, (int,))
                self.assertEqual(alias.__parameters__, ())

    def test_unsubscriptable(self):
        for t in int, str, float, Sized, Hashable:
            tname = t.__name__
            with self.subTest(f"Testing {tname}"):
                with self.assertRaisesRegex(TypeError, tname):
                    t[int]

    def test_instantiate(self):
        for t in tuple, list, dict, set, frozenset, defaultdict, deque:
            tname = t.__name__
            with self.subTest(f"Testing {tname}"):
                alias = t[int]
                self.assertEqual(alias(), t())
                if t is dict:
                    self.assertEqual(alias(iter([('a', 1), ('b', 2)])), dict(a=1, b=2))
                    self.assertEqual(alias(a=1, b=2), dict(a=1, b=2))
                elif t is defaultdict:
                    def default():
                        return 'value'
                    a = alias(default)
                    d = defaultdict(default)
                    self.assertEqual(a['test'], d['test'])
                else:
                    self.assertEqual(alias(iter((1, 2, 3))), t((1, 2, 3)))

    def test_unbound_methods(self):
        t = list[int]
        a = t()
        t.append(a, 'foo')
        self.assertEqual(a, ['foo'])
        x = t.__getitem__(a, 0)
        self.assertEqual(x, 'foo')
        self.assertEqual(t.__len__(a), 1)

    def test_subclassing(self):
        class C(list[int]):
            pass
        self.assertEqual(C.__bases__, (list,))
        self.assertEqual(C.__class__, type)

    def test_class_methods(self):
        t = dict[int, None]
        self.assertEqual(dict.fromkeys(range(2)), {0: None, 1: None})  # This works
        self.assertEqual(t.fromkeys(range(2)), {0: None, 1: None})  # Should be equivalent

    def test_no_chaining(self):
        t = list[int]
        with self.assertRaises(TypeError):
            t[int]

    def test_generic_subclass(self):
        class MyList(list):
            pass
        t = MyList[int]
        self.assertIs(t.__origin__, MyList)
        self.assertEqual(t.__args__, (int,))
        self.assertEqual(t.__parameters__, ())

    def test_repr(self):
        class MyList(list):
            pass
        self.assertEqual(repr(list[str]), 'list[str]')
        self.assertEqual(repr(list[()]), 'list[()]')
        self.assertEqual(repr(tuple[int, ...]), 'tuple[int, ...]')
        x1 = tuple[
            tuple(  # Effectively the same as starring; TODO
                tuple[int]
            )
        ]
        self.assertEqual(repr(x1), 'tuple[*tuple[int]]')
        x2 = tuple[
            tuple(  # Ditto TODO
                tuple[int, str]
            )
        ]
        self.assertEqual(repr(x2), 'tuple[*tuple[int, str]]')
        x3 = tuple[
            tuple(  # Ditto TODO
                tuple[int, ...]
            )
        ]
        self.assertEqual(repr(x3), 'tuple[*tuple[int, ...]]')
        self.assertTrue(repr(MyList[int]).endswith('.BaseTest.test_repr.<locals>.MyList[int]'))
        self.assertEqual(repr(list[str]()), '[]')  # instances should keep their normal repr

    def test_exposed_type(self):
        import types
        a = types.GenericAlias(list, int)
        self.assertEqual(str(a), 'list[int]')
        self.assertIs(a.__origin__, list)
        self.assertEqual(a.__args__, (int,))
        self.assertEqual(a.__parameters__, ())

    def test_parameters(self):
        from typing import List, Dict, Callable

        D0 = dict[str, int]
        self.assertEqual(D0.__args__, (str, int))
        self.assertEqual(D0.__parameters__, ())
        D1a = dict[str, V]
        self.assertEqual(D1a.__args__, (str, V))
        self.assertEqual(D1a.__parameters__, (V,))
        D1b = dict[K, int]
        self.assertEqual(D1b.__args__, (K, int))
        self.assertEqual(D1b.__parameters__, (K,))
        D2a = dict[K, V]
        self.assertEqual(D2a.__args__, (K, V))
        self.assertEqual(D2a.__parameters__, (K, V))
        D2b = dict[T, T]
        self.assertEqual(D2b.__args__, (T, T))
        self.assertEqual(D2b.__parameters__, (T,))

        L0 = list[str]
        self.assertEqual(L0.__args__, (str,))
        self.assertEqual(L0.__parameters__, ())
        L1 = list[T]
        self.assertEqual(L1.__args__, (T,))
        self.assertEqual(L1.__parameters__, (T,))
        L2 = list[list[T]]
        self.assertEqual(L2.__args__, (list[T],))
        self.assertEqual(L2.__parameters__, (T,))
        L3 = list[List[T]]
        self.assertEqual(L3.__args__, (List[T],))
        self.assertEqual(L3.__parameters__, (T,))
        L4a = list[Dict[K, V]]
        self.assertEqual(L4a.__args__, (Dict[K, V],))
        self.assertEqual(L4a.__parameters__, (K, V))
        L4b = list[Dict[T, int]]
        self.assertEqual(L4b.__args__, (Dict[T, int],))
        self.assertEqual(L4b.__parameters__, (T,))
        L5 = list[Callable[[K, V], K]]
        self.assertEqual(L5.__args__, (Callable[[K, V], K],))
        self.assertEqual(L5.__parameters__, (K, V))

        T1 = tuple[
            tuple(  # Ditto TODO
                tuple[int]
            )
        ]
        self.assertEqual(
            T1.__args__,
            tuple(  # Ditto TODO
                tuple[int]
            )
        )
        self.assertEqual(T1.__parameters__, ())

        T2 = tuple[
            tuple(  # Ditto TODO
                tuple[T]
            )
        ]
        self.assertEqual(
            T2.__args__,
            tuple(  # Ditto TODO
                tuple[T]
            )
        )
        self.assertEqual(T2.__parameters__, (T,))

        T4 = tuple[
            tuple(  # Ditto TODO
                tuple[int, str]
            )
        ]
        self.assertEqual(
            T4.__args__,
            tuple(  # Ditto TODO
                tuple[int, str]
            )
        )
        self.assertEqual(T4.__parameters__, ())

    def test_parameter_chaining(self):
        from typing import List, Dict, Union, Callable
        self.assertEqual(list[T][int], list[int])
        self.assertEqual(dict[str, T][int], dict[str, int])
        self.assertEqual(dict[T, int][str], dict[str, int])
        self.assertEqual(dict[K, V][str, int], dict[str, int])
        self.assertEqual(dict[T, T][int], dict[int, int])

        self.assertEqual(list[list[T]][int], list[list[int]])
        self.assertEqual(list[dict[T, int]][str], list[dict[str, int]])
        self.assertEqual(list[dict[str, T]][int], list[dict[str, int]])
        self.assertEqual(list[dict[K, V]][str, int], list[dict[str, int]])
        self.assertEqual(dict[T, list[int]][str], dict[str, list[int]])

        self.assertEqual(list[List[T]][int], list[List[int]])
        self.assertEqual(list[Dict[K, V]][str, int], list[Dict[str, int]])
        self.assertEqual(list[Union[K, V]][str, int], list[Union[str, int]])
        self.assertEqual(list[Callable[[K, V], K]][str, int],
                         list[Callable[[str, int], str]])
        self.assertEqual(dict[T, List[int]][str], dict[str, List[int]])

        with self.assertRaises(TypeError):
            list[int][int]
            dict[T, int][str, int]
            dict[str, T][str, int]
            dict[T, T][str, int]

    def test_equality(self):
        self.assertEqual(list[int], list[int])
        self.assertEqual(dict[str, int], dict[str, int])
        self.assertEqual((*tuple[int],)[0], (*tuple[int],)[0])
        self.assertEqual(
            tuple[
                tuple(  # Effectively the same as starring; TODO
                    tuple[int]
                )
            ],
            tuple[
                tuple(  # Ditto TODO
                    tuple[int]
                )
            ]
        )
        self.assertNotEqual(dict[str, int], dict[str, str])
        self.assertNotEqual(list, list[int])
        self.assertNotEqual(list[int], list)
        self.assertNotEqual(list[int], tuple[int])
        self.assertNotEqual((*tuple[int],)[0], tuple[int])

    def test_isinstance(self):
        self.assertTrue(isinstance([], list))
        with self.assertRaises(TypeError):
            isinstance([], list[str])

    def test_issubclass(self):
        class L(list): ...
        self.assertTrue(issubclass(L, list))
        with self.assertRaises(TypeError):
            issubclass(L, list[str])

    def test_type_generic(self):
        t = type[int]
        Test = t('Test', (), {})
        self.assertTrue(isinstance(Test, type))
        test = Test()
        self.assertEqual(t(test), Test)
        self.assertEqual(t(0), int)

    def test_type_subclass_generic(self):
        class MyType(type):
            pass
        with self.assertRaisesRegex(TypeError, 'MyType'):
            MyType[int]

    def test_pickle(self):
        aliases = [GenericAlias(list, T)] + _UNPACKED_TUPLES
        for alias in aliases:
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(alias=alias, proto=proto):
                    s = pickle.dumps(alias, proto)
                    loaded = pickle.loads(s)
                    self.assertEqual(loaded.__origin__, alias.__origin__)
                    self.assertEqual(loaded.__args__, alias.__args__)
                    self.assertEqual(loaded.__parameters__, alias.__parameters__)
                    self.assertEqual(type(loaded), type(alias))

    def test_copy(self):
        class X(list):
            def __copy__(self):
                return self
            def __deepcopy__(self, memo):
                return self

        aliases = [
            GenericAlias(list, T),
            GenericAlias(deque, T),
            GenericAlias(X, T)
        ] + _UNPACKED_TUPLES
        for alias in aliases:
            with self.subTest(alias=alias):
                copied = copy.copy(alias)
                self.assertEqual(copied.__origin__, alias.__origin__)
                self.assertEqual(copied.__args__, alias.__args__)
                self.assertEqual(copied.__parameters__, alias.__parameters__)
                copied = copy.deepcopy(alias)
                self.assertEqual(copied.__origin__, alias.__origin__)
                self.assertEqual(copied.__args__, alias.__args__)
                self.assertEqual(copied.__parameters__, alias.__parameters__)

    def test_unpack(self):
        alias = tuple[str, ...]
        self.assertIs(alias.__unpacked__, False)
        unpacked = (*alias,)[0]
        self.assertIs(unpacked.__unpacked__, True)

    def test_union(self):
        a = typing.Union[list[int], list[str]]
        self.assertEqual(a.__args__, (list[int], list[str]))
        self.assertEqual(a.__parameters__, ())

    def test_union_generic(self):
        a = typing.Union[list[T], tuple[T, ...]]
        self.assertEqual(a.__args__, (list[T], tuple[T, ...]))
        self.assertEqual(a.__parameters__, (T,))

    def test_dir(self):
        dir_of_gen_alias = set(dir(list[int]))
        self.assertTrue(dir_of_gen_alias.issuperset(dir(list)))
        for generic_alias_property in ("__origin__", "__args__", "__parameters__"):
            self.assertIn(generic_alias_property, dir_of_gen_alias)

    def test_weakref(self):
        for t in self.generic_types:
            if t is None:
                continue
            tname = t.__name__
            with self.subTest(f"Testing {tname}"):
                alias = t[int]
                self.assertEqual(ref(alias)(), alias)

    def test_no_kwargs(self):
        # bpo-42576
        with self.assertRaises(TypeError):
            GenericAlias(bad=float)

    def test_subclassing_types_genericalias(self):
        class SubClass(GenericAlias): ...
        alias = SubClass(list, int)
        class Bad(GenericAlias):
            def __new__(cls, *args, **kwargs):
                super().__new__(cls, *args, **kwargs)

        self.assertEqual(alias, list[int])
        with self.assertRaises(TypeError):
            Bad(list, int, bad=int)

    def test_iter_creates_starred_tuple(self):
        t = tuple[int, str]
        iter_t = iter(t)
        x = next(iter_t)
        self.assertEqual(repr(x), '*tuple[int, str]')

    def test_calling_next_twice_raises_stopiteration(self):
        t = tuple[int, str]
        iter_t = iter(t)
        next(iter_t)
        with self.assertRaises(StopIteration):
            next(iter_t)

    def test_del_iter(self):
        t = tuple[int, str]
        iter_x = iter(t)
        del iter_x


if __name__ == "__main__":
    unittest.main()
