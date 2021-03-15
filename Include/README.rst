The Python C API
================

The C API is divided into three sections:

1. ``Include/``
2. ``Include/cpython/``
3. ``Include/internal/``


Include: Limited API and Stable ABI
===================================

``Include/`` contains the public Limited API and the Stable ABI.  The
Stable ABI is defined by :pep:`384`.

Functions stealing references or returning borrowed references *must not*
be added to the Limited API API or the Stable ABI.  A strong reference
*must* be returned.  The API in ``Include/`` should not expose or leak
implementation details.

Please start a public discussion before adding new C API to ``Include/``.

Functions or macros with a ``_Py`` prefix do not belong in ``Include/``.


Include/cpython: CPython implementation details
===============================================

``Include/cpython/`` contains the public API that is excluded from the
Limited and Stable API.  :pep:`384` introduced the ``Py_LIMITED_API``
define to exclude functions from the Limited API.  When a new API is
introduced, it has to be explicitly excluded using
``#ifndef Py_LIMITED_API``.

Functions stealing references or returning borrowed references *must not*
be added to ``Include/cpython/``.  A strong reference *must* be
returned.

See also `bpo-35134 <https://bugs.python.org/issue35134>`_.


Include/internal: The internal API
==================================


With PyAPI_FUNC or PyAPI_DATA
-----------------------------

``Include/internal/`` API defined with ``PyAPI_FUNC`` or ``PyAPI_DATA``
contains the internal API which is exposed for specific use cases like
debuggers and profilers.


With extern keyword
-------------------

``Include/internal/`` API defined with ``extern`` *must not and can not*
be used outside the CPython code base.  Only built-in stdlib extensions
(built with Py_BUILD_CORE_BUILTIN macro defined) can use it.

In case of doubt, new internal C API must be defined in the internal C
API with ``extern``.
