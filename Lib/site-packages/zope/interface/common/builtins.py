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
"""
Interface definitions for builtin types.

After this module is imported, the standard library types will declare
that they implement the appropriate interface.

.. versionadded:: 5.0.0
"""
from __future__ import absolute_import

from zope.interface import classImplements

from zope.interface.common import collections
from zope.interface.common import numbers
from zope.interface.common import io

__all__ = [
    'IList',
    'ITuple',
    'ITextString',
    'IByteString',
    'INativeString',
    'IBool',
    'IDict',
    'IFile',
]

# pylint:disable=no-self-argument
class IList(collections.IMutableSequence):
    """
    Interface for :class:`list`
    """
    extra_classes = (list,)

    def sort(key=None, reverse=False):
        """
        Sort the list in place and return None.

        *key* and *reverse* must be passed by name only.
        """


class ITuple(collections.ISequence):
    """
    Interface for :class:`tuple`
    """
    extra_classes = (tuple,)


class ITextString(collections.ISequence):
    """
    Interface for text (unicode) strings.

    On Python 2, this is :class:`unicode`. On Python 3,
    this is :class:`str`
    """
    extra_classes = (type(u'unicode'),)


class IByteString(collections.IByteString):
    """
    Interface for immutable byte strings.

    On all Python versions this is :class:`bytes`.

    Unlike :class:`zope.interface.common.collections.IByteString`
    (the parent of this interface) this does *not* include
    :class:`bytearray`.
    """
    extra_classes = (bytes,)


class INativeString(IByteString if str is bytes else ITextString):
    """
    Interface for native strings.

    On all Python versions, this is :class:`str`. On Python 2,
    this extends :class:`IByteString`, while on Python 3 it extends
    :class:`ITextString`.
    """
# We're not extending ABCInterface so extra_classes won't work
classImplements(str, INativeString)


class IBool(numbers.IIntegral):
    """
    Interface for :class:`bool`
    """
    extra_classes = (bool,)


class IDict(collections.IMutableMapping):
    """
    Interface for :class:`dict`
    """
    extra_classes = (dict,)


class IFile(io.IIOBase):
    """
    Interface for :class:`file`.

    It is recommended to use the interfaces from :mod:`zope.interface.common.io`
    instead of this interface.

    On Python 3, there is no single implementation of this interface;
    depending on the arguments, the :func:`open` builtin can return
    many different classes that implement different interfaces from
    :mod:`zope.interface.common.io`.
    """
    try:
        extra_classes = (file,)
    except NameError:
        extra_classes = ()
