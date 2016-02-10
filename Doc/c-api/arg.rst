.. highlightlang:: c

.. _arg-parsing:

Parsing arguments and building values
=====================================

These functions are useful when creating your own extensions functions and
methods.  Additional information and examples are available in
:ref:`extending-index`.

The first three of these functions described, :c:func:`PyArg_ParseTuple`,
:c:func:`PyArg_ParseTupleAndKeywords`, and :c:func:`PyArg_Parse`, all use
*format strings* which are used to tell the function about the expected
arguments.  The format strings use the same syntax for each of these
functions.

A format string consists of zero or more "format units."  A format unit
describes one Python object; it is usually a single character or a
parenthesized sequence of format units.  With a few exceptions, a format unit
that is not a parenthesized sequence normally corresponds to a single address
argument to these functions.  In the following description, the quoted form is
the format unit; the entry in (round) parentheses is the Python object type
that matches the format unit; and the entry in [square] brackets is the type
of the C variable(s) whose address should be passed.

These formats allow accessing an object as a contiguous chunk of memory.
You don't have to provide raw storage for the returned unicode or bytes
area.  Also, you won't have to release any memory yourself, except with the
``es``, ``es#``, ``et`` and ``et#`` formats.

``s`` (string or Unicode) [const char \*]
   Convert a Python string or Unicode object to a C pointer to a character
   string.  You must not provide storage for the string itself; a pointer to
   an existing string is stored into the character pointer variable whose
   address you pass.  The C string is NUL-terminated.  The Python string must
   not contain embedded NUL bytes; if it does, a :exc:`TypeError` exception is
   raised. Unicode objects are converted to C strings using the default
   encoding.  If this conversion fails, a :exc:`UnicodeError` is raised.

``s#`` (string, Unicode or any read buffer compatible object) [const char \*, int (or :c:type:`Py_ssize_t`, see below)]
   This variant on ``s`` stores into two C variables, the first one a pointer
   to a character string, the second one its length.  In this case the Python
   string may contain embedded null bytes.  Unicode objects pass back a
   pointer to the default encoded string version of the object if such a
   conversion is possible.  All other read-buffer compatible objects pass back
   a reference to the raw internal data representation.

   Starting with Python 2.5 the type of the length argument can be controlled
   by defining the macro :c:macro:`PY_SSIZE_T_CLEAN` before including
   :file:`Python.h`.  If the macro is defined, length is a :c:type:`Py_ssize_t`
   rather than an int.

``s*`` (string, Unicode, or any buffer compatible object) [Py_buffer]
   Similar to ``s#``, this code fills a Py_buffer structure provided by the
   caller.  The buffer gets locked, so that the caller can subsequently use
   the buffer even inside a ``Py_BEGIN_ALLOW_THREADS`` block; the caller is
   responsible for calling ``PyBuffer_Release`` with the structure after it
   has processed the data.

   .. versionadded:: 2.6

``z`` (string, Unicode  or ``None``) [const char \*]
   Like ``s``, but the Python object may also be ``None``, in which case the C
   pointer is set to *NULL*.

``z#`` (string, Unicode, ``None`` or any read buffer compatible object) [const char \*, int]
   This is to ``s#`` as ``z`` is to ``s``.

``z*`` (string, Unicode, ``None`` or any buffer compatible object) [Py_buffer]
   This is to ``s*`` as ``z`` is to ``s``.

   .. versionadded:: 2.6

``u`` (Unicode) [Py_UNICODE \*]
   Convert a Python Unicode object to a C pointer to a NUL-terminated buffer
   of 16-bit Unicode (UTF-16) data.  As with ``s``, there is no need to
   provide storage for the Unicode data buffer; a pointer to the existing
   Unicode data is stored into the :c:type:`Py_UNICODE` pointer variable whose
   address you pass.

``u#`` (Unicode) [Py_UNICODE \*, int]
   This variant on ``u`` stores into two C variables, the first one a pointer
   to a Unicode data buffer, the second one its length. Non-Unicode objects
   are handled by interpreting their read-buffer pointer as pointer to a
   :c:type:`Py_UNICODE` array.

