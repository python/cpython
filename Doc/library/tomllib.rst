:mod:`tomllib` --- Parse TOML files
===================================

.. module:: tomllib
   :synopsis: Parse TOML files.

.. versionadded:: 3.11

.. moduleauthor:: Taneli Hukkinen
.. sectionauthor:: Taneli Hukkinen

**Source code:** :source:`Lib/tomllib`

--------------

This module provides an interface for parsing TOML (Tom's Obvious Minimal
Language, `https://toml.io <https://toml.io/en/>`_). This module does not
support writing TOML.

.. seealso::

    The `Tomli-W package <https://pypi.org/project/tomli-w/>`__
    is a TOML writer that can be used in conjunction with this module,
    providing a write API familiar to users of the standard library
    :mod:`marshal` and :mod:`pickle` modules.

.. seealso::

    The `TOML Kit package <https://pypi.org/project/tomlkit/>`__
    is a style-preserving TOML library with both read and write capability.
    It is a recommended replacement for this module for editing already
    existing TOML files.


This module defines the following functions:

.. function:: load(fp, /, *, parse_float=float)

   Read a TOML file. The first argument should be a readable and binary file object.
   Return a :class:`dict`.

   *parse_float* will be called with the string of every TOML
   float to be decoded.  By default, this is equivalent to ``float(num_str)``.
   This can be used to use another datatype or parser for TOML floats
   (e.g. :class:`decimal.Decimal`).

   A :exc:`TOMLDecodeError` will be raised on an invalid TOML document.


.. function:: loads(s, /, *, parse_float=float)

   Load TOML from a :class:`str` object. The *parse_float* argument has the
   same meaning as in :func:`load`.

   A :exc:`TOMLDecodeError` will be raised on an invalid TOML document.


The following exceptions are available:

.. exception:: TOMLDecodeError

   Subclass of :exc:`ValueError`.


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
