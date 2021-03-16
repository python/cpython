The Python C API
================

The C API is divided into three sections:

1. ``Include/``
2. ``Include/cpython/``
3. ``Include/internal/``


Include: Limited API and Stable ABI
===================================

``Include/``, excluding the ``cpython`` and ``internal`` subdirectories,
contains the public Limited API and the Stable ABI.  The Stable ABI is
defined by :pep:`384`.

*Note:* Functions stealing references or returning borrowed references
*must not* be added to the Limited API or the Stable ABI.  A strong
reference *must* be returned.  The API in ``Include/`` should not expose
or leak implementation details.

Please start a public discussion before adding new functions or macros to
the Limited API.

Functions or macros with a ``_Py`` prefix do not belong in ``Include/``.


Include/cpython: CPython implementation details
===============================================

``Include/cpython/`` contains the public API that is excluded from the
Limited API and the Stable ABI.  :pep:`384` introduced the
``Py_LIMITED_API`` define to exclude functions from the Limited API.
When a new function or macro is introduced, it has to be explicitly
excluded using ``#ifndef Py_LIMITED_API``.

*Note:* Functions stealing references or returning borrowed references
*must not* be added to ``Include/cpython/``.  A strong reference *must*
be returned.


Include/internal: The internal API
==================================


With PyAPI_FUNC or PyAPI_DATA
-----------------------------

Functions or structures in ``Include/internal/`` defined with
``PyAPI_FUNC`` or ``PyAPI_DATA`` are internal functions which are
exposed only for specific use cases like debuggers and profilers.


With extern keyword
-------------------

Functions in ``Include/internal/`` defined with ``extern`` *must not and can
not* be used outside the CPython code base.  Only built-in stdlib
extensions (built with ``Py_BUILD_CORE_BUILTIN`` macro defined) can use
such functions.

When in doubt, new internal C functions should be defined in
``Include/internal`` using ``extern``.
