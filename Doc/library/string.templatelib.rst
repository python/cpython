:mod:`!string.templatelib` --- Support for template string literals
===================================================================

.. module:: string.templatelib
   :synopsis: Support for template string literals.

**Source code:** :source:`Lib/string/templatelib.py`

--------------

.. versionadded:: 3.14

.. seealso::

   :pep:`750` -- Template Strings

Types
-----

.. class:: Template

.. class:: Interpolation

Helper functions
----------------

.. function:: convert(obj, /, conversion)

   Applies formatted string literal :ref:`conversion <formatstrings-conversion>`
   semantics to the given object *obj*.
   This is frequently useful for custom template string processing logic.

   Three conversion flags are currently supported:

   * ``'!s'`` which calls :func:`str` on the value,
   * ``'!r'`` which calls :func:`repr`, and
   * ``'!a'`` which calls :func:`ascii`.

   If the conversion flag is ``None``, *obj* is returned unchanged.

.. _template-strings:

Template String Syntax
----------------------

.. TODO: Add section similar to :ref:`formatstrings`.
