The Python C API
================

The C API is divided into three sections:

1. ``Include/``
2. ``Include/cpython/``
3. ``Include/internal/``


Include: Limited API and Stable API
===================================

``Include/`` contains the public Limited API and the Stable API.  The
Stable API is defined by :pep:`384`.

Functions stealing references or returning borrowed references *must not*
be added to the Limited API API or the Stable API.  A strong reference
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
``#ifndef Py_LIMITED_API``.  If the author forgets it, the function is
added to be Stable and Limited API by mistake!

Functions stealing references or returning borrowed references *must not*
be added to ``Include/cpython/``.  A strong reference *must* be
returned.

Please start a public discussion before adding new C API to
``Include/cpython/``.

The API in ``Include/cpython/`` is prefixed with ``_Py``.

See also `bpo-35134 <https://bugs.python.org/issue35134>`_.


Include/internal: The internal API
==================================

With extern keyword
-------------------

``Include/internal/`` with ``extern`` contains the C API which *must not
and can not* be used outside the CPython code base.  Only stdlib
extensions built as built-in extensions can use it.

In case of doubt, new C API must be added to the internal C API using
``extern``.


With PyAPI_FUNC or PyAPI_DATA
-----------------------------

``Include/internal/`` with ``PyAPI_FUNC`` or ``PyAPI_DATA`` contains the
internal API which is exposed for specific use cases like debuggers and
profilers.
