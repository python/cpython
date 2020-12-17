##############################################################################
#
# Copyright (c) 2006 Zope Foundation and Contributors.
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
Support functions for dealing with differences in platforms, including Python
versions and implementations.

This file should have no imports from the rest of zope.interface because it is
used during early bootstrapping.
"""
import os
import sys
import types

if sys.version_info[0] < 3:

    def _normalize_name(name):
        if isinstance(name, basestring):
            return unicode(name)
        raise TypeError("name must be a regular or unicode string")

    CLASS_TYPES = (type, types.ClassType)
    STRING_TYPES = (basestring,)

    _BUILTINS = '__builtin__'

    PYTHON3 = False
    PYTHON2 = True

else:

    def _normalize_name(name):
        if isinstance(name, bytes):
            name = str(name, 'ascii')
        if isinstance(name, str):
            return name
        raise TypeError("name must be a string or ASCII-only bytes")

    CLASS_TYPES = (type,)
    STRING_TYPES = (str,)

    _BUILTINS = 'builtins'

    PYTHON3 = True
    PYTHON2 = False

PYPY = hasattr(sys, 'pypy_version_info')
PYPY2 = PYTHON2 and PYPY

def _skip_under_py3k(test_method):
    import unittest
    return unittest.skipIf(sys.version_info[0] >= 3, "Only on Python 2")(test_method)


def _skip_under_py2(test_method):
    import unittest
    return unittest.skipIf(sys.version_info[0] < 3, "Only on Python 3")(test_method)


def _c_optimizations_required():
    """
    Return a true value if the C optimizations are required.

    This uses the ``PURE_PYTHON`` variable as documented in `_use_c_impl`.
    """
    pure_env = os.environ.get('PURE_PYTHON')
    require_c = pure_env == "0"
    return require_c


def _c_optimizations_available():
    """
    Return the C optimization module, if available, otherwise
    a false value.

    If the optimizations are required but not available, this
    raises the ImportError.

    This does not say whether they should be used or not.
    """
    catch = () if _c_optimizations_required() else (ImportError,)
    try:
        from zope.interface import _zope_interface_coptimizations as c_opt
        return c_opt
    except catch: # pragma: no cover (only Jython doesn't build extensions)
        return False


def _c_optimizations_ignored():
    """
    The opposite of `_c_optimizations_required`.
    """
    pure_env = os.environ.get('PURE_PYTHON')
    return pure_env is not None and pure_env != "0"


def _should_attempt_c_optimizations():
    """
    Return a true value if we should attempt to use the C optimizations.

    This takes into account whether we're on PyPy and the value of the
    ``PURE_PYTHON`` environment variable, as defined in `_use_c_impl`.
    """
    is_pypy = hasattr(sys, 'pypy_version_info')

    if _c_optimizations_required():
        return True
    if is_pypy:
        return False
    return not _c_optimizations_ignored()


def _use_c_impl(py_impl, name=None, globs=None):
    """
    Decorator. Given an object implemented in Python, with a name like
    ``Foo``, import the corresponding C implementation from
    ``zope.interface._zope_interface_coptimizations`` with the name
    ``Foo`` and use it instead.

    If the ``PURE_PYTHON`` environment variable is set to any value
    other than ``"0"``, or we're on PyPy, ignore the C implementation
    and return the Python version. If the C implementation cannot be
    imported, return the Python version. If ``PURE_PYTHON`` is set to
    0, *require* the C implementation (let the ImportError propagate);
    note that PyPy can import the C implementation in this case (and all
    tests pass).

    In all cases, the Python version is kept available. in the module
    globals with the name ``FooPy`` and the name ``FooFallback`` (both
    conventions have been used; the C implementation of some functions
    looks for the ``Fallback`` version, as do some of the Sphinx
    documents).

    Example::

        @_use_c_impl
        class Foo(object):
            ...
    """
    name = name or py_impl.__name__
    globs = globs or sys._getframe(1).f_globals

    def find_impl():
        if not _should_attempt_c_optimizations():
            return py_impl

        c_opt = _c_optimizations_available()
        if not c_opt: # pragma: no cover (only Jython doesn't build extensions)
            return py_impl

        __traceback_info__ = c_opt
        return getattr(c_opt, name)

    c_impl = find_impl()
    # Always make available by the FooPy name and FooFallback
    # name (for testing and documentation)
    globs[name + 'Py'] = py_impl
    globs[name + 'Fallback'] = py_impl

    return c_impl
