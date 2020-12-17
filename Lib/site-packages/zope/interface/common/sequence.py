##############################################################################
#
# Copyright (c) 2001, 2002 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
#
##############################################################################
"""
Sequence Interfaces

Importing this module does *not* mark any standard classes as
implementing any of these interfaces.

While this module is not deprecated, new code should generally use
:mod:`zope.interface.common.collections`, specifically
:class:`~zope.interface.common.collections.ISequence` and
:class:`~zope.interface.common.collections.IMutableSequence`. This
module is occasionally useful for its fine-grained breakdown of interfaces.

The standard library :class:`list`, :class:`tuple` and
:class:`collections.UserList`, among others, implement ``ISequence``
or ``IMutableSequence`` but *do not* implement any of the interfaces
in this module.
"""

__docformat__ = 'restructuredtext'
from zope.interface import Interface
from zope.interface.common import collections
from zope.interface._compat import PYTHON2 as PY2

class IMinimalSequence(collections.IIterable):
    """Most basic sequence interface.

    All sequences are iterable.  This requires at least one of the
    following:

    - a `__getitem__()` method that takes a single argument; integer
      values starting at 0 must be supported, and `IndexError` should
      be raised for the first index for which there is no value, or

    - an `__iter__()` method that returns an iterator as defined in
      the Python documentation (http://docs.python.org/lib/typeiter.html).

    """

    def __getitem__(index):
        """``x.__getitem__(index) <==> x[index]``

        Declaring this interface does not specify whether `__getitem__`
        supports slice objects."""

class IFiniteSequence(collections.ISized, IMinimalSequence):
    """
    A sequence of bound size.

    .. versionchanged:: 5.0.0
       Extend ``ISized``
    """

class IReadSequence(collections.IContainer, IFiniteSequence):
    """
    read interface shared by tuple and list

    This interface is similar to
    :class:`~zope.interface.common.collections.ISequence`, but
    requires that all instances be totally ordered. Most users
    should prefer ``ISequence``.

    .. versionchanged:: 5.0.0
       Extend ``IContainer``
    """

    def __contains__(item):
        """``x.__contains__(item) <==> item in x``"""
        # Optional in IContainer, required here.

    def __lt__(other):
        """``x.__lt__(other) <==> x < other``"""

    def __le__(other):
        """``x.__le__(other) <==> x <= other``"""

    def __eq__(other):
        """``x.__eq__(other) <==> x == other``"""

    def __ne__(other):
        """``x.__ne__(other) <==> x != other``"""

    def __gt__(other):
        """``x.__gt__(other) <==> x > other``"""

    def __ge__(other):
        """``x.__ge__(other) <==> x >= other``"""

    def __add__(other):
        """``x.__add__(other) <==> x + other``"""

    def __mul__(n):
        """``x.__mul__(n) <==> x * n``"""

    def __rmul__(n):
        """``x.__rmul__(n) <==> n * x``"""

    if PY2:
        def __getslice__(i, j):
            """``x.__getslice__(i, j) <==> x[i:j]``

            Use of negative indices is not supported.

            Deprecated since Python 2.0 but still a part of `UserList`.
            """

class IExtendedReadSequence(IReadSequence):
    """Full read interface for lists"""

    def count(item):
        """Return number of occurrences of value"""

    def index(item, *args):
        """index(value, [start, [stop]]) -> int

        Return first index of *value*
        """

class IUniqueMemberWriteSequence(Interface):
    """The write contract for a sequence that may enforce unique members"""

    def __setitem__(index, item):
        """``x.__setitem__(index, item) <==> x[index] = item``

        Declaring this interface does not specify whether `__setitem__`
        supports slice objects.
        """

    def __delitem__(index):
        """``x.__delitem__(index) <==> del x[index]``

        Declaring this interface does not specify whether `__delitem__`
        supports slice objects.
        """

    if PY2:
        def __setslice__(i, j, other):
            """``x.__setslice__(i, j, other) <==> x[i:j] = other``

            Use of negative indices is not supported.

            Deprecated since Python 2.0 but still a part of `UserList`.
            """

        def __delslice__(i, j):
            """``x.__delslice__(i, j) <==> del x[i:j]``

            Use of negative indices is not supported.

            Deprecated since Python 2.0 but still a part of `UserList`.
            """

    def __iadd__(y):
        """``x.__iadd__(y) <==> x += y``"""

    def append(item):
        """Append item to end"""

    def insert(index, item):
        """Insert item before index"""

    def pop(index=-1):
        """Remove and return item at index (default last)"""

    def remove(item):
        """Remove first occurrence of value"""

    def reverse():
        """Reverse *IN PLACE*"""

    def sort(cmpfunc=None):
        """Stable sort *IN PLACE*; `cmpfunc(x, y)` -> -1, 0, 1"""

    def extend(iterable):
        """Extend list by appending elements from the iterable"""

class IWriteSequence(IUniqueMemberWriteSequence):
    """Full write contract for sequences"""

    def __imul__(n):
        """``x.__imul__(n) <==> x *= n``"""

class ISequence(IReadSequence, IWriteSequence):
    """
    Full sequence contract.

    New code should prefer
    :class:`~zope.interface.common.collections.IMutableSequence`.

    Compared to that interface, which is implemented by :class:`list`
    (:class:`~zope.interface.common.builtins.IList`), among others,
    this interface is missing the following methods:

        - clear

        - count

        - index

    This interface adds the following methods:

        - sort
    """