``es`` (string, Unicode or character buffer compatible object) [const char \*encoding, char \*\*buffer]
   This variant on ``s`` is used for encoding Unicode and objects convertible
   to Unicode into a character buffer. It only works for encoded data without
   embedded NUL bytes.

   This format requires two arguments.  The first is only used as input, and
   must be a :c:type:`const char\*` which points to the name of an encoding as
   a NUL-terminated string, or *NULL*, in which case the default encoding is
   used.  An exception is raised if the named encoding is not known to Python.
   The second argument must be a :c:type:`char\*\*`; the value of the pointer
   it references will be set to a buffer with the contents of the argument
   text.  The text will be encoded in the encoding specified by the first
   argument.

   :c:func:`PyArg_ParseTuple` will allocate a buffer of the needed size, copy
   the encoded data into this buffer and adjust *\*buffer* to reference the
   newly allocated storage.  The caller is responsible for calling
   :c:func:`PyMem_Free` to free the allocated buffer after use.

``et`` (string, Unicode or character buffer compatible object) [const char \*encoding, char \*\*buffer]
   Same as ``es`` except that 8-bit string objects are passed through without
   recoding them.  Instead, the implementation assumes that the string object
   uses the encoding passed in as parameter.

``es#`` (string, Unicode or character buffer compatible object) [const char \*encoding, char \*\*buffer, int \*buffer_length]
   This variant on ``s#`` is used for encoding Unicode and objects convertible
   to Unicode into a character buffer.  Unlike the ``es`` format, this variant
   allows input data which contains NUL characters.

   It requires three arguments.  The first is only used as input, and must be
   a :c:type:`const char\*` which points to the name of an encoding as a
   NUL-terminated string, or *NULL*, in which case the default encoding is
   used.  An exception is raised if the named encoding is not known to Python.
   The second argument must be a :c:type:`char\*\*`; the value of the pointer
   it references will be set to a buffer with the contents of the argument
   text.  The text will be encoded in the encoding specified by the first
   argument.  The third argument must be a pointer to an integer; the
   referenced integer will be set to the number of bytes in the output buffer.

   There are two modes of operation:

   If *\*buffer* points a *NULL* pointer, the function will allocate a buffer
   of the needed size, copy the encoded data into this buffer and set
   *\*buffer* to reference the newly allocated storage.  The caller is
   responsible for calling :c:func:`PyMem_Free` to free the allocated buffer
   after usage.

   If *\*buffer* points to a non-*NULL* pointer (an already allocated buffer),
   :c:func:`PyArg_ParseTuple` will use this location as the buffer and
   interpret the initial value of *\*buffer_length* as the buffer size.  It
   will then copy the encoded data into the buffer and NUL-terminate it.  If
   the buffer is not large enough, a :exc:`TypeError` will be set.
   Note: starting from Python 3.6 a :exc:`ValueError` will be set.

   In both cases, *\*buffer_length* is set to the length of the encoded data
   without the trailing NUL byte.

``et#`` (string, Unicode or character buffer compatible object) [const char \*encoding, char \*\*buffer, int \*buffer_length]
   Same as ``es#`` except that string objects are passed through without
   recoding them. Instead, the implementation assumes that the string object
   uses the encoding passed in as parameter.

``b`` (integer) [unsigned char]
   Convert a nonnegative Python integer to an unsigned tiny int, stored in a C
   :c:type:`unsigned char`.

``B`` (integer) [unsigned char]
   Convert a Python integer to a tiny int without overflow checking, stored in
   a C :c:type:`unsigned char`.

   .. versionadded:: 2.3

``h`` (integer) [short int]
   Convert a Python integer to a C :c:type:`short int`.

``H`` (integer) [unsigned short int]
   Convert a Python integer to a C :c:type:`unsigned short int`, without
   overflow checking.

   .. versionadded:: 2.3

