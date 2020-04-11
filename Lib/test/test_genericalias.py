"""Tests for C-implemented GenericAlias."""

import unittest
import pickle
from collections import (
    defaultdict, deque, OrderedDict, Counter, UserDict, UserList
)
from collections.abc import *
from concurrent.futures import Future
from concurrent.futures.thread import _WorkItem
from contextlib import AbstractContextManager, AbstractAsyncContextManager
from ctypes import Array, LibraryLoader
from difflib import SequenceMatcher
from filecmp import dircmp
from fileinput import FileInput
from mmap import mmap
from ipaddress import IPv4Network, IPv4Interface, IPv6Network, IPv6Interface
from itertools import chain
from http.cookies import Morsel
from multiprocessing.managers import ValueProxy
from multiprocessing.pool import ApplyResult
try:
    from multiprocessing.shared_memory import ShareableList
except ImportError:
    # multiprocessing.shared_memory is not available on e.g. Android
    ShareableList = None
from multiprocessing.queues import SimpleQueue
from os import DirEntry
from re import Pattern, Match
from types import GenericAlias, MappingProxyType, AsyncGeneratorType
from tempfile import TemporaryDirectory, SpooledTemporaryFile
from urllib.parse import SplitResult, ParseResult
from unittest.case import _AssertRaisesContext
from queue import Queue, SimpleQueue
import typing

from typing import TypeVar
T = TypeVar('T')

class BaseTest(unittest.TestCase):
    """Test basics."""

    def test_subscriptable(self):
        for t in (type, tuple, list, dict, set, frozenset, enumerate,
                  mmap,
                  defaultdict, deque,
                  SequenceMatcher,
                  dircmp,
                  FileInput,
                  OrderedDict, Counter, UserDict, UserList,
                  Pattern, Match,
                  AbstractContextManager, AbstractAsyncContextManager,
                  Awaitable, Coroutine,
                  AsyncIterable, AsyncIterator,
                  AsyncGenerator, Generator,
                  Iterable, Iterator,
                  Reversible,
                  Container, Collection,
                  Callable,
                  Set, MutableSet,
                  Mapping, MutableMapping, MappingView,
                  KeysView, ItemsView, ValuesView,
                  Sequence, MutableSequence,
                  MappingProxyType, AsyncGeneratorType,
                  DirEntry,
                  IPv4Network, IPv4Interface, IPv6Network, IPv6Interface,
                  chain,
                  TemporaryDirectory, SpooledTemporaryFile,
                  Queue, SimpleQueue,
                  _AssertRaisesContext,
                  Array, LibraryLoader,
                  SplitResult, ParseResult,
                  ValueProxy, ApplyResult,
                  ShareableList, SimpleQueue,
                  Future, _WorkItem,
                  Morsel,
                  ):
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
                with self.assertRaises(TypeError):
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
        from typing import TypeVar
        T = TypeVar('T')
        K = TypeVar('K')
        V = TypeVar('V')
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

    def test_parameter_chaining(self):
        from typing import TypeVar
        T = TypeVar('T')
        self.assertEqual(list[T][int], list[int])
        self.assertEqual(dict[str, T][int], dict[str, int])
        self.assertEqual(dict[T, int][str], dict[str, int])
        self.assertEqual(dict[T, T][int], dict[int, int])
        with self.assertRaises(TypeError):
            list[int][int]
            dict[T, int][str, int]
            dict[str, T][str, int]
            dict[T, T][str, int]

    def test_equality(self):
        self.assertEqual(list[int], list[int])
        self.assertEqual(dict[str, int], dict[str, int])
        self.assertNotEqual(dict[str, int], dict[str, str])
        self.assertNotEqual(list, list[int])
        self.assertNotEqual(list[int], list)

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
        with self.assertRaises(TypeError):
            MyType[int]

    def test_pickle(self):
        alias = GenericAlias(list, T)
        s = pickle.dumps(alias)
        loaded = pickle.loads(s)
        self.assertEqual(alias.__origin__, loaded.__origin__)
        self.assertEqual(alias.__args__, loaded.__args__)
        self.assertEqual(alias.__parameters__, loaded.__parameters__)

    def test_union(self):
        a = typing.Union[list[int], list[str]]
        self.assertEqual(a.__args__, (list[int], list[str]))
        self.assertEqual(a.__parameters__, ())

    def test_union_generic(self):
        T = typing.TypeVar('T')
        a = typing.Union[list[T], tuple[T, ...]]
        self.assertEqual(a.__args__, (list[T], tuple[T, ...]))
        self.assertEqual(a.__parameters__, (T,))


if __name__ == "__main__":
    unittest.main()
