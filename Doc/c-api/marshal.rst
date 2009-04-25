.. highlightlang:: c

.. _marshalling-utils:

Data marshalling support
========================

These routines allow C code to work with serialized objects using the same
data format as the :mod:`marshal` module.  There are functions to write data
into the serialization format, and additional functions that can be used to
read the data back.  Files used to store marshalled data must be opened in
binary mode.

Numeric values are stored with the least significant byte first.

The module supports two versions of the data format: version 0 is the
historical version, version 1 (new in Python 2.4) shares interned strings in
the file, and upon unmarshalling.  Version 2 (new in Python 2.5) uses a binary
format for floating point numbers.  *Py_MARSHAL_VERSION* indicates the current
file format (currently 2).


.. cfunction:: void PyMarshal_WriteLongToFile(long value, FILE *file, int version)

   Marshal a :ctype:`long` integer, *value*, to *file*.  This will only write
   the least-significant 32 bits of *value*; regardless of the size of the
   native :ctype:`long` type.

   .. versionchanged:: 2.4
      *version* indicates the file format.


.. cfunction:: void PyMarshal_WriteObjectToFile(PyObject *value, FILE *file, int version)

   Marshal a Python object, *value*, to *file*.

   .. versionchanged:: 2.4
      *version* indicates the file format.


.. cfunction:: PyObject* PyMarshal_WriteObjectToString(PyObject *value, int version)

   Return a string object containing the marshalled representation of *value*.

   .. versionchanged:: 2.4
      *version* indicates the file format.


The following functions allow marshalled values to be read back in.

XXX What about error detection?  It appears that reading past the end of the
file will always result in a negative numeric value (where that's relevant),
but it's not clear that negative values won't be handled properly when there's
no error.  What's the right way to tell? Should only non-negative values be
written using these routines?


.. cfunction:: long PyMarshal_ReadLongFromFile(FILE *file)

   Return a C :ctype:`long` from the data stream in a :ctype:`FILE\*` opened
   for reading.  Only a 32-bit value can be read in using this function,
   regardless of the native size of :ctype:`long`.


.. cfunction:: int PyMarshal_ReadShortFromFile(FILE *file)

   Return a C :ctype:`short` from the data stream in a :ctype:`FILE\*` opened
   for reading.  Only a 16-bit value can be read in using this function,
   regardless of the native size of :ctype:`short`.


.. cfunction:: PyObject* PyMarshal_ReadObjectFromFile(FILE *file)

   Return a Python object from the data stream in a :ctype:`FILE\*` opened for
   reading.  On error, sets the appropriate exception (:exc:`EOFError` or
   :exc:`TypeError`) and returns *NULL*.


.. cfunction:: PyObject* PyMarshal_ReadLastObjectFromFile(FILE *file)

   Return a Python object from the data stream in a :ctype:`FILE\*` opened for
   reading.  Unlike :cfunc:`PyMarshal_ReadObjectFromFile`, this function
   assumes that no further objects will be read from the file, allowing it to
   aggressively load file data into memory so that the de-serialization can
   operate from data in memory rather than reading a byte at a time from the
   file.  Only use these variant if you are certain that you won't be reading
   anything else from the file.  On error, sets the appropriate exception
   (:exc:`EOFError` or :exc:`TypeError`) and returns *NULL*.


.. cfunction:: PyObject* PyMarshal_ReadObjectFromString(char *string, Py_ssize_t len)

   Return a Python object from the data stream in a character buffer
   containing *len* bytes pointed to by *string*.  On error, sets the
   appropriate exception (:exc:`EOFError` or :exc:`TypeError`) and returns
   *NULL*.

   .. versionchanged:: 2.5
      This function used an :ctype:`int` type for *len*. This might require
      changes in your code for properly supporting 64-bit systems.
