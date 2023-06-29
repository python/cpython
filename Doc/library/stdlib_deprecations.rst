:mod:`stdlib_deprecations` --- Standard library deprecations
============================================================

.. module:: stdlib_deprecations
   :synopsis: Standard library deprecations.

**Source code:** :source:`Lib/stdlib_deprecations.py`

--------------

Standard library deprecations.

.. versionadded:: 3.13

.. class:: Deprecated

   Attributes::

   .. attribute:: name

      API name (string).

   .. attribute:: version

      Version when the API was deprecated (tuple of int).

   .. attribute:: remove

      Version when the API was or will be removed (tuple of int, or None).

   .. attribute:: message

      Message about the deprecation, usually explain how to replace the
      deprecated API (str or None).


.. function:: get_deprecated(name)

   Return a :class:`Deprecated` object if the specified module or function is deprecated.
   For a function name, return an :class:`Deprecated` object if its module is deprecated.
   Return ``None`` if the API is not deprecated.


.. function:: get_capi_deprecated(name)

   Return a :class:`Deprecated` object if the specified C API symbol is deprecated.
   Return ``None`` if the C API is not deprecated.
