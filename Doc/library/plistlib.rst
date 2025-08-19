:mod:`!plistlib` --- Generate and parse Apple ``.plist`` files
==============================================================

.. module:: plistlib
   :synopsis: Generate and parse Apple plist files.

.. moduleauthor:: Jack Jansen
.. sectionauthor:: Georg Brandl <georg@python.org>
.. (harvested from docstrings in the original file)

**Source code:** :source:`Lib/plistlib.py`

.. index::
   pair: plist; file
   single: property list

--------------

This module provides an interface for reading and writing the "property list"
files used by Apple, primarily on macOS and iOS. This module supports both binary
and XML plist files.

The property list (``.plist``) file format is a simple serialization supporting
basic object types, like dictionaries, lists, numbers and strings.  Usually the
top level object is a dictionary.

To write out and to parse a plist file, use the :func:`dump` and
:func:`load` functions.

To work with plist data in bytes or string objects, use :func:`dumps`
and :func:`loads`.

Values can be strings, integers, floats, booleans, tuples, lists, dictionaries
(but only with string keys), :class:`bytes`, :class:`bytearray`
or :class:`datetime.datetime` objects.

.. versionchanged:: 3.4
   New API, old API deprecated.  Support for binary format plists added.

.. versionchanged:: 3.8
   Support added for reading and writing :class:`UID` tokens in binary plists as used
   by NSKeyedArchiver and NSKeyedUnarchiver.

.. versionchanged:: 3.9
   Old API removed.

.. seealso::

   `PList manual page <https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/PropertyLists/>`_
      Apple's documentation of the file format.


This module defines the following functions:

.. function:: load(fp, *, fmt=None, dict_type=dict, aware_datetime=False)

   Read a plist file. *fp* should be a readable and binary file object.
   Return the unpacked root object (which usually is a
   dictionary).

   The *fmt* is the format of the file and the following values are valid:

   * :data:`None`: Autodetect the file format

   * :data:`FMT_XML`: XML file format

   * :data:`FMT_BINARY`: Binary plist format

   The *dict_type* is the type used for dictionaries that are read from the
   plist file.

   When *aware_datetime* is true, fields with type ``datetime.datetime`` will
   be created as :ref:`aware object <datetime-naive-aware>`, with
   :attr:`!tzinfo` as :const:`datetime.UTC`.

   XML data for the :data:`FMT_XML` format is parsed using the Expat parser
   from :mod:`xml.parsers.expat` -- see its documentation for possible
   exceptions on ill-formed XML.  Unknown elements will simply be ignored
   by the plist parser.

   The parser raises :exc:`InvalidFileException` when the file cannot be parsed.

   .. versionadded:: 3.4

   .. versionchanged:: 3.13
      The keyword-only parameter *aware_datetime* has been added.


.. function:: loads(data, *, fmt=None, dict_type=dict, aware_datetime=False)

   Load a plist from a bytes or string object. See :func:`load` for an
   explanation of the keyword arguments.

   .. versionadded:: 3.4

   .. versionchanged:: 3.13
      *data* can be a string when *fmt* equals :data:`FMT_XML`.

.. function:: dump(value, fp, *, fmt=FMT_XML, sort_keys=True, skipkeys=False, aware_datetime=False)

   Write *value* to a plist file. *fp* should be a writable, binary
   file object.

   The *fmt* argument specifies the format of the plist file and can be
   one of the following values:

   * :data:`FMT_XML`: XML formatted plist file

   * :data:`FMT_BINARY`: Binary formatted plist file

   When *sort_keys* is true (the default) the keys for dictionaries will be
   written to the plist in sorted order, otherwise they will be written in
   the iteration order of the dictionary.

   When *skipkeys* is false (the default) the function raises :exc:`TypeError`
   when a key of a dictionary is not a string, otherwise such keys are skipped.

   When *aware_datetime* is true and any field with type ``datetime.datetime``
   is set as an :ref:`aware object <datetime-naive-aware>`, it will convert to
   UTC timezone before writing it.

   A :exc:`TypeError` will be raised if the object is of an unsupported type or
   a container that contains objects of unsupported types.

   An :exc:`OverflowError` will be raised for integer values that cannot
   be represented in (binary) plist files.

   .. versionadded:: 3.4

   .. versionchanged:: 3.13
      The keyword-only parameter *aware_datetime* has been added.


.. function:: dumps(value, *, fmt=FMT_XML, sort_keys=True, skipkeys=False, aware_datetime=False)

   Return *value* as a plist-formatted bytes object. See
   the documentation for :func:`dump` for an explanation of the keyword
   arguments of this function.

   .. versionadded:: 3.4


The following classes are available:

.. class:: UID(data)

   Wraps an :class:`int`.  This is used when reading or writing NSKeyedArchiver
   encoded data, which contains UID (see PList manual).

   .. attribute:: data

      Int value of the UID.  It must be in the range ``0 <= data < 2**64``.

   .. versionadded:: 3.8


The following constants are available:

.. data:: FMT_XML

   The XML format for plist files.

   .. versionadded:: 3.4


.. data:: FMT_BINARY

   The binary format for plist files

   .. versionadded:: 3.4


The module defines the following exceptions:

.. exception:: InvalidFileException

   Raised when a file cannot be parsed.

   .. versionadded:: 3.4


Examples
--------

Generating a plist::

    import datetime
    import plistlib

    pl = dict(
        aString = "Doodah",
        aList = ["A", "B", 12, 32.1, [1, 2, 3]],
        aFloat = 0.1,
        anInt = 728,
        aDict = dict(
            anotherString = "<hello & hi there!>",
            aThirdString = "M\xe4ssig, Ma\xdf",
            aTrueValue = True,
            aFalseValue = False,
        ),
        someData = b"<binary gunk>",
        someMoreData = b"<lots of binary gunk>" * 10,
        aDate = datetime.datetime.now()
    )
    print(plistlib.dumps(pl).decode())

Parsing a plist::

    import plistlib

    plist = b"""<plist version="1.0">
    <dict>
        <key>foo</key>
        <string>bar</string>
    </dict>
    </plist>"""
    pl = plistlib.loads(plist)
    print(pl["foo"])
