
:mod:`marshal` --- Internal Python object serialization
=======================================================

.. module:: marshal
   :synopsis: Convert Python objects to streams of bytes and back (with different
              constraints).


This module contains functions that can read and write Python values in a binary
format.  The format is specific to Python, but independent of machine
architecture issues (e.g., you can write a Python value to a file on a PC,
transport the file to a Sun, and read it back there).  Details of the format are
undocumented on purpose; it may change between Python versions (although it
rarely does). [#]_

.. index::
   module: pickle
   module: shelve
   object: code

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

Not all Python object types are supported; in general, only objects whose value
is independent from a particular invocation of Python can be written and read by
this module.  The following types are supported: ``None``, integers, long
integers, floating point numbers, strings, Unicode objects, tuples, lists, sets,
dictionaries, and code objects, where it should be understood that tuples, lists
and dictionaries are only supported as long as the values contained therein are
themselves supported; and recursive lists and dictionaries should not be written
(they will cause infinite loops).

.. warning::

   On machines where C's ``long int`` type has more than 32 bits (such as the
   DEC Alpha), it is possible to create plain Python integers that are longer
   than 32 bits. If such an integer is marshaled and read back in on a machine
   where C's ``long int`` type has only 32 bits, a Python long integer object
   is returned instead.  While of a different type, the numeric value is the
   same.  (This behavior is new in Python 2.2.  In earlier versions, all but the
   least-significant 32 bits of the value were lost, and a warning message was
   printed.)

There are functions that read/write files as well as functions operating on
strings.

The module defines these functions:


.. function:: dump(value, file[, version])

   Write the value on the open file.  The value must be a supported type.  The
   file must be an open file object such as ``sys.stdout`` or returned by
   :func:`open` or :func:`os.popen`.  It must be opened in binary mode (``'wb'``
   or ``'w+b'``).

   If the value has (or contains an object that has) an unsupported type, a
   :exc:`ValueError` exception is raised --- but garbage data will also be written
   to the file.  The object will not be properly read back by :func:`load`.

   .. versionadded:: 2.4
      The *version* argument indicates the data format that ``dump`` should use
      (see below).


.. function:: load(file)

   Read one value from the open file and return it.  If no valid value is read
   (e.g. because the data has a different Python version's incompatible marshal
   format), raise :exc:`EOFError`, :exc:`ValueError` or :exc:`TypeError`.  The
   file must be an open file object opened in binary mode (``'rb'`` or
   ``'r+b'``).

   .. note::

      If an object containing an unsupported type was marshalled with :func:`dump`,
      :func:`load` will substitute ``None`` for the unmarshallable type.


.. function:: dumps(value[, version])

   Return the string that would be written to a file by ``dump(value, file)``.  The
   value must be a supported type.  Raise a :exc:`ValueError` exception if value
   has (or contains an object that has) an unsupported type.

   .. versionadded:: 2.4
      The *version* argument indicates the data format that ``dumps`` should use
      (see below).


.. function:: loads(string)

   Convert the string to a value.  If no valid value is found, raise
   :exc:`EOFError`, :exc:`ValueError` or :exc:`TypeError`.  Extra characters in the
   string are ignored.


In addition, the following constants are defined:

.. data:: version

   Indicates the format that the module uses. Version 0 is the historical format,
   version 1 (added in Python 2.4) shares interned strings and version 2 (added in
   Python 2.5) uses a binary format for floating point numbers. The current version
   is 2.

   .. versionadded:: 2.4


.. rubric:: Footnotes

.. [#] The name of this module stems from a bit of terminology used by the designers of
   Modula-3 (amongst others), who use the term "marshalling" for shipping of data
   around in a self-contained form. Strictly speaking, "to marshal" means to
   convert some data from internal to external form (in an RPC buffer for instance)
   and "unmarshalling" for the reverse process.

