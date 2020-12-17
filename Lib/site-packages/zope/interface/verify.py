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
"""Verify interface implementations
"""
from __future__ import print_function
import inspect
import sys
from types import FunctionType
from types import MethodType

from zope.interface._compat import PYPY2

from zope.interface.exceptions import BrokenImplementation
from zope.interface.exceptions import BrokenMethodImplementation
from zope.interface.exceptions import DoesNotImplement
from zope.interface.exceptions import Invalid
from zope.interface.exceptions import MultipleInvalid

from zope.interface.interface import fromMethod, fromFunction, Method

__all__ = [
    'verifyObject',
    'verifyClass',
]

# This will be monkey-patched when running under Zope 2, so leave this
# here:
MethodTypes = (MethodType, )


def _verify(iface, candidate, tentative=False, vtype=None):
    """
    Verify that *candidate* might correctly provide *iface*.

    This involves:

    - Making sure the candidate claims that it provides the
      interface using ``iface.providedBy`` (unless *tentative* is `True`,
      in which case this step is skipped). This means that the candidate's class
      declares that it `implements <zope.interface.implementer>` the interface,
      or the candidate itself declares that it `provides <zope.interface.provider>`
      the interface

    - Making sure the candidate defines all the necessary methods

    - Making sure the methods have the correct signature (to the
      extent possible)

    - Making sure the candidate defines all the necessary attributes

    :return bool: Returns a true value if everything that could be
       checked passed.
    :raises zope.interface.Invalid: If any of the previous
       conditions does not hold.

    .. versionchanged:: 5.0
        If multiple methods or attributes are invalid, all such errors
        are collected and reported. Previously, only the first error was reported.
        As a special case, if only one such error is present, it is raised
        alone, like before.
    """

    if vtype == 'c':
        tester = iface.implementedBy
    else:
        tester = iface.providedBy

    excs = []
    if not tentative and not tester(candidate):
        excs.append(DoesNotImplement(iface, candidate))

    for name, desc in iface.namesAndDescriptions(all=True):
        try:
            _verify_element(iface, name, desc, candidate, vtype)
        except Invalid as e:
            excs.append(e)

    if excs:
        if len(excs) == 1:
            raise excs[0]
        raise MultipleInvalid(iface, candidate, excs)

    return True

def _verify_element(iface, name, desc, candidate, vtype):
    # Here the `desc` is either an `Attribute` or `Method` instance
    try:
        attr = getattr(candidate, name)
    except AttributeError:
        if (not isinstance(desc, Method)) and vtype == 'c':
            # We can't verify non-methods on classes, since the
            # class may provide attrs in it's __init__.
            return
        # TODO: On Python 3, this should use ``raise...from``
        raise BrokenImplementation(iface, desc, candidate)

    if not isinstance(desc, Method):
        # If it's not a method, there's nothing else we can test
        return

    if inspect.ismethoddescriptor(attr) or inspect.isbuiltin(attr):
        # The first case is what you get for things like ``dict.pop``
        # on CPython (e.g., ``verifyClass(IFullMapping, dict))``). The
        # second case is what you get for things like ``dict().pop`` on
        # CPython (e.g., ``verifyObject(IFullMapping, dict()))``.
        # In neither case can we get a signature, so there's nothing
        # to verify. Even the inspect module gives up and raises
        # ValueError: no signature found. The ``__text_signature__`` attribute
        # isn't typically populated either.
        #
        # Note that on PyPy 2 or 3 (up through 7.3 at least), these are
        # not true for things like ``dict.pop`` (but might be true for C extensions?)
        return

    if isinstance(attr, FunctionType):
        if sys.version_info[0] >= 3 and isinstance(candidate, type) and vtype == 'c':
            # This is an "unbound method" in Python 3.
            # Only unwrap this if we're verifying implementedBy;
            # otherwise we can unwrap @staticmethod on classes that directly
            # provide an interface.
            meth = fromFunction(attr, iface, name=name,
                                imlevel=1)
        else:
            # Nope, just a normal function
            meth = fromFunction(attr, iface, name=name)
    elif (isinstance(attr, MethodTypes)
          and type(attr.__func__) is FunctionType):
        meth = fromMethod(attr, iface, name)
    elif isinstance(attr, property) and vtype == 'c':
        # Without an instance we cannot be sure it's not a
        # callable.
        # TODO: This should probably check inspect.isdatadescriptor(),
        # a more general form than ``property``
        return

    else:
        if not callable(attr):
            raise BrokenMethodImplementation(desc, "implementation is not a method",
                                             attr, iface, candidate)
        # sigh, it's callable, but we don't know how to introspect it, so
        # we have to give it a pass.
        return

    # Make sure that the required and implemented method signatures are
    # the same.
    mess = _incompat(desc.getSignatureInfo(), meth.getSignatureInfo())
    if mess:
        if PYPY2 and _pypy2_false_positive(mess, candidate, vtype):
            return
        raise BrokenMethodImplementation(desc, mess, attr, iface, candidate)



def verifyClass(iface, candidate, tentative=False):
    """
    Verify that the *candidate* might correctly provide *iface*.
    """
    return _verify(iface, candidate, tentative, vtype='c')

def verifyObject(iface, candidate, tentative=False):
    return _verify(iface, candidate, tentative, vtype='o')

verifyObject.__doc__ = _verify.__doc__

_MSG_TOO_MANY = 'implementation requires too many arguments'
_KNOWN_PYPY2_FALSE_POSITIVES = frozenset((
    _MSG_TOO_MANY,
))


def _pypy2_false_positive(msg, candidate, vtype):
    # On PyPy2, builtin methods and functions like
    # ``dict.pop`` that take pseudo-optional arguments
    # (those with no default, something you can't express in Python 2
    # syntax; CPython uses special internal APIs to implement these methods)
    # return false failures because PyPy2 doesn't expose any way
    # to detect this pseudo-optional status. PyPy3 doesn't have this problem
    # because of __defaults_count__, and CPython never gets here because it
    # returns true for ``ismethoddescriptor`` or ``isbuiltin``.
    #
    # We can't catch all such cases, but we can handle the common ones.
    #
    if msg not in _KNOWN_PYPY2_FALSE_POSITIVES:
        return False

    known_builtin_types = vars(__builtins__).values()
    candidate_type = candidate if vtype == 'c' else type(candidate)
    if candidate_type in known_builtin_types:
        return True

    return False


def _incompat(required, implemented):
    #if (required['positional'] !=
    #    implemented['positional'][:len(required['positional'])]
    #    and implemented['kwargs'] is None):
    #    return 'imlementation has different argument names'
    if len(implemented['required']) > len(required['required']):
        return _MSG_TOO_MANY
    if ((len(implemented['positional']) < len(required['positional']))
        and not implemented['varargs']):
        return "implementation doesn't allow enough arguments"
    if required['kwargs'] and not implemented['kwargs']:
        return "implementation doesn't support keyword arguments"
    if required['varargs'] and not implemented['varargs']:
        return "implementation doesn't support variable arguments"
