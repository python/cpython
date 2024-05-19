:mod:`!marshal` --- Internal Python object serialization
========================================================

.. module:: marshal
   :synopsis: Convert Python objects to streams of bytes and back (with different
              constraints).

--------------

This module contains functions that can read and write Python values in a binary
format.  The format is specific to Python, but independent of machine
architecture issues (e.g., you can write a Python value to a file on a PC,
transport the file to a Mac, and read it back there).  Details of the format are
undocumented on purpose; it may change between Python versions (although it
rarely does). [#]_

.. index::
   pair: module; pickle
   pair: module; shelve

This is not a general "persistence" module.  For general persistence and
transfer of Python objects through RPC calls, see the modules :mod:`pickle` and
:mod:`shelve`.  The :mod:`marshal` module exists mainly to support reading and
writing the "pseudo-compiled" code for Python modules of :file:`.pyc` files.
Therefore, the Python maintainers reserve the right to modify the marshal format
in backward incompatible ways should the need arise.  If you're serializing and
de-serializing Python objects, use the :mod:`pickle` module instead -- the
performance is comparable, version independence is guaranteed, and pickle
supports a substantially wider range of objects than marshal.

.. warning::

   The :mod:`marshal` module is not intended to be secure against erroneous or
   maliciously constructed data.  Never unmarshal data received from an
   untrusted or unauthenticated source.

.. index:: object; code, code object

Not all Python object types are supported; in general, only objects whose value
is independent from a particular invocation of Python can be written and read by
this module.  The following types are supported: booleans, integers, floating
point numbers, complex numbers, strings, bytes, bytearrays, tuples, lists, sets,
frozensets, dictionaries, and code objects, where it should be understood that
tuples, lists, sets, frozensets and dictionaries are only supported as long as
the values contained therein are themselves supported.  The
singletons :const:`None`, :const:`Ellipsis` and :exc:`StopIteration` can also be
marshalled and unmarshalled.
For format *version* lower than 3, recursive lists, sets and dictionaries cannot
be written (see below).

There are functions that read/write files as well as functions operating on
bytes-like objects.

The module defines these functions:


.. function:: dump(value, file[, version])

   Write the value on the open file.  The value must be a supported type.  The
   file must be a writeable :term:`binary file`.

   If the value has (or contains an object that has) an unsupported type, a
   :exc:`ValueError` exception is raised --- but garbage data will also be written
   to the file.  The object will not be properly read back by :func:`load`.

   The *version* argument indicates the data format that ``dump`` should use
   (see below).

   .. audit-event:: marshal.dumps value,version marshal.dump


.. function:: load(file)

   Read one value from the open file and return it.  If no valid value is read
   (e.g. because the data has a different Python version's incompatible marshal
   format), raise :exc:`EOFError`, :exc:`ValueError` or :exc:`TypeError`.  The
   file must be a readable :term:`binary file`.

   .. audit-event:: marshal.load "" marshal.load

   .. note::

      If an object containing an unsupported type was marshalled with :func:`dump`,
      :func:`load` will substitute ``None`` for the unmarshallable type.

   .. versionchanged:: 3.10

      This call used to raise a ``code.__new__`` audit event for each code object. Now
      it raises a single ``marshal.load`` event for the entire load operation.


.. function:: dumps(value[, version])

   Return the bytes object that would be written to a file by ``dump(value, file)``.  The
   value must be a supported type.  Raise a :exc:`ValueError` exception if value
   has (or contains an object that has) an unsupported type.

   The *version* argument indicates the data format that ``dumps`` should use
   (see below).

   .. audit-event:: marshal.dumps value,version marshal.dump


.. function:: loads(bytes)

   Convert the :term:`bytes-like object` to a value.  If no valid value is found, raise
   :exc:`EOFError`, :exc:`ValueError` or :exc:`TypeError`.  Extra bytes in the
   input are ignored.

   .. audit-event:: marshal.loads bytes marshal.load

   .. versionchanged:: 3.10

      This call used to raise a ``code.__new__`` audit event for each code object. Now
      it raises a single ``marshal.loads`` event for the entire load operation.


In addition, the following constants are defined:

.. data:: version

   Indicates the format that the module uses. Version 0 is the historical
   format, version 1 shares interned strings and version 2 uses a binary format
   for floating point numbers.
   Version 3 adds support for object instancing and recursion.
   The current version is 4.


.. rubric:: Footnotes

.. [#] The name of this module stems from a bit of terminology used by the designers of
   Modula-3 (amongst others), who use the term "marshalling" for shipping of data
   around in a self-contained form. Strictly speaking, "to marshal" means to
   convert some data from internal to external form (in an RPC buffer for instance)
   and "unmarshalling" for the reverse process.