``i`` (integer) [int]
   Convert a Python integer to a plain C :c:type:`int`.

``I`` (integer) [unsigned int]
   Convert a Python integer to a C :c:type:`unsigned int`, without overflow
   checking.

   .. versionadded:: 2.3

``l`` (integer) [long int]
   Convert a Python integer to a C :c:type:`long int`.

``k`` (integer) [unsigned long]
   Convert a Python integer or long integer to a C :c:type:`unsigned long`
   without overflow checking.

   .. versionadded:: 2.3

``L`` (integer) [PY_LONG_LONG]
   Convert a Python integer to a C :c:type:`long long`.  This format is only
   available on platforms that support :c:type:`long long` (or :c:type:`_int64`
   on Windows).

``K`` (integer) [unsigned PY_LONG_LONG]
   Convert a Python integer or long integer to a C :c:type:`unsigned long long`
   without overflow checking.  This format is only available on platforms that
   support :c:type:`unsigned long long` (or :c:type:`unsigned _int64` on
   Windows).

   .. versionadded:: 2.3

``n`` (integer) [Py_ssize_t]
   Convert a Python integer or long integer to a C :c:type:`Py_ssize_t`.

   .. versionadded:: 2.5

``c`` (string of length 1) [char]
   Convert a Python character, represented as a string of length 1, to a C
   :c:type:`char`.

``f`` (float) [float]
   Convert a Python floating point number to a C :c:type:`float`.

``d`` (float) [double]
   Convert a Python floating point number to a C :c:type:`double`.

``D`` (complex) [Py_complex]
   Convert a Python complex number to a C :c:type:`Py_complex` structure.

``O`` (object) [PyObject \*]
   Store a Python object (without any conversion) in a C object pointer.  The
   C program thus receives the actual object that was passed.  The object's
   reference count is not increased.  The pointer stored is not *NULL*.

``O!`` (object) [*typeobject*, PyObject \*]
   Store a Python object in a C object pointer.  This is similar to ``O``, but
   takes two C arguments: the first is the address of a Python type object,
   the second is the address of the C variable (of type :c:type:`PyObject\*`)
   into which the object pointer is stored.  If the Python object does not
   have the required type, :exc:`TypeError` is raised.

``O&`` (object) [*converter*, *anything*]
   Convert a Python object to a C variable through a *converter* function.
   This takes two arguments: the first is a function, the second is the
   address of a C variable (of arbitrary type), converted to :c:type:`void \*`.
   The *converter* function in turn is called as follows::

      status = converter(object, address);

   where *object* is the Python object to be converted and *address* is the
   :c:type:`void\*` argument that was passed to the :c:func:`PyArg_Parse\*`
   function.  The returned *status* should be ``1`` for a successful
   conversion and ``0`` if the conversion has failed.  When the conversion
   fails, the *converter* function should raise an exception and leave the
   content of *address* unmodified.

``S`` (string) [PyStringObject \*]
   Like ``O`` but requires that the Python object is a string object.  Raises
   :exc:`TypeError` if the object is not a string object.  The C variable may
   also be declared as :c:type:`PyObject\*`.

``U`` (Unicode string) [PyUnicodeObject \*]
   Like ``O`` but requires that the Python object is a Unicode object.  Raises
   :exc:`TypeError` if the object is not a Unicode object.  The C variable may
   also be declared as :c:type:`PyObject\*`.

``t#`` (read-only character buffer) [char \*, int]
   Like ``s#``, but accepts any object which implements the read-only buffer
   interface.  The :c:type:`char\*` variable is set to point to the first byte
   of the buffer, and the :c:type:`int` is set to the length of the buffer.
   Only single-segment buffer objects are accepted; :exc:`TypeError` is raised
   for all others.

``w`` (read-write character buffer) [char \*]
   Similar to ``s``, but accepts any object which implements the read-write
   buffer interface.  The caller must determine the length of the buffer by
   other means, or use ``w#`` instead.  Only single-segment buffer objects are
   accepted; :exc:`TypeError` is raised for all others.

