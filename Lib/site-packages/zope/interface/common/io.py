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
Interface definitions paralleling the abstract base classes defined in
:mod:`io`.

After this module is imported, the standard library types will declare
that they implement the appropriate interface.

.. versionadded:: 5.0.0
"""
from __future__ import absolute_import

import io as abc

from zope.interface.common import ABCInterface

# pylint:disable=inherit-non-class,
# pylint:disable=no-member

class IIOBase(ABCInterface):
    abc = abc.IOBase


class IRawIOBase(IIOBase):
    abc = abc.RawIOBase


class IBufferedIOBase(IIOBase):
    abc = abc.BufferedIOBase
    try:
        import cStringIO
    except ImportError:
        # Python 3
        extra_classes = ()
    else:
        import StringIO
        extra_classes = (StringIO.StringIO, cStringIO.InputType, cStringIO.OutputType)
        del cStringIO
        del StringIO


class ITextIOBase(IIOBase):
    abc = abc.TextIOBase
