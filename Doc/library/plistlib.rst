:mod:`plistlib` --- Generate and parse Mac OS X ``.plist`` files
================================================================

.. module:: plistlib
   :synopsis: Generate and parse Mac OS X plist files.
.. moduleauthor:: Jack Jansen
.. sectionauthor:: Georg Brandl <georg@python.org>
.. (harvested from docstrings in the original file)

.. index::
   pair: plist; file
   single: property list

**Source code:** :source:`Lib/plistlib.py`

--------------

This module provides an interface for reading and writing the "property list"
XML files used mainly by Mac OS X.

The property list (``.plist``) file format is a simple XML pickle supporting
basic object types, like dictionaries, lists, numbers and strings.  Usually the
top level object is a dictionary.

To write out and to parse a plist file, use the :func:`writePlist` and
:func:`readPlist` functions.

To work with plist data in bytes objects, use :func:`writePlistToBytes`
and :func:`readPlistFromBytes`.

Values can be strings, integers, floats, booleans, tuples, lists, dictionaries
(but only with string keys), :class:`Data` or :class:`datetime.datetime`
objects.  String values (including dictionary keys) have to be unicode strings --
they will be written out as UTF-8.

The ``<data>`` plist type is supported through the :class:`Data` class.  This is
a thin wrapper around a Python bytes object.  Use :class:`Data` if your strings
contain control characters.

.. seealso::

   `PList manual page <http://developer.apple.com/documentation/Darwin/Reference/ManPages/man5/plist.5.html>`_
      Apple's documentation of the file format.


This module defines the following functions:

.. function:: readPlist(pathOrFile)

   Read a plist file. *pathOrFile* may either be a file name or a (readable)
   file object.  Return the unpacked root object (which usually is a
   dictionary).

   The XML data is parsed using the Expat parser from :mod:`xml.parsers.expat`
   -- see its documentation for possible exceptions on ill-formed XML.
   Unknown elements will simply be ignored by the plist parser.


.. function:: writePlist(rootObject, pathOrFile)

    Write *rootObject* to a plist file. *pathOrFile* may either be a file name
    or a (writable) file object.

    A :exc:`TypeError` will be raised if the object is of an unsupported type or
    a container that contains objects of unsupported types.


.. function:: readPlistFromBytes(data)

   Read a plist data from a bytes object.  Return the root object.


.. function:: writePlistToBytes(rootObject)

   Return *rootObject* as a plist-formatted bytes object.


The following class is available:

.. class:: Data(data)

   Return a "data" wrapper object around the bytes object *data*.  This is used
   in functions converting from/to plists to represent the ``<data>`` type
   available in plists.

   It has one attribute, :attr:`data`, that can be used to retrieve the Python
   bytes object stored in it.


Examples
--------

Generating a plist::

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
        someData = Data(b"<binary gunk>"),
        someMoreData = Data(b"<lots of binary gunk>" * 10),
        aDate = datetime.datetime.fromtimestamp(time.mktime(time.gmtime())),
    )
    writePlist(pl, fileName)

Parsing a plist::

    pl = readPlist(pathOrFile)
    print(pl["aKey"])