``w#`` (read-write character buffer) [char \*, Py_ssize_t]
   Like ``s#``, but accepts any object which implements the read-write buffer
   interface.  The :c:type:`char \*` variable is set to point to the first byte
   of the buffer, and the :c:type:`Py_ssize_t` is set to the length of the
   buffer.  Only single-segment buffer objects are accepted; :exc:`TypeError`
   is raised for all others.

``w*`` (read-write byte-oriented buffer) [Py_buffer]
   This is to ``w`` what ``s*`` is to ``s``.

   .. versionadded:: 2.6

``(items)`` (tuple) [*matching-items*]
   The object must be a Python sequence whose length is the number of format
   units in *items*.  The C arguments must correspond to the individual format
   units in *items*.  Format units for sequences may be nested.

   .. note::

      Prior to Python version 1.5.2, this format specifier only accepted a
      tuple containing the individual parameters, not an arbitrary sequence.
      Code which previously caused :exc:`TypeError` to be raised here may now
      proceed without an exception.  This is not expected to be a problem for
      existing code.

It is possible to pass Python long integers where integers are requested;
however no proper range checking is done --- the most significant bits are
silently truncated when the receiving field is too small to receive the value
(actually, the semantics are inherited from downcasts in C --- your mileage
may vary).

A few other characters have a meaning in a format string.  These may not occur
inside nested parentheses.  They are:

``|``
   Indicates that the remaining arguments in the Python argument list are
   optional.  The C variables corresponding to optional arguments should be
   initialized to their default value --- when an optional argument is not
   specified, :c:func:`PyArg_ParseTuple` does not touch the contents of the
   corresponding C variable(s).

``:``
   The list of format units ends here; the string after the colon is used as
   the function name in error messages (the "associated value" of the
   exception that :c:func:`PyArg_ParseTuple` raises).

``;``
   The list of format units ends here; the string after the semicolon is used
   as the error message *instead* of the default error message.  ``:`` and
   ``;`` mutually exclude each other.

Note that any Python object references which are provided to the caller are
*borrowed* references; do not decrement their reference count!

Additional arguments passed to these functions must be addresses of variables
whose type is determined by the format string; these are used to store values
from the input tuple.  There are a few cases, as described in the list of
format units above, where these parameters are used as input values; they
should match what is specified for the corresponding format unit in that case.

For the conversion to succeed, the *arg* object must match the format and the
format must be exhausted.  On success, the :c:func:`PyArg_Parse\*` functions
return true, otherwise they return false and raise an appropriate exception.
When the :c:func:`PyArg_Parse\*` functions fail due to conversion failure in
one of the format units, the variables at the addresses corresponding to that
and the following format units are left untouched.


.. c:function:: int PyArg_ParseTuple(PyObject *args, const char *format, ...)

   Parse the parameters of a function that takes only positional parameters
   into local variables.  Returns true on success; on failure, it returns
   false and raises the appropriate exception.


.. c:function:: int PyArg_VaParse(PyObject *args, const char *format, va_list vargs)

   Identical to :c:func:`PyArg_ParseTuple`, except that it accepts a va_list
   rather than a variable number of arguments.


.. c:function:: int PyArg_ParseTupleAndKeywords(PyObject *args, PyObject *kw, const char *format, char *keywords[], ...)

   Parse the parameters of a function that takes both positional and keyword
   parameters into local variables.  Returns true on success; on failure, it
   returns false and raises the appropriate exception.


.. c:function:: int PyArg_VaParseTupleAndKeywords(PyObject *args, PyObject *kw, const char *format, char *keywords[], va_list vargs)

   Identical to :c:func:`PyArg_ParseTupleAndKeywords`, except that it accepts a
   va_list rather than a variable number of arguments.


.. c:function:: int PyArg_Parse(PyObject *args, const char *format, ...)

   Function used to deconstruct the argument lists of "old-style" functions
   --- these are functions which use the :const:`METH_OLDARGS` parameter
   parsing method.  This is not recommended for use in parameter parsing in
   new code, and most code in the standard interpreter has been modified to no
   longer use this for that purpose.  It does remain a convenient way to
   decompose other tuples, however, and may continue to be used for that
   purpose.


