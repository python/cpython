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
:mod:`numbers`.

After this module is imported, the standard library types will declare
that they implement the appropriate interface.

.. versionadded:: 5.0.0
"""
from __future__ import absolute_import

import numbers as abc

from zope.interface.common import ABCInterface
from zope.interface.common import optional

from zope.interface._compat import PYTHON2 as PY2

# pylint:disable=inherit-non-class,
# pylint:disable=no-self-argument,no-method-argument
# pylint:disable=unexpected-special-method-signature
# pylint:disable=no-value-for-parameter


class INumber(ABCInterface):
    abc = abc.Number


class IComplex(INumber):
    abc = abc.Complex

    @optional
    def __complex__():
        """
        Rarely implemented, even in builtin types.
        """

    if PY2:
        @optional
        def __eq__(other):
            """
            The interpreter may supply one through complicated rules.
            """

        __ne__ = __eq__

class IReal(IComplex):
    abc = abc.Real

    @optional
    def __complex__():
        """
        Rarely implemented, even in builtin types.
        """

    __floor__ = __ceil__ = __complex__

    if PY2:
        @optional
        def __le__(other):
            """
            The interpreter may supply one through complicated rules.
            """

        __lt__ = __le__


class IRational(IReal):
    abc = abc.Rational


class IIntegral(IRational):
    abc = abc.Integral
