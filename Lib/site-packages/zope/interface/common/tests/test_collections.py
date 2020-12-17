##############################################################################
# Copyright (c) 2020 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
##############################################################################


import unittest
try:
    import collections.abc as abc
except ImportError:
    import collections as abc
from collections import deque
from collections import OrderedDict


try:
    from types import MappingProxyType
except ImportError:
    MappingProxyType = object()

from zope.interface import Invalid


# Note that importing z.i.c.collections does work on import.
from zope.interface.common import collections


from zope.interface._compat import PYPY
from zope.interface._compat import PYTHON2 as PY2

from . import add_abc_interface_tests
from . import VerifyClassMixin
from . import VerifyObjectMixin

class TestVerifyClass(VerifyClassMixin, unittest.TestCase):

    # Here we test some known builtin classes that are defined to implement
    # various collection interfaces as a quick sanity test.
    def test_frozenset(self):
        self.assertIsInstance(frozenset(), abc.Set)
        self.assertTrue(self.verify(collections.ISet, frozenset))

    def test_list(self):
        self.assertIsInstance(list(), abc.MutableSequence)
        self.assertTrue(self.verify(collections.IMutableSequence, list))

    # Here we test some derived classes.
    def test_UserList(self):
        self.assertTrue(self.verify(collections.IMutableSequence,
                                    collections.UserList))

    def test_UserDict(self):
        self.assertTrue(self.verify(collections.IMutableMapping,
                                    collections.UserDict))

    def test_UserString(self):
        self.assertTrue(self.verify(collections.ISequence,
                                    collections.UserString))

    def test_non_iterable_UserDict(self):
        try:
            from UserDict import UserDict as NonIterableUserDict # pylint:disable=import-error
        except ImportError:
            # Python 3
            self.skipTest("No UserDict.NonIterableUserDict on Python 3")

        with self.assertRaises(Invalid):
            self.verify(collections.IMutableMapping, NonIterableUserDict)

    # Now we go through the registry, which should have several things,
    # mostly builtins, but if we've imported other libraries already,
    # it could contain things from outside of there too. We aren't concerned
    # about third-party code here, just standard library types. We start with a
    # blacklist of things to exclude, but if that gets out of hand we can figure
    # out a better whitelisting.
    UNVERIFIABLE = {
        # This is declared to be an ISequence, but is missing lots of methods,
        # including some that aren't part of a language protocol, such as
        # ``index`` and ``count``.
        memoryview,
        # 'pkg_resources._vendor.pyparsing.ParseResults' is registered as a
        # MutableMapping but is missing methods like ``popitem`` and ``setdefault``.
        # It's imported due to namespace packages.
        'ParseResults',
        # sqlite3.Row claims ISequence but also misses ``index`` and ``count``.
        # It's imported because...? Coverage imports it, but why do we have it without
        # coverage?
        'Row',
    }

    if PYPY:
        UNVERIFIABLE.update({
            # collections.deque.pop() doesn't support the index= argument to
            # MutableSequence.pop(). We can't verify this on CPython because we can't
            # get the signature, but on PyPy we /can/ get the signature, and of course
            # it doesn't match.
            deque,
            # Likewise for index
            range,
        })
    if PY2:
        # pylint:disable=undefined-variable,no-member
        # There are a lot more types that are fundamentally unverifiable on Python 2.
        UNVERIFIABLE.update({
            # Missing several key methods like __getitem__
            basestring,
            # Missing __iter__ and __contains__, hard to construct.
            buffer,
            # Missing ``__contains__``, ``count`` and ``index``.
            xrange,
            # These two are missing Set.isdisjoint()
            type({}.viewitems()),
            type({}.viewkeys()),
        })
        NON_STRICT_RO = {
        }

add_abc_interface_tests(TestVerifyClass, collections.ISet.__module__)


class TestVerifyObject(VerifyObjectMixin,
                       TestVerifyClass):
    CONSTRUCTORS = {
        collections.IValuesView: {}.values,
        collections.IItemsView: {}.items,
        collections.IKeysView: {}.keys,
        memoryview: lambda: memoryview(b'abc'),
        range: lambda: range(10),
        MappingProxyType: lambda: MappingProxyType({}),
        collections.UserString: lambda: collections.UserString('abc'),
        type(iter(bytearray())): lambda: iter(bytearray()),
        type(iter(b'abc')): lambda: iter(b'abc'),
        'coroutine': unittest.SkipTest,
        type(iter({}.keys())): lambda: iter({}.keys()),
        type(iter({}.items())): lambda: iter({}.items()),
        type(iter({}.values())): lambda: iter({}.values()),
        type((i for i in range(1))): lambda: (i for i in range(3)),
        type(iter([])): lambda: iter([]),
        type(reversed([])): lambda: reversed([]),
        'longrange_iterator': unittest.SkipTest,
        'range_iterator': lambda: iter(range(3)),
        'rangeiterator': lambda: iter(range(3)),
        type(iter(set())): lambda: iter(set()),
        type(iter('')): lambda: iter(''),
        'async_generator': unittest.SkipTest,
        type(iter(tuple())): lambda: iter(tuple()),
    }

    if PY2:
        # pylint:disable=undefined-variable,no-member
        CONSTRUCTORS.update({
            collections.IValuesView: {}.viewvalues,
        })