.. c:function:: int PyArg_UnpackTuple(PyObject *args, const char *name, Py_ssize_t min, Py_ssize_t max, ...)

   A simpler form of parameter retrieval which does not use a format string to
   specify the types of the arguments.  Functions which use this method to
   retrieve their parameters should be declared as :const:`METH_VARARGS` in
   function or method tables.  The tuple containing the actual parameters
   should be passed as *args*; it must actually be a tuple.  The length of the
   tuple must be at least *min* and no more than *max*; *min* and *max* may be
   equal.  Additional arguments must be passed to the function, each of which
   should be a pointer to a :c:type:`PyObject\*` variable; these will be filled
   in with the values from *args*; they will contain borrowed references.  The
   variables which correspond to optional parameters not given by *args* will
   not be filled in; these should be initialized by the caller. This function
   returns true on success and false if *args* is not a tuple or contains the
   wrong number of elements; an exception will be set if there was a failure.

   This is an example of the use of this function, taken from the sources for
   the :mod:`_weakref` helper module for weak references::

      static PyObject *
      weakref_ref(PyObject *self, PyObject *args)
      {
          PyObject *object;
          PyObject *callback = NULL;
          PyObject *result = NULL;

          if (PyArg_UnpackTuple(args, "ref", 1, 2, &object, &callback)) {
              result = PyWeakref_NewRef(object, callback);
          }
          return result;
      }

   The call to :c:func:`PyArg_UnpackTuple` in this example is entirely
   equivalent to this call to :c:func:`PyArg_ParseTuple`::

      PyArg_ParseTuple(args, "O|O:ref", &object, &callback)

   .. versionadded:: 2.2

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *min* and *max*. This might
      require changes in your code for properly supporting 64-bit systems.


