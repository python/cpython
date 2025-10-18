:mod:`!tomllib` --- Parse TOML files
====================================

.. module:: tomllib
   :synopsis: Parse TOML files.

.. versionadded:: 3.11

.. moduleauthor:: Taneli Hukkinen
.. sectionauthor:: Taneli Hukkinen

**Source code:** :source:`Lib/tomllib`

--------------

This module provides an interface for parsing TOML 1.0.0 (Tom's Obvious Minimal
Language, `https://toml.io <https://toml.io/en/>`_). This module does not
support writing TOML.

.. seealso::

    The :pypi:`Tomli-W package <tomli-w>`
    is a TOML writer that can be used in conjunction with this module,
    providing a write API familiar to users of the standard library
    :mod:`marshal` and :mod:`pickle` modules.

.. seealso::

    The :pypi:`TOML Kit package <tomlkit>`
    is a style-preserving TOML library with both read and write capability.
    It is a recommended replacement for this module for editing already
    existing TOML files.


This module defines the following functions:

.. function:: load(fp, /, *, parse_float=float)

   Read a TOML file. The first argument should be a readable and binary file object.
   Return a :class:`dict`. Convert TOML types to Python using this
   :ref:`conversion table <toml-to-py-table>`.

   *parse_float* will be called with the string of every TOML
   float to be decoded.  By default, this is equivalent to ``float(num_str)``.
   This can be used to use another datatype or parser for TOML floats
   (e.g. :class:`decimal.Decimal`). The callable must not return a
   :class:`dict` or a :class:`list`, else a :exc:`ValueError` is raised.

   A :exc:`TOMLDecodeError` will be raised on an invalid TOML document.


.. function:: loads(s, /, *, parse_float=float)

   Load TOML from a :class:`str` object. Return a :class:`dict`. Convert TOML
   types to Python using this :ref:`conversion table <toml-to-py-table>`. The
   *parse_float* argument has the same meaning as in :func:`load`.

   A :exc:`TOMLDecodeError` will be raised on an invalid TOML document.


The following exceptions are available:

.. exception:: TOMLDecodeError(msg, doc, pos)

   Subclass of :exc:`ValueError` with the following additional attributes:

   .. attribute:: msg

      The unformatted error message.

   .. attribute:: doc

      The TOML document being parsed.

   .. attribute:: pos

      The index of *doc* where parsing failed.

   .. attribute:: lineno

      The line corresponding to *pos*.

   .. attribute:: colno

      The column corresponding to *pos*.

   .. versionchanged:: 3.14
      Added the *msg*, *doc* and *pos* parameters.
      Added the :attr:`msg`, :attr:`doc`, :attr:`pos`, :attr:`lineno` and :attr:`colno` attributes.

   .. deprecated:: 3.14
      Passing free-form positional arguments is deprecated.


Examples
--------

Parsing a TOML file::

    import tomllib

    with open("pyproject.toml", "rb") as f:
        data = tomllib.load(f)

Parsing a TOML string::

    import tomllib

    toml_str = """
    python-version = "3.11.0"
    python-implementation = "CPython"
    """

    data = tomllib.loads(toml_str)


Conversion Table
----------------

.. _toml-to-py-table:

+------------------+--------------------------------------------------------------------------------------+
| TOML             | Python                                                                               |
+==================+======================================================================================+
| TOML document    | dict                                                                                 |
+------------------+--------------------------------------------------------------------------------------+
| string           | str                                                                                  |
+------------------+--------------------------------------------------------------------------------------+
| integer          | int                                                                                  |
+------------------+--------------------------------------------------------------------------------------+
| float            | float (configurable with *parse_float*)                                              |
+------------------+--------------------------------------------------------------------------------------+
| boolean          | bool                                                                                 |
+------------------+--------------------------------------------------------------------------------------+
| offset date-time | datetime.datetime (``tzinfo`` attribute set to an instance of ``datetime.timezone``) |
+------------------+--------------------------------------------------------------------------------------+
| local date-time  | datetime.datetime (``tzinfo`` attribute set to ``None``)                             |
+------------------+--------------------------------------------------------------------------------------+
| local date       | datetime.date                                                                        |
+------------------+--------------------------------------------------------------------------------------+
| local time       | datetime.time                                                                        |
+------------------+--------------------------------------------------------------------------------------+
| array            | list                                                                                 |
+------------------+--------------------------------------------------------------------------------------+
| table            | dict                                                                                 |
+------------------+--------------------------------------------------------------------------------------+
| inline table     | dict                                                                                 |
+------------------+--------------------------------------------------------------------------------------+
| array of tables  | list of dicts                                                                        |
+------------------+--------------------------------------------------------------------------------------+
