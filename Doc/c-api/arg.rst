.. highlight:: c

.. _arg-parsing:

Parsing arguments and building values
=====================================

These functions are useful when creating your own extension functions and
methods.  Additional information and examples are available in
:ref:`extending-index`.

The first three of these functions described, :c:func:`PyArg_ParseTuple`,
:c:func:`PyArg_ParseTupleAndKeywords`, and :c:func:`PyArg_Parse`, all use *format
strings* which are used to tell the function about the expected arguments.  The
format strings use the same syntax for each of these functions.

-----------------
Parsing arguments
-----------------

A format string consists of zero or more "format units."  A format unit
describes one Python object; it is usually a single character or a parenthesized
sequence of format units.  With a few exceptions, a format unit that is not a
parenthesized sequence normally corresponds to a single address argument to
these functions.  In the following description, the quoted form is the format
unit; the entry in (round) parentheses is the Python object type that matches
the format unit; and the entry in [square] brackets is the type of the C
variable(s) whose address should be passed.

.. _arg-parsing-string-and-buffers:

Strings and buffers
-------------------

.. note::

   On Python 3.12 and older, the macro :c:macro:`!PY_SSIZE_T_CLEAN` must be
   defined before including :file:`Python.h` to use all ``#`` variants of
   formats (``s#``, ``y#``, etc.) explained below.
   This is not necessary on Python 3.13 and later.

These formats allow accessing an object as a contiguous chunk of memory.
You don't have to provide raw storage for the returned unicode or bytes
area.

Unless otherwise stated, buffers are not NUL-terminated.

There are three ways strings and buffers can be converted to C:

*  Formats such as ``y*`` and ``s*`` fill a :c:type:`Py_buffer` structure.
   This locks the underlying buffer so that the caller can subsequently use
   the buffer even inside a :c:type:`Py_BEGIN_ALLOW_THREADS`
   block without the risk of mutable data being resized or destroyed.
   As a result, **you have to call** :c:func:`PyBuffer_Release` after you have
   finished processing the data (or in any early abort case).

*  The ``es``, ``es#``, ``et`` and ``et#`` formats allocate the result buffer.
   **You have to call** :c:func:`PyMem_Free` after you have finished
   processing the data (or in any early abort case).

*  .. _c-arg-borrowed-buffer:

   Other formats take a :class:`str` or a read-only :term:`bytes-like object`,
   such as :class:`bytes`, and provide a ``const char *`` pointer to
   its buffer.
   In this case the buffer is "borrowed": it is managed by the corresponding
   Python object, and shares the lifetime of this object.
   You won't have to release any memory yourself.

   To ensure that the underlying buffer may be safely borrowed, the object's
   :c:member:`PyBufferProcs.bf_releasebuffer` field must be ``NULL``.
   This disallows common mutable objects such as :class:`bytearray`,
   but also some read-only objects such as :class:`memoryview` of
   :class:`bytes`.

   Besides this ``bf_releasebuffer`` requirement, there is no check to verify
   whether the input object is immutable (e.g. whether it would honor a request
   for a writable buffer, or whether another thread can mutate the data).

``s`` (:class:`str`) [const char \*]
   Convert a Unicode object to a C pointer to a character string.
   A pointer to an existing string is stored in the character pointer
   variable whose address you pass.  The C string is NUL-terminated.
   The Python string must not contain embedded null code points; if it does,
   a :exc:`ValueError` exception is raised. Unicode objects are converted
   to C strings using ``'utf-8'`` encoding. If this conversion fails, a
   :exc:`UnicodeError` is raised.

   .. note::
      This format does not accept :term:`bytes-like objects
      <bytes-like object>`.  If you want to accept
      filesystem paths and convert them to C character strings, it is
      preferable to use the ``O&`` format with :c:func:`PyUnicode_FSConverter`
      as *converter*.

   .. versionchanged:: 3.5
      Previously, :exc:`TypeError` was raised when embedded null code points
      were encountered in the Python string.

``s*`` (:class:`str` or :term:`bytes-like object`) [Py_buffer]
   This format accepts Unicode objects as well as bytes-like objects.
   It fills a :c:type:`Py_buffer` structure provided by the caller.
   In this case the resulting C string may contain embedded NUL bytes.
   Unicode objects are converted to C strings using ``'utf-8'`` encoding.

``s#`` (:class:`str`, read-only :term:`bytes-like object`) [const char \*, :c:type:`Py_ssize_t`]
   Like ``s*``, except that it provides a :ref:`borrowed buffer <c-arg-borrowed-buffer>`.
   The result is stored into two C variables,
   the first one a pointer to a C string, the second one its length.
   The string may contain embedded null bytes. Unicode objects are converted
   to C strings using ``'utf-8'`` encoding.

``z`` (:class:`str` or ``None``) [const char \*]
   Like ``s``, but the Python object may also be ``None``, in which case the C
   pointer is set to ``NULL``.
   It is the same as ``s?`` with the C pointer was initialized to ``NULL``.

``z*`` (:class:`str`, :term:`bytes-like object` or ``None``) [Py_buffer]
   Like ``s*``, but the Python object may also be ``None``, in which case the
   ``buf`` member of the :c:type:`Py_buffer` structure is set to ``NULL``.
   It is the same as ``s*?`` with the ``buf`` member of the :c:type:`Py_buffer`
   structure was initialized to ``NULL``.

``z#`` (:class:`str`, read-only :term:`bytes-like object` or ``None``) [const char \*, :c:type:`Py_ssize_t`]
   Like ``s#``, but the Python object may also be ``None``, in which case the C
   pointer is set to ``NULL``.
   It is the same as ``s#?`` with the C pointer was initialized to ``NULL``.

``y`` (read-only :term:`bytes-like object`) [const char \*]
   This format converts a bytes-like object to a C pointer to a
   :ref:`borrowed <c-arg-borrowed-buffer>` character string;
   it does not accept Unicode objects.  The bytes buffer must not
   contain embedded null bytes; if it does, a :exc:`ValueError`
   exception is raised.

   .. versionchanged:: 3.5
      Previously, :exc:`TypeError` was raised when embedded null bytes were
      encountered in the bytes buffer.

``y*`` (:term:`bytes-like object`) [Py_buffer]
   This variant on ``s*`` doesn't accept Unicode objects, only
   bytes-like objects.  **This is the recommended way to accept
   binary data.**

``y#`` (read-only :term:`bytes-like object`) [const char \*, :c:type:`Py_ssize_t`]
   This variant on ``s#`` doesn't accept Unicode objects, only bytes-like
   objects.

``S`` (:class:`bytes`) [PyBytesObject \*]
   Requires that the Python object is a :class:`bytes` object, without
   attempting any conversion.  Raises :exc:`TypeError` if the object is not
   a bytes object.  The C variable may also be declared as :c:expr:`PyObject*`.

``Y`` (:class:`bytearray`) [PyByteArrayObject \*]
   Requires that the Python object is a :class:`bytearray` object, without
   attempting any conversion.  Raises :exc:`TypeError` if the object is not
   a :class:`bytearray` object. The C variable may also be declared as :c:expr:`PyObject*`.

``U`` (:class:`str`) [PyObject \*]
   Requires that the Python object is a Unicode object, without attempting
   any conversion.  Raises :exc:`TypeError` if the object is not a Unicode
   object.  The C variable may also be declared as :c:expr:`PyObject*`.

``w*`` (read-write :term:`bytes-like object`) [Py_buffer]
   This format accepts any object which implements the read-write buffer
   interface. It fills a :c:type:`Py_buffer` structure provided by the caller.
   The buffer may contain embedded null bytes. The caller have to call
   :c:func:`PyBuffer_Release` when it is done with the buffer.

``es`` (:class:`str`) [const char \*encoding, char \*\*buffer]
   This variant on ``s`` is used for encoding Unicode into a character buffer.
   It only works for encoded data without embedded NUL bytes.

   This format requires two arguments.  The first is only used as input, and
   must be a :c:expr:`const char*` which points to the name of an encoding as a
   NUL-terminated string, or ``NULL``, in which case ``'utf-8'`` encoding is used.
   An exception is raised if the named encoding is not known to Python.  The
   second argument must be a :c:expr:`char**`; the value of the pointer it
   references will be set to a buffer with the contents of the argument text.
   The text will be encoded in the encoding specified by the first argument.

   :c:func:`PyArg_ParseTuple` will allocate a buffer of the needed size, copy the
   encoded data into this buffer and adjust *\*buffer* to reference the newly
   allocated storage.  The caller is responsible for calling :c:func:`PyMem_Free` to
   free the allocated buffer after use.

``et`` (:class:`str`, :class:`bytes` or :class:`bytearray`) [const char \*encoding, char \*\*buffer]
   Same as ``es`` except that byte string objects are passed through without
   recoding them.  Instead, the implementation assumes that the byte string object uses
   the encoding passed in as parameter.

``es#`` (:class:`str`) [const char \*encoding, char \*\*buffer, :c:type:`Py_ssize_t` \*buffer_length]
   This variant on ``s#`` is used for encoding Unicode into a character buffer.
   Unlike the ``es`` format, this variant allows input data which contains NUL
   characters.

   It requires three arguments.  The first is only used as input, and must be a
   :c:expr:`const char*` which points to the name of an encoding as a
   NUL-terminated string, or ``NULL``, in which case ``'utf-8'`` encoding is used.
   An exception is raised if the named encoding is not known to Python.  The
   second argument must be a :c:expr:`char**`; the value of the pointer it
   references will be set to a buffer with the contents of the argument text.
   The text will be encoded in the encoding specified by the first argument.
   The third argument must be a pointer to an integer; the referenced integer
   will be set to the number of bytes in the output buffer.

   There are two modes of operation:

   If *\*buffer* points a ``NULL`` pointer, the function will allocate a buffer of
   the needed size, copy the encoded data into this buffer and set *\*buffer* to
   reference the newly allocated storage.  The caller is responsible for calling
   :c:func:`PyMem_Free` to free the allocated buffer after usage.

   If *\*buffer* points to a non-``NULL`` pointer (an already allocated buffer),
   :c:func:`PyArg_ParseTuple` will use this location as the buffer and interpret the
   initial value of *\*buffer_length* as the buffer size.  It will then copy the
   encoded data into the buffer and NUL-terminate it.  If the buffer is not large
   enough, a :exc:`ValueError` will be set.

   In both cases, *\*buffer_length* is set to the length of the encoded data
   without the trailing NUL byte.

``et#`` (:class:`str`, :class:`bytes` or :class:`bytearray`) [const char \*encoding, char \*\*buffer, :c:type:`Py_ssize_t` \*buffer_length]
   Same as ``es#`` except that byte string objects are passed through without recoding
   them. Instead, the implementation assumes that the byte string object uses the
   encoding passed in as parameter.

.. versionchanged:: 3.12
   ``u``, ``u#``, ``Z``, and ``Z#`` are removed because they used a legacy
   ``Py_UNICODE*`` representation.


Numbers
-------

These formats allow representing Python numbers or single characters as C numbers.
Formats that require :class:`int`, :class:`float` or :class:`complex` can
also use the corresponding special methods :meth:`~object.__index__`,
:meth:`~object.__float__` or :meth:`~object.__complex__` to convert
the Python object to the required type.

For signed integer formats, :exc:`OverflowError` is raised if the value
is out of range for the C type.
For unsigned integer formats, no range checking is done --- the
most significant bits are silently truncated when the receiving field is too
small to receive the value.

``b`` (:class:`int`) [unsigned char]
   Convert a nonnegative Python integer to an unsigned tiny integer, stored in a C
   :c:expr:`unsigned char`.

``B`` (:class:`int`) [unsigned char]
   Convert a Python integer to a tiny integer without overflow checking, stored in a C
   :c:expr:`unsigned char`.

``h`` (:class:`int`) [short int]
   Convert a Python integer to a C :c:expr:`short int`.

``H`` (:class:`int`) [unsigned short int]
   Convert a Python integer to a C :c:expr:`unsigned short int`, without overflow
   checking.

``i`` (:class:`int`) [int]
   Convert a Python integer to a plain C :c:expr:`int`.

``I`` (:class:`int`) [unsigned int]
   Convert a Python integer to a C :c:expr:`unsigned int`, without overflow
   checking.

``l`` (:class:`int`) [long int]
   Convert a Python integer to a C :c:expr:`long int`.

``k`` (:class:`int`) [unsigned long]
   Convert a Python integer to a C :c:expr:`unsigned long` without
   overflow checking.

   .. versionchanged:: 3.14
      Use :meth:`~object.__index__` if available.

``L`` (:class:`int`) [long long]
   Convert a Python integer to a C :c:expr:`long long`.

``K`` (:class:`int`) [unsigned long long]
   Convert a Python integer to a C :c:expr:`unsigned long long`
   without overflow checking.

   .. versionchanged:: 3.14
      Use :meth:`~object.__index__` if available.

``n`` (:class:`int`) [:c:type:`Py_ssize_t`]
   Convert a Python integer to a C :c:type:`Py_ssize_t`.

``c`` (:class:`bytes` or :class:`bytearray` of length 1) [char]
   Convert a Python byte, represented as a :class:`bytes` or
   :class:`bytearray` object of length 1, to a C :c:expr:`char`.

   .. versionchanged:: 3.3
      Allow :class:`bytearray` objects.

``C`` (:class:`str` of length 1) [int]
   Convert a Python character, represented as a :class:`str` object of
   length 1, to a C :c:expr:`int`.

``f`` (:class:`float`) [float]
   Convert a Python floating-point number to a C :c:expr:`float`.

``d`` (:class:`float`) [double]
   Convert a Python floating-point number to a C :c:expr:`double`.

``D`` (:class:`complex`) [Py_complex]
   Convert a Python complex number to a C :c:type:`Py_complex` structure.

Other objects
-------------

``O`` (object) [PyObject \*]
   Store a Python object (without any conversion) in a C object pointer.  The C
   program thus receives the actual object that was passed.  A new
   :term:`strong reference` to the object is not created
   (i.e. its reference count is not increased).
   The pointer stored is not ``NULL``.

``O!`` (object) [*typeobject*, PyObject \*]
   Store a Python object in a C object pointer.  This is similar to ``O``, but
   takes two C arguments: the first is the address of a Python type object, the
   second is the address of the C variable (of type :c:expr:`PyObject*`) into which
   the object pointer is stored.  If the Python object does not have the required
   type, :exc:`TypeError` is raised.

.. _o_ampersand:

``O&`` (object) [*converter*, *address*]
   Convert a Python object to a C variable through a *converter* function.  This
   takes two arguments: the first is a function, the second is the address of a C
   variable (of arbitrary type), converted to :c:expr:`void *`.  The *converter*
   function in turn is called as follows::

      status = converter(object, address);

   where *object* is the Python object to be converted and *address* is the
   :c:expr:`void*` argument that was passed to the ``PyArg_Parse*`` function.
   The returned *status* should be ``1`` for a successful conversion and ``0`` if
   the conversion has failed.  When the conversion fails, the *converter* function
   should raise an exception and leave the content of *address* unmodified.

   .. c:macro:: Py_CLEANUP_SUPPORTED
      :no-typesetting:

   If the *converter* returns :c:macro:`!Py_CLEANUP_SUPPORTED`, it may get called a
   second time if the argument parsing eventually fails, giving the converter a
   chance to release any memory that it had already allocated. In this second
   call, the *object* parameter will be ``NULL``; *address* will have the same value
   as in the original call.

   Examples of converters: :c:func:`PyUnicode_FSConverter` and
   :c:func:`PyUnicode_FSDecoder`.

   .. versionchanged:: 3.1
      :c:macro:`!Py_CLEANUP_SUPPORTED` was added.

``p`` (:class:`bool`) [int]
   Tests the value passed in for truth (a boolean **p**\ redicate) and converts
   the result to its equivalent C true/false integer value.
   Sets the int to ``1`` if the expression was true and ``0`` if it was false.
   This accepts any valid Python value.  See :ref:`truth` for more
   information about how Python tests values for truth.

   .. versionadded:: 3.3

``(items)`` (sequence) [*matching-items*]
   The object must be a Python sequence (except :class:`str`, :class:`bytes`
   or :class:`bytearray`) whose length is the number of format units
   in *items*.  The C arguments must correspond to the individual format units in
   *items*.  Format units for sequences may be nested.

   If *items* contains format units which store a :ref:`borrowed buffer
   <c-arg-borrowed-buffer>` (``s``, ``s#``, ``z``, ``z#``, ``y``, or ``y#``)
   or a :term:`borrowed reference` (``S``, ``Y``, ``U``, ``O``, or ``O!``),
   the object must be a Python tuple.
   The *converter* for the ``O&`` format unit in *items* must not store
   a borrowed buffer or a borrowed reference.

   .. versionchanged:: 3.14
      :class:`str` and :class:`bytearray` objects no longer accepted as a sequence.

   .. deprecated:: 3.14
      Non-tuple sequences are deprecated if *items* contains format units
      which store a borrowed buffer or a borrowed reference.

``unit?`` (anything or ``None``) [*matching-variable(s)*]
   ``?`` modifies the behavior of the preceding format unit.
   The C variable(s) corresponding to that parameter should be initialized
   to their default value --- when the argument is ``None``,
   :c:func:`PyArg_ParseTuple` does not touch the contents of the corresponding
   C variable(s).
   If the argument is not ``None``, it is parsed according to the specified
   format unit.

   .. versionadded:: 3.14

A few other characters have a meaning in a format string.  These may not occur
inside nested parentheses.  They are:

``|``
   Indicates that the remaining arguments in the Python argument list are optional.
   The C variables corresponding to optional arguments should be initialized to
   their default value --- when an optional argument is not specified,
   :c:func:`PyArg_ParseTuple` does not touch the contents of the corresponding C
   variable(s).

``$``
   :c:func:`PyArg_ParseTupleAndKeywords` only:
   Indicates that the remaining arguments in the Python argument list are
   keyword-only.  Currently, all keyword-only arguments must also be optional
   arguments, so ``|`` must always be specified before ``$`` in the format
   string.

   .. versionadded:: 3.3

``:``
   The list of format units ends here; the string after the colon is used as the
   function name in error messages (the "associated value" of the exception that
   :c:func:`PyArg_ParseTuple` raises).

``;``
   The list of format units ends here; the string after the semicolon is used as
   the error message *instead* of the default error message.  ``:`` and ``;``
   mutually exclude each other.

Note that any Python object references which are provided to the caller are
*borrowed* references; do not release them
(i.e. do not decrement their reference count)!

Additional arguments passed to these functions must be addresses of variables
whose type is determined by the format string; these are used to store values
from the input tuple.  There are a few cases, as described in the list of format
units above, where these parameters are used as input values; they should match
what is specified for the corresponding format unit in that case.

For the conversion to succeed, the *arg* object must match the format
and the format must be exhausted.  On success, the
``PyArg_Parse*`` functions return true, otherwise they return
false and raise an appropriate exception. When the
``PyArg_Parse*`` functions fail due to conversion failure in one
of the format units, the variables at the addresses corresponding to that
and the following format units are left untouched.

API Functions
-------------

.. c:function:: int PyArg_ParseTuple(PyObject *args, const char *format, ...)

   Parse the parameters of a function that takes only positional parameters into
   local variables.  Returns true on success; on failure, it returns false and
   raises the appropriate exception.


.. c:function:: int PyArg_VaParse(PyObject *args, const char *format, va_list vargs)

   Identical to :c:func:`PyArg_ParseTuple`, except that it accepts a va_list rather
   than a variable number of arguments.


.. c:function:: int PyArg_ParseTupleAndKeywords(PyObject *args, PyObject *kw, const char *format, char * const *keywords, ...)

   Parse the parameters of a function that takes both positional and keyword
   parameters into local variables.
   The *keywords* argument is a ``NULL``-terminated array of keyword parameter
   names specified as null-terminated ASCII or UTF-8 encoded C strings.
   Empty names denote
   :ref:`positional-only parameters <positional-only_parameter>`.
   Returns true on success; on failure, it returns false and raises the
   appropriate exception.

   .. note::

      The *keywords* parameter declaration is :c:expr:`char * const *` in C and
      :c:expr:`const char * const *` in C++.
      This can be overridden with the :c:macro:`PY_CXX_CONST` macro.

   .. versionchanged:: 3.6
      Added support for :ref:`positional-only parameters
      <positional-only_parameter>`.

   .. versionchanged:: 3.13
      The *keywords* parameter has now type :c:expr:`char * const *` in C and
      :c:expr:`const char * const *` in C++, instead of :c:expr:`char **`.
      Added support for non-ASCII keyword parameter names.



.. c:function:: int PyArg_VaParseTupleAndKeywords(PyObject *args, PyObject *kw, const char *format, char * const *keywords, va_list vargs)

   Identical to :c:func:`PyArg_ParseTupleAndKeywords`, except that it accepts a
   va_list rather than a variable number of arguments.


.. c:function:: int PyArg_ValidateKeywordArguments(PyObject *)

   Ensure that the keys in the keywords argument dictionary are strings.  This
   is only needed if :c:func:`PyArg_ParseTupleAndKeywords` is not used, since the
   latter already does this check.

   .. versionadded:: 3.2


.. c:function:: int PyArg_Parse(PyObject *args, const char *format, ...)

   Parse the parameter of a function that takes a single positional parameter
   into a local variable.  Returns true on success; on failure, it returns
   false and raises the appropriate exception.

   Example::

       // Function using METH_O calling convention
       static PyObject*
       my_function(PyObject *module, PyObject *arg)
       {
           int value;
           if (!PyArg_Parse(arg, "i:my_function", &value)) {
               return NULL;
           }
           // ... use value ...
       }


.. c:function:: int PyArg_UnpackTuple(PyObject *args, const char *name, Py_ssize_t min, Py_ssize_t max, ...)

   A simpler form of parameter retrieval which does not use a format string to
   specify the types of the arguments.  Functions which use this method to retrieve
   their parameters should be declared as :c:macro:`METH_VARARGS` in function or
   method tables.  The tuple containing the actual parameters should be passed as
   *args*; it must actually be a tuple.  The length of the tuple must be at least
   *min* and no more than *max*; *min* and *max* may be equal.  Additional
   arguments must be passed to the function, each of which should be a pointer to a
   :c:expr:`PyObject*` variable; these will be filled in with the values from
   *args*; they will contain :term:`borrowed references <borrowed reference>`.
   The variables which correspond
   to optional parameters not given by *args* will not be filled in; these should
   be initialized by the caller. This function returns true on success and false if
   *args* is not a tuple or contains the wrong number of elements; an exception
   will be set if there was a failure.

   This is an example of the use of this function, taken from the sources for the
   :mod:`!_weakref` helper module for weak references::

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

   The call to :c:func:`PyArg_UnpackTuple` in this example is entirely equivalent to
   this call to :c:func:`PyArg_ParseTuple`::

      PyArg_ParseTuple(args, "O|O:ref", &object, &callback)

.. c:macro:: PY_CXX_CONST

   The value to be inserted, if any, before :c:expr:`char * const *`
   in the *keywords* parameter declaration of
   :c:func:`PyArg_ParseTupleAndKeywords` and
   :c:func:`PyArg_VaParseTupleAndKeywords`.
   Default empty for C and ``const`` for C++
   (:c:expr:`const char * const *`).
   To override, define it to the desired value before including
   :file:`Python.h`.

   .. versionadded:: 3.13


---------------
Building values
---------------

.. c:function:: PyObject* Py_BuildValue(const char *format, ...)

   Create a new value based on a format string similar to those accepted by the
   ``PyArg_Parse*`` family of functions and a sequence of values.  Returns
   the value or ``NULL`` in the case of an error; an exception will be raised if
   ``NULL`` is returned.

   :c:func:`Py_BuildValue` does not always build a tuple.  It builds a tuple only if
   its format string contains two or more format units.  If the format string is
   empty, it returns ``None``; if it contains exactly one format unit, it returns
   whatever object is described by that format unit.  To force it to return a tuple
   of size 0 or one, parenthesize the format string.

   When memory buffers are passed as parameters to supply data to build objects, as
   for the ``s`` and ``s#`` formats, the required data is copied.  Buffers provided
   by the caller are never referenced by the objects created by
   :c:func:`Py_BuildValue`.  In other words, if your code invokes :c:func:`malloc`
   and passes the allocated memory to :c:func:`Py_BuildValue`, your code is
   responsible for calling :c:func:`free` for that memory once
   :c:func:`Py_BuildValue` returns.

   In the following description, the quoted form is the format unit; the entry in
   (round) parentheses is the Python object type that the format unit will return;
   and the entry in [square] brackets is the type of the C value(s) to be passed.

   The characters space, tab, colon and comma are ignored in format strings (but
   not within format units such as ``s#``).  This can be used to make long format
   strings a tad more readable.

   ``s`` (:class:`str` or ``None``) [const char \*]
      Convert a null-terminated C string to a Python :class:`str` object using ``'utf-8'``
      encoding. If the C string pointer is ``NULL``, ``None`` is used.

   ``s#`` (:class:`str` or ``None``) [const char \*, :c:type:`Py_ssize_t`]
      Convert a C string and its length to a Python :class:`str` object using ``'utf-8'``
      encoding. If the C string pointer is ``NULL``, the length is ignored and
      ``None`` is returned.

   ``y`` (:class:`bytes`) [const char \*]
      This converts a C string to a Python :class:`bytes` object.  If the C
      string pointer is ``NULL``, ``None`` is returned.

   ``y#`` (:class:`bytes`) [const char \*, :c:type:`Py_ssize_t`]
      This converts a C string and its lengths to a Python object.  If the C
      string pointer is ``NULL``, ``None`` is returned.

   ``z`` (:class:`str` or ``None``) [const char \*]
      Same as ``s``.

   ``z#`` (:class:`str` or ``None``) [const char \*, :c:type:`Py_ssize_t`]
      Same as ``s#``.

   ``u`` (:class:`str`) [const wchar_t \*]
      Convert a null-terminated :c:type:`wchar_t` buffer of Unicode (UTF-16 or UCS-4)
      data to a Python Unicode object.  If the Unicode buffer pointer is ``NULL``,
      ``None`` is returned.

   ``u#`` (:class:`str`) [const wchar_t \*, :c:type:`Py_ssize_t`]
      Convert a Unicode (UTF-16 or UCS-4) data buffer and its length to a Python
      Unicode object.   If the Unicode buffer pointer is ``NULL``, the length is ignored
      and ``None`` is returned.

   ``U`` (:class:`str` or ``None``) [const char \*]
      Same as ``s``.

   ``U#`` (:class:`str` or ``None``) [const char \*, :c:type:`Py_ssize_t`]
      Same as ``s#``.

   ``i`` (:class:`int`) [int]
      Convert a plain C :c:expr:`int` to a Python integer object.

   ``b`` (:class:`int`) [char]
      Convert a plain C :c:expr:`char` to a Python integer object.

   ``h`` (:class:`int`) [short int]
      Convert a plain C :c:expr:`short int` to a Python integer object.

   ``l`` (:class:`int`) [long int]
      Convert a C :c:expr:`long int` to a Python integer object.

   ``B`` (:class:`int`) [unsigned char]
      Convert a C :c:expr:`unsigned char` to a Python integer object.

   ``H`` (:class:`int`) [unsigned short int]
      Convert a C :c:expr:`unsigned short int` to a Python integer object.

   ``I`` (:class:`int`) [unsigned int]
      Convert a C :c:expr:`unsigned int` to a Python integer object.

   ``k`` (:class:`int`) [unsigned long]
      Convert a C :c:expr:`unsigned long` to a Python integer object.

   ``L`` (:class:`int`) [long long]
      Convert a C :c:expr:`long long` to a Python integer object.

   .. _capi-py-buildvalue-format-K:

   ``K`` (:class:`int`) [unsigned long long]
      Convert a C :c:expr:`unsigned long long` to a Python integer object.

   ``n`` (:class:`int`) [:c:type:`Py_ssize_t`]
      Convert a C :c:type:`Py_ssize_t` to a Python integer.

   ``p`` (:class:`bool`) [int]
      Convert a C :c:expr:`int` to a Python :class:`bool` object.

      Be aware that this format requires an ``int`` argument.
      Unlike most other contexts in C, variadic arguments are not coerced to
      a suitable type automatically.
      You can convert another type (for example, a pointer or a float) to a
      suitable ``int`` value using ``(x) ? 1 : 0`` or ``!!x``.

      .. versionadded:: 3.14

   ``c`` (:class:`bytes` of length 1) [char]
      Convert a C :c:expr:`int` representing a byte to a Python :class:`bytes` object of
      length 1.

   ``C`` (:class:`str` of length 1) [int]
      Convert a C :c:expr:`int` representing a character to Python :class:`str`
      object of length 1.

   ``d`` (:class:`float`) [double]
      Convert a C :c:expr:`double` to a Python floating-point number.

   ``f`` (:class:`float`) [float]
      Convert a C :c:expr:`float` to a Python floating-point number.

   ``D`` (:class:`complex`) [Py_complex \*]
      Convert a C :c:type:`Py_complex` structure to a Python complex number.

   ``O`` (object) [PyObject \*]
      Pass a Python object untouched but create a new
      :term:`strong reference` to it
      (i.e. its reference count is incremented by one).
      If the object passed in is a ``NULL`` pointer, it is assumed
      that this was caused because the call producing the argument found an error and
      set an exception. Therefore, :c:func:`Py_BuildValue` will return ``NULL`` but won't
      raise an exception.  If no exception has been raised yet, :exc:`SystemError` is
      set.

   ``S`` (object) [PyObject \*]
      Same as ``O``.

   ``N`` (object) [PyObject \*]
      Same as ``O``, except it doesn't create a new :term:`strong reference`.
      Useful when the object is created by a call to an object constructor in the
      argument list.

   ``O&`` (object) [*converter*, *anything*]
      Convert *anything* to a Python object through a *converter* function.  The
      function is called with *anything* (which should be compatible with :c:expr:`void*`)
      as its argument and should return a "new" Python object, or ``NULL`` if an
      error occurred.

   ``(items)`` (:class:`tuple`) [*matching-items*]
      Convert a sequence of C values to a Python tuple with the same number of items.

   ``[items]`` (:class:`list`) [*matching-items*]
      Convert a sequence of C values to a Python list with the same number of items.

   ``{items}`` (:class:`dict`) [*matching-items*]
      Convert a sequence of C values to a Python dictionary.  Each pair of consecutive
      C values adds one item to the dictionary, serving as key and value,
      respectively.

   If there is an error in the format string, the :exc:`SystemError` exception is
   set and ``NULL`` returned.

.. c:function:: PyObject* Py_VaBuildValue(const char *format, va_list vargs)

   Identical to :c:func:`Py_BuildValue`, except that it accepts a va_list
   rather than a variable number of arguments.