.. c:function:: PyObject* Py_BuildValue(const char *format, ...)

   Create a new value based on a format string similar to those accepted by
   the :c:func:`PyArg_Parse\*` family of functions and a sequence of values.
   Returns the value or *NULL* in the case of an error; an exception will be
   raised if *NULL* is returned.

   :c:func:`Py_BuildValue` does not always build a tuple.  It builds a tuple
   only if its format string contains two or more format units.  If the format
   string is empty, it returns ``None``; if it contains exactly one format
   unit, it returns whatever object is described by that format unit.  To
   force it to return a tuple of size 0 or one, parenthesize the format
   string.

   When memory buffers are passed as parameters to supply data to build
   objects, as for the ``s`` and ``s#`` formats, the required data is copied.
   Buffers provided by the caller are never referenced by the objects created
   by :c:func:`Py_BuildValue`.  In other words, if your code invokes
   :c:func:`malloc` and passes the allocated memory to :c:func:`Py_BuildValue`,
   your code is responsible for calling :c:func:`free` for that memory once
   :c:func:`Py_BuildValue` returns.

   In the following description, the quoted form is the format unit; the entry
   in (round) parentheses is the Python object type that the format unit will
   return; and the entry in [square] brackets is the type of the C value(s) to
   be passed.

   The characters space, tab, colon and comma are ignored in format strings
   (but not within format units such as ``s#``).  This can be used to make
   long format strings a tad more readable.

   ``s`` (string) [char \*]
      Convert a null-terminated C string to a Python object.  If the C string
      pointer is *NULL*, ``None`` is used.

   ``s#`` (string) [char \*, int]
      Convert a C string and its length to a Python object.  If the C string
      pointer is *NULL*, the length is ignored and ``None`` is returned.

   ``z`` (string or ``None``) [char \*]
      Same as ``s``.

   ``z#`` (string or ``None``) [char \*, int]
      Same as ``s#``.

   ``u`` (Unicode string) [Py_UNICODE \*]
      Convert a null-terminated buffer of Unicode (UCS-2 or UCS-4) data to a
      Python Unicode object.  If the Unicode buffer pointer is *NULL*,
      ``None`` is returned.

   ``u#`` (Unicode string) [Py_UNICODE \*, int]
      Convert a Unicode (UCS-2 or UCS-4) data buffer and its length to a
      Python Unicode object.   If the Unicode buffer pointer is *NULL*, the
      length is ignored and ``None`` is returned.

   ``i`` (integer) [int]
      Convert a plain C :c:type:`int` to a Python integer object.

   ``b`` (integer) [char]
      Convert a plain C :c:type:`char` to a Python integer object.

   ``h`` (integer) [short int]
      Convert a plain C :c:type:`short int` to a Python integer object.

   ``l`` (integer) [long int]
      Convert a C :c:type:`long int` to a Python integer object.

   ``B`` (integer) [unsigned char]
      Convert a C :c:type:`unsigned char` to a Python integer object.

   ``H`` (integer) [unsigned short int]
      Convert a C :c:type:`unsigned short int` to a Python integer object.

   ``I`` (integer/long) [unsigned int]
      Convert a C :c:type:`unsigned int` to a Python integer object or a Python
      long integer object, if it is larger than ``sys.maxint``.

   ``k`` (integer/long) [unsigned long]
      Convert a C :c:type:`unsigned long` to a Python integer object or a
      Python long integer object, if it is larger than ``sys.maxint``.

   ``L`` (long) [PY_LONG_LONG]
      Convert a C :c:type:`long long` to a Python long integer object. Only
      available on platforms that support :c:type:`long long`.

   ``K`` (long) [unsigned PY_LONG_LONG]
      Convert a C :c:type:`unsigned long long` to a Python long integer object.
      Only available on platforms that support :c:type:`unsigned long long`.

   ``n`` (int) [Py_ssize_t]
      Convert a C :c:type:`Py_ssize_t` to a Python integer or long integer.

      .. versionadded:: 2.5

   ``c`` (string of length 1) [char]
      Convert a C :c:type:`int` representing a character to a Python string of
      length 1.

   ``d`` (float) [double]
      Convert a C :c:type:`double` to a Python floating point number.

   ``f`` (float) [float]
      Same as ``d``.

   ``D`` (complex) [Py_complex \*]
      Convert a C :c:type:`Py_complex` structure to a Python complex number.

   ``O`` (object) [PyObject \*]
      Pass a Python object untouched (except for its reference count, which is
      incremented by one).  If the object passed in is a *NULL* pointer, it is
      assumed that this was caused because the call producing the argument
      found an error and set an exception. Therefore, :c:func:`Py_BuildValue`
      will return *NULL* but won't raise an exception.  If no exception has
      been raised yet, :exc:`SystemError` is set.

   ``S`` (object) [PyObject \*]
      Same as ``O``.

   ``N`` (object) [PyObject \*]
      Same as ``O``, except it doesn't increment the reference count on the
      object.  Useful when the object is created by a call to an object
      constructor in the argument list.

   ``O&`` (object) [*converter*, *anything*]
      Convert *anything* to a Python object through a *converter* function.
      The function is called with *anything* (which should be compatible with
      :c:type:`void \*`) as its argument and should return a "new" Python
      object, or *NULL* if an error occurred.

   ``(items)`` (tuple) [*matching-items*]
      Convert a sequence of C values to a Python tuple with the same number of
      items.

   ``[items]`` (list) [*matching-items*]
      Convert a sequence of C values to a Python list with the same number of
      items.

   ``{items}`` (dictionary) [*matching-items*]
      Convert a sequence of C values to a Python dictionary.  Each pair of
      consecutive C values adds one item to the dictionary, serving as key and
      value, respectively.

   If there is an error in the format string, the :exc:`SystemError` exception
   is set and *NULL* returned.

.. c:function:: PyObject* Py_VaBuildValue(const char *format, va_list vargs)

   Identical to :c:func:`Py_BuildValue`, except that it accepts a va_list
   rather than a variable number of arguments.
