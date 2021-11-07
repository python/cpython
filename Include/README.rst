The Python C API
================

The C API is divided into three sections:

1. ``Include/``
2. ``Include/cpython/``
3. ``Include/internal/``


Include: Limited API
====================

``Include/``, excluding the ``cpython`` and ``internal`` subdirectories,
contains the public Limited API (Application Programming Interface).
The Limited API is a subset of the C API, designed to guarantee ABI
stability across Python 3 versions, and is defined in :pep:`384`.

Guidelines for expanding the Limited API:

- Functions *must not* steal references
- Functions *must not* return borrowed references
- Functions returning references *must* return a strong reference
- Macros should not expose implementation details
- Please start a public discussion before expanding the API
- Functions or macros with a ``_Py`` prefix do not belong in ``Include/``.

It is possible to add a function or macro to the Limited API from a
given Python version.  For example, to add a function to the Limited API
from Python 3.10 and onwards, wrap it with
``#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030A0000``.


Include/cpython: CPython implementation details
===============================================

``Include/cpython/`` contains the public API that is excluded from the
Limited API and the Stable ABI.

Guidelines for expanding the public API:

- Functions *must not* steal references
- Functions *must not* return borrowed references
- Functions returning references *must* return a strong reference


Include/internal: The internal API
==================================


With PyAPI_FUNC or PyAPI_DATA
-----------------------------

Functions or structures in ``Include/internal/`` defined with
``PyAPI_FUNC`` or ``PyAPI_DATA`` are internal functions which are
exposed only for specific use cases like debuggers and profilers.


With the extern keyword
-----------------------

Functions in ``Include/internal/`` defined with the ``extern`` keyword
*must not and can not* be used outside the CPython code base.  Only
built-in stdlib extensions (built with the ``Py_BUILD_CORE_BUILTIN``
macro defined) can use such functions.

When in doubt, new internal C functions should be defined in
``Include/internal`` using the ``extern`` keyword.
