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
Mapping Interfaces.

Importing this module does *not* mark any standard classes as
implementing any of these interfaces.

While this module is not deprecated, new code should generally use
:mod:`zope.interface.common.collections`, specifically
:class:`~zope.interface.common.collections.IMapping` and
:class:`~zope.interface.common.collections.IMutableMapping`. This
module is occasionally useful for its extremely fine grained breakdown
of interfaces.

The standard library :class:`dict` and :class:`collections.UserDict`
implement ``IMutableMapping``, but *do not* implement any of the
interfaces in this module.
"""
from zope.interface import Interface
from zope.interface._compat import PYTHON2 as PY2
from zope.interface.common import collections

class IItemMapping(Interface):
    """Simplest readable mapping object
    """

    def __getitem__(key):
        """Get a value for a key

        A `KeyError` is raised if there is no value for the key.
        """


class IReadMapping(collections.IContainer, IItemMapping):
    """
    Basic mapping interface.

    .. versionchanged:: 5.0.0
       Extend ``IContainer``
    """

    def get(key, default=None):
        """Get a value for a key

        The default is returned if there is no value for the key.
        """

    def __contains__(key):
        """Tell if a key exists in the mapping."""
        # Optional in IContainer, required by this interface.


class IWriteMapping(Interface):
    """Mapping methods for changing data"""

    def __delitem__(key):
        """Delete a value from the mapping using the key."""

    def __setitem__(key, value):
        """Set a new item in the mapping."""


class IEnumerableMapping(collections.ISized, IReadMapping):
    """
    Mapping objects whose items can be enumerated.

    .. versionchanged:: 5.0.0
       Extend ``ISized``
    """

    def keys():
        """Return the keys of the mapping object.
        """

    def __iter__():
        """Return an iterator for the keys of the mapping object.
        """

    def values():
        """Return the values of the mapping object.
        """

    def items():
        """Return the items of the mapping object.
        """

class IMapping(IWriteMapping, IEnumerableMapping):
    ''' Simple mapping interface '''

class IIterableMapping(IEnumerableMapping):
    """A mapping that has distinct methods for iterating
    without copying.

    On Python 2, a `dict` has these methods, but on Python 3
    the methods defined in `IEnumerableMapping` already iterate
    without copying.
    """

    if PY2:
        def iterkeys():
            "iterate over keys; equivalent to ``__iter__``"

        def itervalues():
            "iterate over values"

        def iteritems():
            "iterate over items"

class IClonableMapping(Interface):
    """Something that can produce a copy of itself.

    This is available in `dict`.
    """

    def copy():
        "return copy of dict"

class IExtendedReadMapping(IIterableMapping):
    """
    Something with a particular method equivalent to ``__contains__``.

    On Python 2, `dict` provides this method, but it was removed
    in Python 3.
    """

    if PY2:
        def has_key(key):
            """Tell if a key exists in the mapping; equivalent to ``__contains__``"""

class IExtendedWriteMapping(IWriteMapping):
    """Additional mutation methods.

    These are all provided by `dict`.
    """

    def clear():
        "delete all items"

    def update(d):
        " Update D from E: for k in E.keys(): D[k] = E[k]"

    def setdefault(key, default=None):
        "D.setdefault(k[,d]) -> D.get(k,d), also set D[k]=d if k not in D"

    def pop(k, default=None):
        """
        pop(k[,default]) -> value

        Remove specified key and return the corresponding value.

        If key is not found, *default* is returned if given, otherwise
        `KeyError` is raised. Note that *default* must not be passed by
        name.
        """

    def popitem():
        """remove and return some (key, value) pair as a
        2-tuple; but raise KeyError if mapping is empty"""

class IFullMapping(
        collections.IMutableMapping,
        IExtendedReadMapping, IExtendedWriteMapping, IClonableMapping, IMapping,):
    """
    Full mapping interface.

    Most uses of this interface should instead use
    :class:`~zope.interface.commons.collections.IMutableMapping` (one of the
    bases of this interface). The required methods are the same.

    .. versionchanged:: 5.0.0
       Extend ``IMutableMapping``
    """
