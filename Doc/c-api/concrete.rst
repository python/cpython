.. highlightlang:: c


.. _concrete:

**********************
Concrete Objects Layer
**********************

The functions in this chapter are specific to certain Python object types.
Passing them an object of the wrong type is not a good idea; if you receive an
object from a Python program and you are not sure that it has the right type,
you must perform a type check first; for example, to check that an object is a
dictionary, use :cfunc:`PyDict_Check`.  The chapter is structured like the
"family tree" of Python object types.

.. warning::

   While the functions described in this chapter carefully check the type of the
   objects which are passed in, many of them do not check for *NULL* being passed
   instead of a valid object.  Allowing *NULL* to be passed in can cause memory
   access violations and immediate termination of the interpreter.


.. _fundamental:

Fundamental Objects
===================

This section describes Python type objects and the singleton object ``None``.


.. _typeobjects:

Type Objects
------------

.. index:: object: type


.. ctype:: PyTypeObject

   The C structure of the objects used to describe built-in types.


.. cvar:: PyObject* PyType_Type

   .. index:: single: TypeType (in module types)

   This is the type object for type objects; it is the same object as ``type`` and
   ``types.TypeType`` in the Python layer.


.. cfunction:: int PyType_Check(PyObject *o)

   Return true if the object *o* is a type object, including instances of types
   derived from the standard type object.  Return false in all other cases.


.. cfunction:: int PyType_CheckExact(PyObject *o)

   Return true if the object *o* is a type object, but not a subtype of the
   standard type object.  Return false in all other cases.

   .. versionadded:: 2.2


.. cfunction:: int PyType_HasFeature(PyObject *o, int feature)

   Return true if the type object *o* sets the feature *feature*.  Type features
   are denoted by single bit flags.


.. cfunction:: int PyType_IS_GC(PyObject *o)

   Return true if the type object includes support for the cycle detector; this
   tests the type flag :const:`Py_TPFLAGS_HAVE_GC`.

   .. versionadded:: 2.0


.. cfunction:: int PyType_IsSubtype(PyTypeObject *a, PyTypeObject *b)

   Return true if *a* is a subtype of *b*.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyType_GenericAlloc(PyTypeObject *type, Py_ssize_t nitems)

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyType_GenericNew(PyTypeObject *type, PyObject *args, PyObject *kwds)

   .. versionadded:: 2.2


.. cfunction:: int PyType_Ready(PyTypeObject *type)

   Finalize a type object.  This should be called on all type objects to finish
   their initialization.  This function is responsible for adding inherited slots
   from a type's base class.  Return ``0`` on success, or return ``-1`` and sets an
   exception on error.

   .. versionadded:: 2.2


.. _noneobject:

The None Object
---------------

.. index:: object: None

Note that the :ctype:`PyTypeObject` for ``None`` is not directly exposed in the
Python/C API.  Since ``None`` is a singleton, testing for object identity (using
``==`` in C) is sufficient. There is no :cfunc:`PyNone_Check` function for the
same reason.


.. cvar:: PyObject* Py_None

   The Python ``None`` object, denoting lack of value.  This object has no methods.
   It needs to be treated just like any other object with respect to reference
   counts.


.. cmacro:: Py_RETURN_NONE

   Properly handle returning :cdata:`Py_None` from within a C function.

   .. versionadded:: 2.4


.. _numericobjects:

Numeric Objects
===============

.. index:: object: numeric


.. _intobjects:

Plain Integer Objects
---------------------

.. index:: object: integer


.. ctype:: PyIntObject

   This subtype of :ctype:`PyObject` represents a Python integer object.


.. cvar:: PyTypeObject PyInt_Type

   .. index:: single: IntType (in modules types)

   This instance of :ctype:`PyTypeObject` represents the Python plain integer type.
   This is the same object as ``int`` and ``types.IntType``.


.. cfunction:: int PyInt_Check(PyObject *o)

   Return true if *o* is of type :cdata:`PyInt_Type` or a subtype of
   :cdata:`PyInt_Type`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyInt_CheckExact(PyObject *o)

   Return true if *o* is of type :cdata:`PyInt_Type`, but not a subtype of
   :cdata:`PyInt_Type`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyInt_FromString(char *str, char **pend, int base)

   Return a new :ctype:`PyIntObject` or :ctype:`PyLongObject` based on the string
   value in *str*, which is interpreted according to the radix in *base*.  If
   *pend* is non-*NULL*, ``*pend`` will point to the first character in *str* which
   follows the representation of the number.  If *base* is ``0``, the radix will be
   determined based on the leading characters of *str*: if *str* starts with
   ``'0x'`` or ``'0X'``, radix 16 will be used; if *str* starts with ``'0'``, radix
   8 will be used; otherwise radix 10 will be used.  If *base* is not ``0``, it
   must be between ``2`` and ``36``, inclusive.  Leading spaces are ignored.  If
   there are no digits, :exc:`ValueError` will be raised.  If the string represents
   a number too large to be contained within the machine's :ctype:`long int` type
   and overflow warnings are being suppressed, a :ctype:`PyLongObject` will be
   returned.  If overflow warnings are not being suppressed, *NULL* will be
   returned in this case.


.. cfunction:: PyObject* PyInt_FromLong(long ival)

   Create a new integer object with a value of *ival*.

   The current implementation keeps an array of integer objects for all integers
   between ``-5`` and ``256``, when you create an int in that range you actually
   just get back a reference to the existing object. So it should be possible to
   change the value of ``1``.  I suspect the behaviour of Python in this case is
   undefined. :-)


.. cfunction:: PyObject* PyInt_FromSsize_t(Py_ssize_t ival)

   Create a new integer object with a value of *ival*. If the value exceeds
   ``LONG_MAX``, a long integer object is returned.

   .. versionadded:: 2.5


.. cfunction:: long PyInt_AsLong(PyObject *io)

   Will first attempt to cast the object to a :ctype:`PyIntObject`, if it is not
   already one, and then return its value. If there is an error, ``-1`` is
   returned, and the caller should check ``PyErr_Occurred()`` to find out whether
   there was an error, or whether the value just happened to be -1.


.. cfunction:: long PyInt_AS_LONG(PyObject *io)

   Return the value of the object *io*.  No error checking is performed.


.. cfunction:: unsigned long PyInt_AsUnsignedLongMask(PyObject *io)

   Will first attempt to cast the object to a :ctype:`PyIntObject` or
   :ctype:`PyLongObject`, if it is not already one, and then return its value as
   unsigned long.  This function does not check for overflow.

   .. versionadded:: 2.3


.. cfunction:: unsigned PY_LONG_LONG PyInt_AsUnsignedLongLongMask(PyObject *io)

   Will first attempt to cast the object to a :ctype:`PyIntObject` or
   :ctype:`PyLongObject`, if it is not already one, and then return its value as
   unsigned long long, without checking for overflow.

   .. versionadded:: 2.3


.. cfunction:: Py_ssize_t PyInt_AsSsize_t(PyObject *io)

   Will first attempt to cast the object to a :ctype:`PyIntObject` or
   :ctype:`PyLongObject`, if it is not already one, and then return its value as
   :ctype:`Py_ssize_t`.

   .. versionadded:: 2.5


.. cfunction:: long PyInt_GetMax()

   .. index:: single: LONG_MAX

   Return the system's idea of the largest integer it can handle
   (:const:`LONG_MAX`, as defined in the system header files).


.. _boolobjects:

Boolean Objects
---------------

Booleans in Python are implemented as a subclass of integers.  There are only
two booleans, :const:`Py_False` and :const:`Py_True`.  As such, the normal
creation and deletion functions don't apply to booleans.  The following macros
are available, however.


.. cfunction:: int PyBool_Check(PyObject *o)

   Return true if *o* is of type :cdata:`PyBool_Type`.

   .. versionadded:: 2.3


.. cvar:: PyObject* Py_False

   The Python ``False`` object.  This object has no methods.  It needs to be
   treated just like any other object with respect to reference counts.


.. cvar:: PyObject* Py_True

   The Python ``True`` object.  This object has no methods.  It needs to be treated
   just like any other object with respect to reference counts.


.. cmacro:: Py_RETURN_FALSE

   Return :const:`Py_False` from a function, properly incrementing its reference
   count.

   .. versionadded:: 2.4


.. cmacro:: Py_RETURN_TRUE

   Return :const:`Py_True` from a function, properly incrementing its reference
   count.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyBool_FromLong(long v)

   Return a new reference to :const:`Py_True` or :const:`Py_False` depending on the
   truth value of *v*.

   .. versionadded:: 2.3


.. _longobjects:

Long Integer Objects
--------------------

.. index:: object: long integer


.. ctype:: PyLongObject

   This subtype of :ctype:`PyObject` represents a Python long integer object.


.. cvar:: PyTypeObject PyLong_Type

   .. index:: single: LongType (in modules types)

   This instance of :ctype:`PyTypeObject` represents the Python long integer type.
   This is the same object as ``long`` and ``types.LongType``.


.. cfunction:: int PyLong_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyLongObject` or a subtype of
   :ctype:`PyLongObject`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyLong_CheckExact(PyObject *p)

   Return true if its argument is a :ctype:`PyLongObject`, but not a subtype of
   :ctype:`PyLongObject`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyLong_FromLong(long v)

   Return a new :ctype:`PyLongObject` object from *v*, or *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromUnsignedLong(unsigned long v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`unsigned long`, or
   *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromLongLong(PY_LONG_LONG v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`long long`, or *NULL*
   on failure.


.. cfunction:: PyObject* PyLong_FromUnsignedLongLong(unsigned PY_LONG_LONG v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`unsigned long long`,
   or *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromDouble(double v)

   Return a new :ctype:`PyLongObject` object from the integer part of *v*, or
   *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromString(char *str, char **pend, int base)

   Return a new :ctype:`PyLongObject` based on the string value in *str*, which is
   interpreted according to the radix in *base*.  If *pend* is non-*NULL*,
   ``*pend`` will point to the first character in *str* which follows the
   representation of the number.  If *base* is ``0``, the radix will be determined
   based on the leading characters of *str*: if *str* starts with ``'0x'`` or
   ``'0X'``, radix 16 will be used; if *str* starts with ``'0'``, radix 8 will be
   used; otherwise radix 10 will be used.  If *base* is not ``0``, it must be
   between ``2`` and ``36``, inclusive.  Leading spaces are ignored.  If there are
   no digits, :exc:`ValueError` will be raised.


.. cfunction:: PyObject* PyLong_FromUnicode(Py_UNICODE *u, Py_ssize_t length, int base)

   Convert a sequence of Unicode digits to a Python long integer value.  The first
   parameter, *u*, points to the first character of the Unicode string, *length*
   gives the number of characters, and *base* is the radix for the conversion.  The
   radix must be in the range [2, 36]; if it is out of range, :exc:`ValueError`
   will be raised.

   .. versionadded:: 1.6


.. cfunction:: PyObject* PyLong_FromVoidPtr(void *p)

   Create a Python integer or long integer from the pointer *p*. The pointer value
   can be retrieved from the resulting value using :cfunc:`PyLong_AsVoidPtr`.

   .. versionadded:: 1.5.2

   .. versionchanged:: 2.5
      If the integer is larger than LONG_MAX, a positive long integer is returned.


.. cfunction:: long PyLong_AsLong(PyObject *pylong)

   .. index::
      single: LONG_MAX
      single: OverflowError (built-in exception)

   Return a C :ctype:`long` representation of the contents of *pylong*.  If
   *pylong* is greater than :const:`LONG_MAX`, an :exc:`OverflowError` is raised.


.. cfunction:: unsigned long PyLong_AsUnsignedLong(PyObject *pylong)

   .. index::
      single: ULONG_MAX
      single: OverflowError (built-in exception)

   Return a C :ctype:`unsigned long` representation of the contents of *pylong*.
   If *pylong* is greater than :const:`ULONG_MAX`, an :exc:`OverflowError` is
   raised.


.. cfunction:: PY_LONG_LONG PyLong_AsLongLong(PyObject *pylong)

   Return a C :ctype:`long long` from a Python long integer.  If *pylong* cannot be
   represented as a :ctype:`long long`, an :exc:`OverflowError` will be raised.

   .. versionadded:: 2.2


.. cfunction:: unsigned PY_LONG_LONG PyLong_AsUnsignedLongLong(PyObject *pylong)

   Return a C :ctype:`unsigned long long` from a Python long integer. If *pylong*
   cannot be represented as an :ctype:`unsigned long long`, an :exc:`OverflowError`
   will be raised if the value is positive, or a :exc:`TypeError` will be raised if
   the value is negative.

   .. versionadded:: 2.2


.. cfunction:: unsigned long PyLong_AsUnsignedLongMask(PyObject *io)

   Return a C :ctype:`unsigned long` from a Python long integer, without checking
   for overflow.

   .. versionadded:: 2.3


.. cfunction:: unsigned PY_LONG_LONG PyLong_AsUnsignedLongLongMask(PyObject *io)

   Return a C :ctype:`unsigned long long` from a Python long integer, without
   checking for overflow.

   .. versionadded:: 2.3


.. cfunction:: double PyLong_AsDouble(PyObject *pylong)

   Return a C :ctype:`double` representation of the contents of *pylong*.  If
   *pylong* cannot be approximately represented as a :ctype:`double`, an
   :exc:`OverflowError` exception is raised and ``-1.0`` will be returned.


.. cfunction:: void* PyLong_AsVoidPtr(PyObject *pylong)

   Convert a Python integer or long integer *pylong* to a C :ctype:`void` pointer.
   If *pylong* cannot be converted, an :exc:`OverflowError` will be raised.  This
   is only assured to produce a usable :ctype:`void` pointer for values created
   with :cfunc:`PyLong_FromVoidPtr`.

   .. versionadded:: 1.5.2

   .. versionchanged:: 2.5
      For values outside 0..LONG_MAX, both signed and unsigned integers are acccepted.


.. _floatobjects:

Floating Point Objects
----------------------

.. index:: object: floating point


.. ctype:: PyFloatObject

   This subtype of :ctype:`PyObject` represents a Python floating point object.


.. cvar:: PyTypeObject PyFloat_Type

   .. index:: single: FloatType (in modules types)

   This instance of :ctype:`PyTypeObject` represents the Python floating point
   type.  This is the same object as ``float`` and ``types.FloatType``.


.. cfunction:: int PyFloat_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyFloatObject` or a subtype of
   :ctype:`PyFloatObject`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyFloat_CheckExact(PyObject *p)

   Return true if its argument is a :ctype:`PyFloatObject`, but not a subtype of
   :ctype:`PyFloatObject`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyFloat_FromString(PyObject *str)

   Create a :ctype:`PyFloatObject` object based on the string value in *str*, or
   *NULL* on failure.


.. cfunction:: PyObject* PyFloat_FromDouble(double v)

   Create a :ctype:`PyFloatObject` object from *v*, or *NULL* on failure.


.. cfunction:: double PyFloat_AsDouble(PyObject *pyfloat)

   Return a C :ctype:`double` representation of the contents of *pyfloat*.  If
   *pyfloat* is not a Python floating point object but has a :meth:`__float__`
   method, this method will first be called to convert *pyfloat* into a float.


.. cfunction:: double PyFloat_AS_DOUBLE(PyObject *pyfloat)

   Return a C :ctype:`double` representation of the contents of *pyfloat*, but
   without error checking.


.. _complexobjects:

Complex Number Objects
----------------------

.. index:: object: complex number

Python's complex number objects are implemented as two distinct types when
viewed from the C API:  one is the Python object exposed to Python programs, and
the other is a C structure which represents the actual complex number value.
The API provides functions for working with both.


Complex Numbers as C Structures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note that the functions which accept these structures as parameters and return
them as results do so *by value* rather than dereferencing them through
pointers.  This is consistent throughout the API.


.. ctype:: Py_complex

   The C structure which corresponds to the value portion of a Python complex
   number object.  Most of the functions for dealing with complex number objects
   use structures of this type as input or output values, as appropriate.  It is
   defined as::

      typedef struct {
         double real;
         double imag;
      } Py_complex;


.. cfunction:: Py_complex _Py_c_sum(Py_complex left, Py_complex right)

   Return the sum of two complex numbers, using the C :ctype:`Py_complex`
   representation.


.. cfunction:: Py_complex _Py_c_diff(Py_complex left, Py_complex right)

   Return the difference between two complex numbers, using the C
   :ctype:`Py_complex` representation.


.. cfunction:: Py_complex _Py_c_neg(Py_complex complex)

   Return the negation of the complex number *complex*, using the C
   :ctype:`Py_complex` representation.


.. cfunction:: Py_complex _Py_c_prod(Py_complex left, Py_complex right)

   Return the product of two complex numbers, using the C :ctype:`Py_complex`
   representation.


.. cfunction:: Py_complex _Py_c_quot(Py_complex dividend, Py_complex divisor)

   Return the quotient of two complex numbers, using the C :ctype:`Py_complex`
   representation.


.. cfunction:: Py_complex _Py_c_pow(Py_complex num, Py_complex exp)

   Return the exponentiation of *num* by *exp*, using the C :ctype:`Py_complex`
   representation.


Complex Numbers as Python Objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. ctype:: PyComplexObject

   This subtype of :ctype:`PyObject` represents a Python complex number object.


.. cvar:: PyTypeObject PyComplex_Type

   This instance of :ctype:`PyTypeObject` represents the Python complex number
   type. It is the same object as ``complex`` and ``types.ComplexType``.


.. cfunction:: int PyComplex_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyComplexObject` or a subtype of
   :ctype:`PyComplexObject`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyComplex_CheckExact(PyObject *p)

   Return true if its argument is a :ctype:`PyComplexObject`, but not a subtype of
   :ctype:`PyComplexObject`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyComplex_FromCComplex(Py_complex v)

   Create a new Python complex number object from a C :ctype:`Py_complex` value.


.. cfunction:: PyObject* PyComplex_FromDoubles(double real, double imag)

   Return a new :ctype:`PyComplexObject` object from *real* and *imag*.


.. cfunction:: double PyComplex_RealAsDouble(PyObject *op)

   Return the real part of *op* as a C :ctype:`double`.


.. cfunction:: double PyComplex_ImagAsDouble(PyObject *op)

   Return the imaginary part of *op* as a C :ctype:`double`.


.. cfunction:: Py_complex PyComplex_AsCComplex(PyObject *op)

   Return the :ctype:`Py_complex` value of the complex number *op*.

   .. versionchanged:: 2.6
      If *op* is not a Python complex number object but has a :meth:`__complex__`
      method, this method will first be called to convert *op* to a Python complex
      number object.


.. _sequenceobjects:

Sequence Objects
================

.. index:: object: sequence

Generic operations on sequence objects were discussed in the previous chapter;
this section deals with the specific kinds of sequence objects that are
intrinsic to the Python language.


.. _stringobjects:

String Objects
--------------

These functions raise :exc:`TypeError` when expecting a string parameter and are
called with a non-string parameter.

.. index:: object: string


.. ctype:: PyStringObject

   This subtype of :ctype:`PyObject` represents a Python string object.


.. cvar:: PyTypeObject PyString_Type

   .. index:: single: StringType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python string type; it is
   the same object as ``str`` and ``types.StringType`` in the Python layer. .


.. cfunction:: int PyString_Check(PyObject *o)

   Return true if the object *o* is a string object or an instance of a subtype of
   the string type.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyString_CheckExact(PyObject *o)

   Return true if the object *o* is a string object, but not an instance of a
   subtype of the string type.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyString_FromString(const char *v)

   Return a new string object with a copy of the string *v* as value on success,
   and *NULL* on failure.  The parameter *v* must not be *NULL*; it will not be
   checked.


.. cfunction:: PyObject* PyString_FromStringAndSize(const char *v, Py_ssize_t len)

   Return a new string object with a copy of the string *v* as value and length
   *len* on success, and *NULL* on failure.  If *v* is *NULL*, the contents of the
   string are uninitialized.


.. cfunction:: PyObject* PyString_FromFormat(const char *format, ...)

   Take a C :cfunc:`printf`\ -style *format* string and a variable number of
   arguments, calculate the size of the resulting Python string and return a string
   with the values formatted into it.  The variable arguments must be C types and
   must correspond exactly to the format characters in the *format* string.  The
   following format characters are allowed:

   .. % This should be exactly the same as the table in PyErr_Format.
   .. % One should just refer to the other.
   .. % The descriptions for %zd and %zu are wrong, but the truth is complicated
   .. % because not all compilers support the %z width modifier -- we fake it
   .. % when necessary via interpolating PY_FORMAT_SIZE_T.
   .. % %u, %lu, %zu should have "new in Python 2.5" blurbs.

   +-------------------+---------------+--------------------------------+
   | Format Characters | Type          | Comment                        |
   +===================+===============+================================+
   | :attr:`%%`        | *n/a*         | The literal % character.       |
   +-------------------+---------------+--------------------------------+
   | :attr:`%c`        | int           | A single character,            |
   |                   |               | represented as an C int.       |
   +-------------------+---------------+--------------------------------+
   | :attr:`%d`        | int           | Exactly equivalent to          |
   |                   |               | ``printf("%d")``.              |
   +-------------------+---------------+--------------------------------+
   | :attr:`%u`        | unsigned int  | Exactly equivalent to          |
   |                   |               | ``printf("%u")``.              |
   +-------------------+---------------+--------------------------------+
   | :attr:`%ld`       | long          | Exactly equivalent to          |
   |                   |               | ``printf("%ld")``.             |
   +-------------------+---------------+--------------------------------+
   | :attr:`%lu`       | unsigned long | Exactly equivalent to          |
   |                   |               | ``printf("%lu")``.             |
   +-------------------+---------------+--------------------------------+
   | :attr:`%zd`       | Py_ssize_t    | Exactly equivalent to          |
   |                   |               | ``printf("%zd")``.             |
   +-------------------+---------------+--------------------------------+
   | :attr:`%zu`       | size_t        | Exactly equivalent to          |
   |                   |               | ``printf("%zu")``.             |
   +-------------------+---------------+--------------------------------+
   | :attr:`%i`        | int           | Exactly equivalent to          |
   |                   |               | ``printf("%i")``.              |
   +-------------------+---------------+--------------------------------+
   | :attr:`%x`        | int           | Exactly equivalent to          |
   |                   |               | ``printf("%x")``.              |
   +-------------------+---------------+--------------------------------+
   | :attr:`%s`        | char\*        | A null-terminated C character  |
   |                   |               | array.                         |
   +-------------------+---------------+--------------------------------+
   | :attr:`%p`        | void\*        | The hex representation of a C  |
   |                   |               | pointer. Mostly equivalent to  |
   |                   |               | ``printf("%p")`` except that   |
   |                   |               | it is guaranteed to start with |
   |                   |               | the literal ``0x`` regardless  |
   |                   |               | of what the platform's         |
   |                   |               | ``printf`` yields.             |
   +-------------------+---------------+--------------------------------+

   An unrecognized format character causes all the rest of the format string to be
   copied as-is to the result string, and any extra arguments discarded.


.. cfunction:: PyObject* PyString_FromFormatV(const char *format, va_list vargs)

   Identical to :func:`PyString_FromFormat` except that it takes exactly two
   arguments.


.. cfunction:: Py_ssize_t PyString_Size(PyObject *string)

   Return the length of the string in string object *string*.


.. cfunction:: Py_ssize_t PyString_GET_SIZE(PyObject *string)

   Macro form of :cfunc:`PyString_Size` but without error checking.


.. cfunction:: char* PyString_AsString(PyObject *string)

   Return a NUL-terminated representation of the contents of *string*.  The pointer
   refers to the internal buffer of *string*, not a copy.  The data must not be
   modified in any way, unless the string was just created using
   ``PyString_FromStringAndSize(NULL, size)``. It must not be deallocated.  If
   *string* is a Unicode object, this function computes the default encoding of
   *string* and operates on that.  If *string* is not a string object at all,
   :cfunc:`PyString_AsString` returns *NULL* and raises :exc:`TypeError`.


.. cfunction:: char* PyString_AS_STRING(PyObject *string)

   Macro form of :cfunc:`PyString_AsString` but without error checking.  Only
   string objects are supported; no Unicode objects should be passed.


.. cfunction:: int PyString_AsStringAndSize(PyObject *obj, char **buffer, Py_ssize_t *length)

   Return a NUL-terminated representation of the contents of the object *obj*
   through the output variables *buffer* and *length*.

   The function accepts both string and Unicode objects as input. For Unicode
   objects it returns the default encoded version of the object.  If *length* is
   *NULL*, the resulting buffer may not contain NUL characters; if it does, the
   function returns ``-1`` and a :exc:`TypeError` is raised.

   The buffer refers to an internal string buffer of *obj*, not a copy. The data
   must not be modified in any way, unless the string was just created using
   ``PyString_FromStringAndSize(NULL, size)``.  It must not be deallocated.  If
   *string* is a Unicode object, this function computes the default encoding of
   *string* and operates on that.  If *string* is not a string object at all,
   :cfunc:`PyString_AsStringAndSize` returns ``-1`` and raises :exc:`TypeError`.


.. cfunction:: void PyString_Concat(PyObject **string, PyObject *newpart)

   Create a new string object in *\*string* containing the contents of *newpart*
   appended to *string*; the caller will own the new reference.  The reference to
   the old value of *string* will be stolen.  If the new string cannot be created,
   the old reference to *string* will still be discarded and the value of
   *\*string* will be set to *NULL*; the appropriate exception will be set.


.. cfunction:: void PyString_ConcatAndDel(PyObject **string, PyObject *newpart)

   Create a new string object in *\*string* containing the contents of *newpart*
   appended to *string*.  This version decrements the reference count of *newpart*.


.. cfunction:: int _PyString_Resize(PyObject **string, Py_ssize_t newsize)

   A way to resize a string object even though it is "immutable". Only use this to
   build up a brand new string object; don't use this if the string may already be
   known in other parts of the code.  It is an error to call this function if the
   refcount on the input string object is not one. Pass the address of an existing
   string object as an lvalue (it may be written into), and the new size desired.
   On success, *\*string* holds the resized string object and ``0`` is returned;
   the address in *\*string* may differ from its input value.  If the reallocation
   fails, the original string object at *\*string* is deallocated, *\*string* is
   set to *NULL*, a memory exception is set, and ``-1`` is returned.


.. cfunction:: PyObject* PyString_Format(PyObject *format, PyObject *args)

   Return a new string object from *format* and *args*. Analogous to ``format %
   args``.  The *args* argument must be a tuple.


.. cfunction:: void PyString_InternInPlace(PyObject **string)

   Intern the argument *\*string* in place.  The argument must be the address of a
   pointer variable pointing to a Python string object.  If there is an existing
   interned string that is the same as *\*string*, it sets *\*string* to it
   (decrementing the reference count of the old string object and incrementing the
   reference count of the interned string object), otherwise it leaves *\*string*
   alone and interns it (incrementing its reference count).  (Clarification: even
   though there is a lot of talk about reference counts, think of this function as
   reference-count-neutral; you own the object after the call if and only if you
   owned it before the call.)


.. cfunction:: PyObject* PyString_InternFromString(const char *v)

   A combination of :cfunc:`PyString_FromString` and
   :cfunc:`PyString_InternInPlace`, returning either a new string object that has
   been interned, or a new ("owned") reference to an earlier interned string object
   with the same value.


.. cfunction:: PyObject* PyString_Decode(const char *s, Py_ssize_t size, const char *encoding, const char *errors)

   Create an object by decoding *size* bytes of the encoded buffer *s* using the
   codec registered for *encoding*.  *encoding* and *errors* have the same meaning
   as the parameters of the same name in the :func:`unicode` built-in function.
   The codec to be used is looked up using the Python codec registry.  Return
   *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyString_AsDecodedObject(PyObject *str, const char *encoding, const char *errors)

   Decode a string object by passing it to the codec registered for *encoding* and
   return the result as Python object. *encoding* and *errors* have the same
   meaning as the parameters of the same name in the string :meth:`encode` method.
   The codec to be used is looked up using the Python codec registry. Return *NULL*
   if an exception was raised by the codec.


.. cfunction:: PyObject* PyString_Encode(const char *s, Py_ssize_t size, const char *encoding, const char *errors)

   Encode the :ctype:`char` buffer of the given size by passing it to the codec
   registered for *encoding* and return a Python object. *encoding* and *errors*
   have the same meaning as the parameters of the same name in the string
   :meth:`encode` method. The codec to be used is looked up using the Python codec
   registry.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyString_AsEncodedObject(PyObject *str, const char *encoding, const char *errors)

   Encode a string object using the codec registered for *encoding* and return the
   result as Python object. *encoding* and *errors* have the same meaning as the
   parameters of the same name in the string :meth:`encode` method. The codec to be
   used is looked up using the Python codec registry. Return *NULL* if an exception
   was raised by the codec.


.. _unicodeobjects:

Unicode Objects
---------------

.. sectionauthor:: Marc-Andre Lemburg <mal@lemburg.com>


These are the basic Unicode object types used for the Unicode implementation in
Python:

.. % --- Unicode Type -------------------------------------------------------


.. ctype:: Py_UNICODE

   This type represents the storage type which is used by Python internally as
   basis for holding Unicode ordinals.  Python's default builds use a 16-bit type
   for :ctype:`Py_UNICODE` and store Unicode values internally as UCS2. It is also
   possible to build a UCS4 version of Python (most recent Linux distributions come
   with UCS4 builds of Python). These builds then use a 32-bit type for
   :ctype:`Py_UNICODE` and store Unicode data internally as UCS4. On platforms
   where :ctype:`wchar_t` is available and compatible with the chosen Python
   Unicode build variant, :ctype:`Py_UNICODE` is a typedef alias for
   :ctype:`wchar_t` to enhance native platform compatibility. On all other
   platforms, :ctype:`Py_UNICODE` is a typedef alias for either :ctype:`unsigned
   short` (UCS2) or :ctype:`unsigned long` (UCS4).

Note that UCS2 and UCS4 Python builds are not binary compatible. Please keep
this in mind when writing extensions or interfaces.


.. ctype:: PyUnicodeObject

   This subtype of :ctype:`PyObject` represents a Python Unicode object.


.. cvar:: PyTypeObject PyUnicode_Type

   This instance of :ctype:`PyTypeObject` represents the Python Unicode type.  It
   is exposed to Python code as ``str``.

The following APIs are really C macros and can be used to do fast checks and to
access internal read-only data of Unicode objects:


.. cfunction:: int PyUnicode_Check(PyObject *o)

   Return true if the object *o* is a Unicode object or an instance of a Unicode
   subtype.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyUnicode_CheckExact(PyObject *o)

   Return true if the object *o* is a Unicode object, but not an instance of a
   subtype.

   .. versionadded:: 2.2


.. cfunction:: Py_ssize_t PyUnicode_GET_SIZE(PyObject *o)

   Return the size of the object.  *o* has to be a :ctype:`PyUnicodeObject` (not
   checked).


.. cfunction:: Py_ssize_t PyUnicode_GET_DATA_SIZE(PyObject *o)

   Return the size of the object's internal buffer in bytes.  *o* has to be a
   :ctype:`PyUnicodeObject` (not checked).


.. cfunction:: Py_UNICODE* PyUnicode_AS_UNICODE(PyObject *o)

   Return a pointer to the internal :ctype:`Py_UNICODE` buffer of the object.  *o*
   has to be a :ctype:`PyUnicodeObject` (not checked).


.. cfunction:: const char* PyUnicode_AS_DATA(PyObject *o)

   Return a pointer to the internal buffer of the object. *o* has to be a
   :ctype:`PyUnicodeObject` (not checked).

Unicode provides many different character properties. The most often needed ones
are available through these macros which are mapped to C functions depending on
the Python configuration.

.. % --- Unicode character properties ---------------------------------------


.. cfunction:: int Py_UNICODE_ISSPACE(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a whitespace character.


.. cfunction:: int Py_UNICODE_ISLOWER(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a lowercase character.


.. cfunction:: int Py_UNICODE_ISUPPER(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is an uppercase character.


.. cfunction:: int Py_UNICODE_ISTITLE(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a titlecase character.


.. cfunction:: int Py_UNICODE_ISLINEBREAK(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a linebreak character.


.. cfunction:: int Py_UNICODE_ISDECIMAL(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a decimal character.


.. cfunction:: int Py_UNICODE_ISDIGIT(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a digit character.


.. cfunction:: int Py_UNICODE_ISNUMERIC(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a numeric character.


.. cfunction:: int Py_UNICODE_ISALPHA(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is an alphabetic character.


.. cfunction:: int Py_UNICODE_ISALNUM(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is an alphanumeric character.

These APIs can be used for fast direct character conversions:


.. cfunction:: Py_UNICODE Py_UNICODE_TOLOWER(Py_UNICODE ch)

   Return the character *ch* converted to lower case.


.. cfunction:: Py_UNICODE Py_UNICODE_TOUPPER(Py_UNICODE ch)

   Return the character *ch* converted to upper case.


.. cfunction:: Py_UNICODE Py_UNICODE_TOTITLE(Py_UNICODE ch)

   Return the character *ch* converted to title case.


.. cfunction:: int Py_UNICODE_TODECIMAL(Py_UNICODE ch)

   Return the character *ch* converted to a decimal positive integer.  Return
   ``-1`` if this is not possible.  This macro does not raise exceptions.


.. cfunction:: int Py_UNICODE_TODIGIT(Py_UNICODE ch)

   Return the character *ch* converted to a single digit integer. Return ``-1`` if
   this is not possible.  This macro does not raise exceptions.


.. cfunction:: double Py_UNICODE_TONUMERIC(Py_UNICODE ch)

   Return the character *ch* converted to a double. Return ``-1.0`` if this is not
   possible.  This macro does not raise exceptions.

To create Unicode objects and access their basic sequence properties, use these
APIs:

.. % --- Plain Py_UNICODE ---------------------------------------------------


.. cfunction:: PyObject* PyUnicode_FromUnicode(const Py_UNICODE *u, Py_ssize_t size)

   Create a Unicode Object from the Py_UNICODE buffer *u* of the given size. *u*
   may be *NULL* which causes the contents to be undefined. It is the user's
   responsibility to fill in the needed data.  The buffer is copied into the new
   object. If the buffer is not *NULL*, the return value might be a shared object.
   Therefore, modification of the resulting Unicode object is only allowed when *u*
   is *NULL*.


.. cfunction:: PyObject* PyUnicode_FromStringAndSize(const char *u, Py_ssize_t size)

   Create a Unicode Object from the char buffer *u*.  The bytes will be interpreted
   as being UTF-8 encoded.  *u* may also be *NULL* which
   causes the contents to be undefined. It is the user's responsibility to fill in
   the needed data.  The buffer is copied into the new object. If the buffer is not
   *NULL*, the return value might be a shared object. Therefore, modification of
   the resulting Unicode object is only allowed when *u* is *NULL*.

   .. versionadded:: 3.0


.. cfunction:: PyObject *PyUnicode_FromString(const char *u)

   Create a Unicode object from an UTF-8 encoded null-terminated char buffer
   *u*.

   .. versionadded:: 3.0


.. cfunction:: PyObject* PyUnicode_FromFormat(const char *format, ...)

   Take a C :cfunc:`printf`\ -style *format* string and a variable number of
   arguments, calculate the size of the resulting Python unicode string and return
   a string with the values formatted into it.  The variable arguments must be C
   types and must correspond exactly to the format characters in the *format*
   string.  The following format characters are allowed:

   .. % The descriptions for %zd and %zu are wrong, but the truth is complicated
   .. % because not all compilers support the %z width modifier -- we fake it
   .. % when necessary via interpolating PY_FORMAT_SIZE_T.

   +-------------------+---------------------+--------------------------------+
   | Format Characters | Type                | Comment                        |
   +===================+=====================+================================+
   | :attr:`%%`        | *n/a*               | The literal % character.       |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%c`        | int                 | A single character,            |
   |                   |                     | represented as an C int.       |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%d`        | int                 | Exactly equivalent to          |
   |                   |                     | ``printf("%d")``.              |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%u`        | unsigned int        | Exactly equivalent to          |
   |                   |                     | ``printf("%u")``.              |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%ld`       | long                | Exactly equivalent to          |
   |                   |                     | ``printf("%ld")``.             |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%lu`       | unsigned long       | Exactly equivalent to          |
   |                   |                     | ``printf("%lu")``.             |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%zd`       | Py_ssize_t          | Exactly equivalent to          |
   |                   |                     | ``printf("%zd")``.             |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%zu`       | size_t              | Exactly equivalent to          |
   |                   |                     | ``printf("%zu")``.             |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%i`        | int                 | Exactly equivalent to          |
   |                   |                     | ``printf("%i")``.              |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%x`        | int                 | Exactly equivalent to          |
   |                   |                     | ``printf("%x")``.              |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%s`        | char\*              | A null-terminated C character  |
   |                   |                     | array.                         |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%p`        | void\*              | The hex representation of a C  |
   |                   |                     | pointer. Mostly equivalent to  |
   |                   |                     | ``printf("%p")`` except that   |
   |                   |                     | it is guaranteed to start with |
   |                   |                     | the literal ``0x`` regardless  |
   |                   |                     | of what the platform's         |
   |                   |                     | ``printf`` yields.             |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%U`        | PyObject\*          | A unicode object.              |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%V`        | PyObject\*, char \* | A unicode object (which may be |
   |                   |                     | *NULL*) and a null-terminated  |
   |                   |                     | C character array as a second  |
   |                   |                     | parameter (which will be used, |
   |                   |                     | if the first parameter is      |
   |                   |                     | *NULL*).                       |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%S`        | PyObject\*          | The result of calling          |
   |                   |                     | :func:`PyObject_Unicode`.      |
   +-------------------+---------------------+--------------------------------+
   | :attr:`%R`        | PyObject\*          | The result of calling          |
   |                   |                     | :func:`PyObject_Repr`.         |
   +-------------------+---------------------+--------------------------------+

   An unrecognized format character causes all the rest of the format string to be
   copied as-is to the result string, and any extra arguments discarded.

   .. versionadded:: 3.0


.. cfunction:: PyObject* PyUnicode_FromFormatV(const char *format, va_list vargs)

   Identical to :func:`PyUnicode_FromFormat` except that it takes exactly two
   arguments.

   .. versionadded:: 3.0


.. cfunction:: Py_UNICODE* PyUnicode_AsUnicode(PyObject *unicode)

   Return a read-only pointer to the Unicode object's internal :ctype:`Py_UNICODE`
   buffer, *NULL* if *unicode* is not a Unicode object.


.. cfunction:: Py_ssize_t PyUnicode_GetSize(PyObject *unicode)

   Return the length of the Unicode object.


.. cfunction:: PyObject* PyUnicode_FromEncodedObject(PyObject *obj, const char *encoding, const char *errors)

   Coerce an encoded object *obj* to an Unicode object and return a reference with
   incremented refcount.

   String and other char buffer compatible objects are decoded according to the
   given encoding and using the error handling defined by errors.  Both can be
   *NULL* to have the interface use the default values (see the next section for
   details).

   All other objects, including Unicode objects, cause a :exc:`TypeError` to be
   set.

   The API returns *NULL* if there was an error.  The caller is responsible for
   decref'ing the returned objects.


.. cfunction:: PyObject* PyUnicode_FromObject(PyObject *obj)

   Shortcut for ``PyUnicode_FromEncodedObject(obj, NULL, "strict")`` which is used
   throughout the interpreter whenever coercion to Unicode is needed.

If the platform supports :ctype:`wchar_t` and provides a header file wchar.h,
Python can interface directly to this type using the following functions.
Support is optimized if Python's own :ctype:`Py_UNICODE` type is identical to
the system's :ctype:`wchar_t`.

.. % --- wchar_t support for platforms which support it ---------------------


.. cfunction:: PyObject* PyUnicode_FromWideChar(const wchar_t *w, Py_ssize_t size)

   Create a Unicode object from the :ctype:`wchar_t` buffer *w* of the given size.
   Return *NULL* on failure.


.. cfunction:: Py_ssize_t PyUnicode_AsWideChar(PyUnicodeObject *unicode, wchar_t *w, Py_ssize_t size)

   Copy the Unicode object contents into the :ctype:`wchar_t` buffer *w*.  At most
   *size* :ctype:`wchar_t` characters are copied (excluding a possibly trailing
   0-termination character).  Return the number of :ctype:`wchar_t` characters
   copied or -1 in case of an error.  Note that the resulting :ctype:`wchar_t`
   string may or may not be 0-terminated.  It is the responsibility of the caller
   to make sure that the :ctype:`wchar_t` string is 0-terminated in case this is
   required by the application.


.. _builtincodecs:

Built-in Codecs
^^^^^^^^^^^^^^^

Python provides a set of builtin codecs which are written in C for speed. All of
these codecs are directly usable via the following functions.

Many of the following APIs take two arguments encoding and errors. These
parameters encoding and errors have the same semantics as the ones of the
builtin unicode() Unicode object constructor.

Setting encoding to *NULL* causes the default encoding to be used which is
ASCII.  The file system calls should use :cdata:`Py_FileSystemDefaultEncoding`
as the encoding for file names. This variable should be treated as read-only: On
some systems, it will be a pointer to a static string, on others, it will change
at run-time (such as when the application invokes setlocale).

Error handling is set by errors which may also be set to *NULL* meaning to use
the default handling defined for the codec.  Default error handling for all
builtin codecs is "strict" (:exc:`ValueError` is raised).

The codecs all use a similar interface.  Only deviation from the following
generic ones are documented for simplicity.

These are the generic codec APIs:

.. % --- Generic Codecs -----------------------------------------------------


.. cfunction:: PyObject* PyUnicode_Decode(const char *s, Py_ssize_t size, const char *encoding, const char *errors)

   Create a Unicode object by decoding *size* bytes of the encoded string *s*.
   *encoding* and *errors* have the same meaning as the parameters of the same name
   in the :func:`unicode` builtin function.  The codec to be used is looked up
   using the Python codec registry.  Return *NULL* if an exception was raised by
   the codec.


.. cfunction:: PyObject* PyUnicode_Encode(const Py_UNICODE *s, Py_ssize_t size, const char *encoding, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size and return a Python
   string object.  *encoding* and *errors* have the same meaning as the parameters
   of the same name in the Unicode :meth:`encode` method.  The codec to be used is
   looked up using the Python codec registry.  Return *NULL* if an exception was
   raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsEncodedString(PyObject *unicode, const char *encoding, const char *errors)

   Encode a Unicode object and return the result as Python string object.
   *encoding* and *errors* have the same meaning as the parameters of the same name
   in the Unicode :meth:`encode` method. The codec to be used is looked up using
   the Python codec registry. Return *NULL* if an exception was raised by the
   codec.

These are the UTF-8 codec APIs:

.. % --- UTF-8 Codecs -------------------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeUTF8(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the UTF-8 encoded string
   *s*. Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_DecodeUTF8Stateful(const char *s, Py_ssize_t size, const char *errors, Py_ssize_t *consumed)

   If *consumed* is *NULL*, behave like :cfunc:`PyUnicode_DecodeUTF8`. If
   *consumed* is not *NULL*, trailing incomplete UTF-8 byte sequences will not be
   treated as an error. Those bytes will not be decoded and the number of bytes
   that have been decoded will be stored in *consumed*.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyUnicode_EncodeUTF8(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using UTF-8 and return a
   Python string object.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsUTF8String(PyObject *unicode)

   Encode a Unicode objects using UTF-8 and return the result as Python string
   object.  Error handling is "strict".  Return *NULL* if an exception was raised
   by the codec.

These are the UTF-32 codec APIs:

.. % --- UTF-32 Codecs ------------------------------------------------------ */


.. cfunction:: PyObject* PyUnicode_DecodeUTF32(const char *s, Py_ssize_t size, const char *errors, int *byteorder)

   Decode *length* bytes from a UTF-32 encoded buffer string and return the
   corresponding Unicode object.  *errors* (if non-*NULL*) defines the error
   handling. It defaults to "strict".

   If *byteorder* is non-*NULL*, the decoder starts decoding using the given byte
   order::

      *byteorder == -1: little endian
      *byteorder == 0:  native order
      *byteorder == 1:  big endian

   and then switches if the first four bytes of the input data are a byte order mark
   (BOM) and the specified byte order is native order.  This BOM is not copied into
   the resulting Unicode string.  After completion, *\*byteorder* is set to the
   current byte order at the end of input data.

   In a narrow build codepoints outside the BMP will be decoded as surrogate pairs.

   If *byteorder* is *NULL*, the codec starts in native order mode.

   Return *NULL* if an exception was raised by the codec.

   .. versionadded:: 2.6


.. cfunction:: PyObject* PyUnicode_DecodeUTF32Stateful(const char *s, Py_ssize_t size, const char *errors, int *byteorder, Py_ssize_t *consumed)

   If *consumed* is *NULL*, behave like :cfunc:`PyUnicode_DecodeUTF32`. If
   *consumed* is not *NULL*, :cfunc:`PyUnicode_DecodeUTF32Stateful` will not treat
   trailing incomplete UTF-32 byte sequences (such as a number of bytes not divisible
   by four) as an error. Those bytes will not be decoded and the number of bytes
   that have been decoded will be stored in *consumed*.

   .. versionadded:: 2.6


.. cfunction:: PyObject* PyUnicode_EncodeUTF32(const Py_UNICODE *s, Py_ssize_t size, const char *errors, int byteorder)

   Return a Python bytes object holding the UTF-32 encoded value of the Unicode
   data in *s*.  If *byteorder* is not ``0``, output is written according to the
   following byte order::

      byteorder == -1: little endian
      byteorder == 0:  native byte order (writes a BOM mark)
      byteorder == 1:  big endian

   If byteorder is ``0``, the output string will always start with the Unicode BOM
   mark (U+FEFF). In the other two modes, no BOM mark is prepended.

   If *Py_UNICODE_WIDE* is not defined, surrogate pairs will be output
   as a single codepoint.

   Return *NULL* if an exception was raised by the codec.

   .. versionadded:: 2.6


.. cfunction:: PyObject* PyUnicode_AsUTF32String(PyObject *unicode)

   Return a Python string using the UTF-32 encoding in native byte order. The
   string always starts with a BOM mark.  Error handling is "strict".  Return
   *NULL* if an exception was raised by the codec.

   .. versionadded:: 2.6


These are the UTF-16 codec APIs:

.. % --- UTF-16 Codecs ------------------------------------------------------ */


.. cfunction:: PyObject* PyUnicode_DecodeUTF16(const char *s, Py_ssize_t size, const char *errors, int *byteorder)

   Decode *length* bytes from a UTF-16 encoded buffer string and return the
   corresponding Unicode object.  *errors* (if non-*NULL*) defines the error
   handling. It defaults to "strict".

   If *byteorder* is non-*NULL*, the decoder starts decoding using the given byte
   order::

      *byteorder == -1: little endian
      *byteorder == 0:  native order
      *byteorder == 1:  big endian

   and then switches if the first two bytes of the input data are a byte order mark
   (BOM) and the specified byte order is native order.  This BOM is not copied into
   the resulting Unicode string.  After completion, *\*byteorder* is set to the
   current byte order at the end of input data.

   If *byteorder* is *NULL*, the codec starts in native order mode.

   Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_DecodeUTF16Stateful(const char *s, Py_ssize_t size, const char *errors, int *byteorder, Py_ssize_t *consumed)

   If *consumed* is *NULL*, behave like :cfunc:`PyUnicode_DecodeUTF16`. If
   *consumed* is not *NULL*, :cfunc:`PyUnicode_DecodeUTF16Stateful` will not treat
   trailing incomplete UTF-16 byte sequences (such as an odd number of bytes or a
   split surrogate pair) as an error. Those bytes will not be decoded and the
   number of bytes that have been decoded will be stored in *consumed*.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyUnicode_EncodeUTF16(const Py_UNICODE *s, Py_ssize_t size, const char *errors, int byteorder)

   Return a Python string object holding the UTF-16 encoded value of the Unicode
   data in *s*.  If *byteorder* is not ``0``, output is written according to the
   following byte order::

      byteorder == -1: little endian
      byteorder == 0:  native byte order (writes a BOM mark)
      byteorder == 1:  big endian

   If byteorder is ``0``, the output string will always start with the Unicode BOM
   mark (U+FEFF). In the other two modes, no BOM mark is prepended.

   If *Py_UNICODE_WIDE* is defined, a single :ctype:`Py_UNICODE` value may get
   represented as a surrogate pair. If it is not defined, each :ctype:`Py_UNICODE`
   values is interpreted as an UCS-2 character.

   Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsUTF16String(PyObject *unicode)

   Return a Python string using the UTF-16 encoding in native byte order. The
   string always starts with a BOM mark.  Error handling is "strict".  Return
   *NULL* if an exception was raised by the codec.

These are the "Unicode Escape" codec APIs:

.. % --- Unicode-Escape Codecs ----------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeUnicodeEscape(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the Unicode-Escape encoded
   string *s*.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_EncodeUnicodeEscape(const Py_UNICODE *s, Py_ssize_t size)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using Unicode-Escape and
   return a Python string object.  Return *NULL* if an exception was raised by the
   codec.


.. cfunction:: PyObject* PyUnicode_AsUnicodeEscapeString(PyObject *unicode)

   Encode a Unicode objects using Unicode-Escape and return the result as Python
   string object.  Error handling is "strict". Return *NULL* if an exception was
   raised by the codec.

These are the "Raw Unicode Escape" codec APIs:

.. % --- Raw-Unicode-Escape Codecs ------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeRawUnicodeEscape(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the Raw-Unicode-Escape
   encoded string *s*.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_EncodeRawUnicodeEscape(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using Raw-Unicode-Escape
   and return a Python string object.  Return *NULL* if an exception was raised by
   the codec.


.. cfunction:: PyObject* PyUnicode_AsRawUnicodeEscapeString(PyObject *unicode)

   Encode a Unicode objects using Raw-Unicode-Escape and return the result as
   Python string object. Error handling is "strict". Return *NULL* if an exception
   was raised by the codec.

These are the Latin-1 codec APIs: Latin-1 corresponds to the first 256 Unicode
ordinals and only these are accepted by the codecs during encoding.

.. % --- Latin-1 Codecs -----------------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeLatin1(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the Latin-1 encoded string
   *s*.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_EncodeLatin1(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using Latin-1 and return
   a Python string object.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsLatin1String(PyObject *unicode)

   Encode a Unicode objects using Latin-1 and return the result as Python string
   object.  Error handling is "strict".  Return *NULL* if an exception was raised
   by the codec.

These are the ASCII codec APIs.  Only 7-bit ASCII data is accepted. All other
codes generate errors.

.. % --- ASCII Codecs -------------------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeASCII(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the ASCII encoded string
   *s*.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_EncodeASCII(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using ASCII and return a
   Python string object.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsASCIIString(PyObject *unicode)

   Encode a Unicode objects using ASCII and return the result as Python string
   object.  Error handling is "strict".  Return *NULL* if an exception was raised
   by the codec.

These are the mapping codec APIs:

.. % --- Character Map Codecs -----------------------------------------------

This codec is special in that it can be used to implement many different codecs
(and this is in fact what was done to obtain most of the standard codecs
included in the :mod:`encodings` package). The codec uses mapping to encode and
decode characters.

Decoding mappings must map single string characters to single Unicode
characters, integers (which are then interpreted as Unicode ordinals) or None
(meaning "undefined mapping" and causing an error).

Encoding mappings must map single Unicode characters to single string
characters, integers (which are then interpreted as Latin-1 ordinals) or None
(meaning "undefined mapping" and causing an error).

The mapping objects provided must only support the __getitem__ mapping
interface.

If a character lookup fails with a LookupError, the character is copied as-is
meaning that its ordinal value will be interpreted as Unicode or Latin-1 ordinal
resp. Because of this, mappings only need to contain those mappings which map
characters to different code points.


.. cfunction:: PyObject* PyUnicode_DecodeCharmap(const char *s, Py_ssize_t size, PyObject *mapping, const char *errors)

   Create a Unicode object by decoding *size* bytes of the encoded string *s* using
   the given *mapping* object.  Return *NULL* if an exception was raised by the
   codec. If *mapping* is *NULL* latin-1 decoding will be done. Else it can be a
   dictionary mapping byte or a unicode string, which is treated as a lookup table.
   Byte values greater that the length of the string and U+FFFE "characters" are
   treated as "undefined mapping".

   .. versionchanged:: 2.4
      Allowed unicode string as mapping argument.


.. cfunction:: PyObject* PyUnicode_EncodeCharmap(const Py_UNICODE *s, Py_ssize_t size, PyObject *mapping, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using the given
   *mapping* object and return a Python string object. Return *NULL* if an
   exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsCharmapString(PyObject *unicode, PyObject *mapping)

   Encode a Unicode objects using the given *mapping* object and return the result
   as Python string object.  Error handling is "strict".  Return *NULL* if an
   exception was raised by the codec.

The following codec API is special in that maps Unicode to Unicode.


.. cfunction:: PyObject* PyUnicode_TranslateCharmap(const Py_UNICODE *s, Py_ssize_t size, PyObject *table, const char *errors)

   Translate a :ctype:`Py_UNICODE` buffer of the given length by applying a
   character mapping *table* to it and return the resulting Unicode object.  Return
   *NULL* when an exception was raised by the codec.

   The *mapping* table must map Unicode ordinal integers to Unicode ordinal
   integers or None (causing deletion of the character).

   Mapping tables need only provide the :meth:`__getitem__` interface; dictionaries
   and sequences work well.  Unmapped character ordinals (ones which cause a
   :exc:`LookupError`) are left untouched and are copied as-is.

These are the MBCS codec APIs. They are currently only available on Windows and
use the Win32 MBCS converters to implement the conversions.  Note that MBCS (or
DBCS) is a class of encodings, not just one.  The target encoding is defined by
the user settings on the machine running the codec.

.. % --- MBCS codecs for Windows --------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeMBCS(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the MBCS encoded string *s*.
   Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_DecodeMBCSStateful(const char *s, int size, const char *errors, int *consumed)

   If *consumed* is *NULL*, behave like :cfunc:`PyUnicode_DecodeMBCS`. If
   *consumed* is not *NULL*, :cfunc:`PyUnicode_DecodeMBCSStateful` will not decode
   trailing lead byte and the number of bytes that have been decoded will be stored
   in *consumed*.

   .. versionadded:: 2.5


.. cfunction:: PyObject* PyUnicode_EncodeMBCS(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using MBCS and return a
   Python string object.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsMBCSString(PyObject *unicode)

   Encode a Unicode objects using MBCS and return the result as Python string
   object.  Error handling is "strict".  Return *NULL* if an exception was raised
   by the codec.

.. % --- Methods & Slots ----------------------------------------------------


.. _unicodemethodsandslots:

Methods and Slot Functions
^^^^^^^^^^^^^^^^^^^^^^^^^^

The following APIs are capable of handling Unicode objects and strings on input
(we refer to them as strings in the descriptions) and return Unicode objects or
integers as appropriate.

They all return *NULL* or ``-1`` if an exception occurs.


.. cfunction:: PyObject* PyUnicode_Concat(PyObject *left, PyObject *right)

   Concat two strings giving a new Unicode string.


.. cfunction:: PyObject* PyUnicode_Split(PyObject *s, PyObject *sep, Py_ssize_t maxsplit)

   Split a string giving a list of Unicode strings.  If sep is *NULL*, splitting
   will be done at all whitespace substrings.  Otherwise, splits occur at the given
   separator.  At most *maxsplit* splits will be done.  If negative, no limit is
   set.  Separators are not included in the resulting list.


.. cfunction:: PyObject* PyUnicode_Splitlines(PyObject *s, int keepend)

   Split a Unicode string at line breaks, returning a list of Unicode strings.
   CRLF is considered to be one line break.  If *keepend* is 0, the Line break
   characters are not included in the resulting strings.


.. cfunction:: PyObject* PyUnicode_Translate(PyObject *str, PyObject *table, const char *errors)

   Translate a string by applying a character mapping table to it and return the
   resulting Unicode object.

   The mapping table must map Unicode ordinal integers to Unicode ordinal integers
   or None (causing deletion of the character).

   Mapping tables need only provide the :meth:`__getitem__` interface; dictionaries
   and sequences work well.  Unmapped character ordinals (ones which cause a
   :exc:`LookupError`) are left untouched and are copied as-is.

   *errors* has the usual meaning for codecs. It may be *NULL* which indicates to
   use the default error handling.


.. cfunction:: PyObject* PyUnicode_Join(PyObject *separator, PyObject *seq)

   Join a sequence of strings using the given separator and return the resulting
   Unicode string.


.. cfunction:: int PyUnicode_Tailmatch(PyObject *str, PyObject *substr, Py_ssize_t start, Py_ssize_t end, int direction)

   Return 1 if *substr* matches *str*[*start*:*end*] at the given tail end
   (*direction* == -1 means to do a prefix match, *direction* == 1 a suffix match),
   0 otherwise. Return ``-1`` if an error occurred.


.. cfunction:: Py_ssize_t PyUnicode_Find(PyObject *str, PyObject *substr, Py_ssize_t start, Py_ssize_t end, int direction)

   Return the first position of *substr* in *str*[*start*:*end*] using the given
   *direction* (*direction* == 1 means to do a forward search, *direction* == -1 a
   backward search).  The return value is the index of the first match; a value of
   ``-1`` indicates that no match was found, and ``-2`` indicates that an error
   occurred and an exception has been set.


.. cfunction:: Py_ssize_t PyUnicode_Count(PyObject *str, PyObject *substr, Py_ssize_t start, Py_ssize_t end)

   Return the number of non-overlapping occurrences of *substr* in
   ``str[start:end]``.  Return ``-1`` if an error occurred.


.. cfunction:: PyObject* PyUnicode_Replace(PyObject *str, PyObject *substr, PyObject *replstr, Py_ssize_t maxcount)

   Replace at most *maxcount* occurrences of *substr* in *str* with *replstr* and
   return the resulting Unicode object. *maxcount* == -1 means replace all
   occurrences.


.. cfunction:: int PyUnicode_Compare(PyObject *left, PyObject *right)

   Compare two strings and return -1, 0, 1 for less than, equal, and greater than,
   respectively.


.. cfunction:: int PyUnicode_RichCompare(PyObject *left,  PyObject *right,  int op)

   Rich compare two unicode strings and return one of the following:

   * ``NULL`` in case an exception was raised
   * :const:`Py_True` or :const:`Py_False` for successful comparisons
   * :const:`Py_NotImplemented` in case the type combination is unknown

   Note that :const:`Py_EQ` and :const:`Py_NE` comparisons can cause a
   :exc:`UnicodeWarning` in case the conversion of the arguments to Unicode fails
   with a :exc:`UnicodeDecodeError`.

   Possible values for *op* are :const:`Py_GT`, :const:`Py_GE`, :const:`Py_EQ`,
   :const:`Py_NE`, :const:`Py_LT`, and :const:`Py_LE`.


.. cfunction:: PyObject* PyUnicode_Format(PyObject *format, PyObject *args)

   Return a new string object from *format* and *args*; this is analogous to
   ``format % args``.  The *args* argument must be a tuple.


.. cfunction:: int PyUnicode_Contains(PyObject *container, PyObject *element)

   Check whether *element* is contained in *container* and return true or false
   accordingly.

   *element* has to coerce to a one element Unicode string. ``-1`` is returned if
   there was an error.


.. cfunction:: void PyUnicode_InternInPlace(PyObject **string)

   Intern the argument *\*string* in place.  The argument must be the address of a
   pointer variable pointing to a Python unicode string object.  If there is an
   existing interned string that is the same as *\*string*, it sets *\*string* to
   it (decrementing the reference count of the old string object and incrementing
   the reference count of the interned string object), otherwise it leaves
   *\*string* alone and interns it (incrementing its reference count).
   (Clarification: even though there is a lot of talk about reference counts, think
   of this function as reference-count-neutral; you own the object after the call
   if and only if you owned it before the call.)


.. cfunction:: PyObject* PyUnicode_InternFromString(const char *v)

   A combination of :cfunc:`PyUnicode_FromString` and
   :cfunc:`PyUnicode_InternInPlace`, returning either a new unicode string object
   that has been interned, or a new ("owned") reference to an earlier interned
   string object with the same value.


.. _bufferobjects:

Buffer Objects
--------------

.. sectionauthor:: Greg Stein <gstein@lyra.org>


.. index::
   object: buffer
   single: buffer interface

Python objects implemented in C can export a group of functions called the
"buffer interface."  These functions can be used by an object to expose its data
in a raw, byte-oriented format. Clients of the object can use the buffer
interface to access the object data directly, without needing to copy it first.

Two examples of objects that support the buffer interface are strings and
arrays. The string object exposes the character contents in the buffer
interface's byte-oriented form. An array can also expose its contents, but it
should be noted that array elements may be multi-byte values.

An example user of the buffer interface is the file object's :meth:`write`
method. Any object that can export a series of bytes through the buffer
interface can be written to a file. There are a number of format codes to
:cfunc:`PyArg_ParseTuple` that operate against an object's buffer interface,
returning data from the target object.

.. index:: single: PyBufferProcs

More information on the buffer interface is provided in the section 
:ref:`buffer-structs`, under the description for :ctype:`PyBufferProcs`.

A "buffer object" is defined in the :file:`bufferobject.h` header (included by
:file:`Python.h`). These objects look very similar to string objects at the
Python programming level: they support slicing, indexing, concatenation, and
some other standard string operations. However, their data can come from one of
two sources: from a block of memory, or from another object which exports the
buffer interface.

Buffer objects are useful as a way to expose the data from another object's
buffer interface to the Python programmer. They can also be used as a zero-copy
slicing mechanism. Using their ability to reference a block of memory, it is
possible to expose any data to the Python programmer quite easily. The memory
could be a large, constant array in a C extension, it could be a raw block of
memory for manipulation before passing to an operating system library, or it
could be used to pass around structured data in its native, in-memory format.


.. ctype:: PyBufferObject

   This subtype of :ctype:`PyObject` represents a buffer object.


.. cvar:: PyTypeObject PyBuffer_Type

   .. index:: single: BufferType (in module types)

   The instance of :ctype:`PyTypeObject` which represents the Python buffer type;
   it is the same object as ``buffer`` and  ``types.BufferType`` in the Python
   layer. .


.. cvar:: int Py_END_OF_BUFFER

   This constant may be passed as the *size* parameter to
   :cfunc:`PyBuffer_FromObject` or :cfunc:`PyBuffer_FromReadWriteObject`.  It
   indicates that the new :ctype:`PyBufferObject` should refer to *base* object
   from the specified *offset* to the end of its exported buffer.  Using this
   enables the caller to avoid querying the *base* object for its length.


.. cfunction:: int PyBuffer_Check(PyObject *p)

   Return true if the argument has type :cdata:`PyBuffer_Type`.


.. cfunction:: PyObject* PyBuffer_FromObject(PyObject *base, Py_ssize_t offset, Py_ssize_t size)

   Return a new read-only buffer object.  This raises :exc:`TypeError` if *base*
   doesn't support the read-only buffer protocol or doesn't provide exactly one
   buffer segment, or it raises :exc:`ValueError` if *offset* is less than zero.
   The buffer will hold a reference to the *base* object, and the buffer's contents
   will refer to the *base* object's buffer interface, starting as position
   *offset* and extending for *size* bytes. If *size* is :const:`Py_END_OF_BUFFER`,
   then the new buffer's contents extend to the length of the *base* object's
   exported buffer data.


.. cfunction:: PyObject* PyBuffer_FromReadWriteObject(PyObject *base, Py_ssize_t offset, Py_ssize_t size)

   Return a new writable buffer object.  Parameters and exceptions are similar to
   those for :cfunc:`PyBuffer_FromObject`.  If the *base* object does not export
   the writeable buffer protocol, then :exc:`TypeError` is raised.


.. cfunction:: PyObject* PyBuffer_FromMemory(void *ptr, Py_ssize_t size)

   Return a new read-only buffer object that reads from a specified location in
   memory, with a specified size.  The caller is responsible for ensuring that the
   memory buffer, passed in as *ptr*, is not deallocated while the returned buffer
   object exists.  Raises :exc:`ValueError` if *size* is less than zero.  Note that
   :const:`Py_END_OF_BUFFER` may *not* be passed for the *size* parameter;
   :exc:`ValueError` will be raised in that case.


.. cfunction:: PyObject* PyBuffer_FromReadWriteMemory(void *ptr, Py_ssize_t size)

   Similar to :cfunc:`PyBuffer_FromMemory`, but the returned buffer is writable.


.. cfunction:: PyObject* PyBuffer_New(Py_ssize_t size)

   Return a new writable buffer object that maintains its own memory buffer of
   *size* bytes.  :exc:`ValueError` is returned if *size* is not zero or positive.
   Note that the memory buffer (as returned by :cfunc:`PyObject_AsWriteBuffer`) is
   not specifically aligned.


.. _tupleobjects:

Tuple Objects
-------------

.. index:: object: tuple


.. ctype:: PyTupleObject

   This subtype of :ctype:`PyObject` represents a Python tuple object.


.. cvar:: PyTypeObject PyTuple_Type

   .. index:: single: TupleType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python tuple type; it is
   the same object as ``tuple`` and ``types.TupleType`` in the Python layer..


.. cfunction:: int PyTuple_Check(PyObject *p)

   Return true if *p* is a tuple object or an instance of a subtype of the tuple
   type.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyTuple_CheckExact(PyObject *p)

   Return true if *p* is a tuple object, but not an instance of a subtype of the
   tuple type.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyTuple_New(Py_ssize_t len)

   Return a new tuple object of size *len*, or *NULL* on failure.


.. cfunction:: PyObject* PyTuple_Pack(Py_ssize_t n, ...)

   Return a new tuple object of size *n*, or *NULL* on failure. The tuple values
   are initialized to the subsequent *n* C arguments pointing to Python objects.
   ``PyTuple_Pack(2, a, b)`` is equivalent to ``Py_BuildValue("(OO)", a, b)``.

   .. versionadded:: 2.4


.. cfunction:: int PyTuple_Size(PyObject *p)

   Take a pointer to a tuple object, and return the size of that tuple.


.. cfunction:: int PyTuple_GET_SIZE(PyObject *p)

   Return the size of the tuple *p*, which must be non-*NULL* and point to a tuple;
   no error checking is performed.


.. cfunction:: PyObject* PyTuple_GetItem(PyObject *p, Py_ssize_t pos)

   Return the object at position *pos* in the tuple pointed to by *p*.  If *pos* is
   out of bounds, return *NULL* and sets an :exc:`IndexError` exception.


.. cfunction:: PyObject* PyTuple_GET_ITEM(PyObject *p, Py_ssize_t pos)

   Like :cfunc:`PyTuple_GetItem`, but does no checking of its arguments.


.. cfunction:: PyObject* PyTuple_GetSlice(PyObject *p, Py_ssize_t low, Py_ssize_t high)

   Take a slice of the tuple pointed to by *p* from *low* to *high* and return it
   as a new tuple.


.. cfunction:: int PyTuple_SetItem(PyObject *p, Py_ssize_t pos, PyObject *o)

   Insert a reference to object *o* at position *pos* of the tuple pointed to by
   *p*. Return ``0`` on success.

   .. note::

      This function "steals" a reference to *o*.


.. cfunction:: void PyTuple_SET_ITEM(PyObject *p, Py_ssize_t pos, PyObject *o)

   Like :cfunc:`PyTuple_SetItem`, but does no error checking, and should *only* be
   used to fill in brand new tuples.

   .. note::

      This function "steals" a reference to *o*.


.. cfunction:: int _PyTuple_Resize(PyObject **p, Py_ssize_t newsize)

   Can be used to resize a tuple.  *newsize* will be the new length of the tuple.
   Because tuples are *supposed* to be immutable, this should only be used if there
   is only one reference to the object.  Do *not* use this if the tuple may already
   be known to some other part of the code.  The tuple will always grow or shrink
   at the end.  Think of this as destroying the old tuple and creating a new one,
   only more efficiently.  Returns ``0`` on success. Client code should never
   assume that the resulting value of ``*p`` will be the same as before calling
   this function. If the object referenced by ``*p`` is replaced, the original
   ``*p`` is destroyed.  On failure, returns ``-1`` and sets ``*p`` to *NULL*, and
   raises :exc:`MemoryError` or :exc:`SystemError`.

   .. versionchanged:: 2.2
      Removed unused third parameter, *last_is_sticky*.


.. _listobjects:

List Objects
------------

.. index:: object: list


.. ctype:: PyListObject

   This subtype of :ctype:`PyObject` represents a Python list object.


.. cvar:: PyTypeObject PyList_Type

   .. index:: single: ListType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python list type.  This is
   the same object as ``list`` and ``types.ListType`` in the Python layer.


.. cfunction:: int PyList_Check(PyObject *p)

   Return true if *p* is a list object or an instance of a subtype of the list
   type.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyList_CheckExact(PyObject *p)

   Return true if *p* is a list object, but not an instance of a subtype of the
   list type.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyList_New(Py_ssize_t len)

   Return a new list of length *len* on success, or *NULL* on failure.

   .. note::

      If *length* is greater than zero, the returned list object's items are set to
      ``NULL``.  Thus you cannot use abstract API functions such as
      :cfunc:`PySequence_SetItem`  or expose the object to Python code before setting
      all items to a real object with :cfunc:`PyList_SetItem`.


.. cfunction:: Py_ssize_t PyList_Size(PyObject *list)

   .. index:: builtin: len

   Return the length of the list object in *list*; this is equivalent to
   ``len(list)`` on a list object.


.. cfunction:: Py_ssize_t PyList_GET_SIZE(PyObject *list)

   Macro form of :cfunc:`PyList_Size` without error checking.


.. cfunction:: PyObject* PyList_GetItem(PyObject *list, Py_ssize_t index)

   Return the object at position *pos* in the list pointed to by *p*.  The position
   must be positive, indexing from the end of the list is not supported.  If *pos*
   is out of bounds, return *NULL* and set an :exc:`IndexError` exception.


.. cfunction:: PyObject* PyList_GET_ITEM(PyObject *list, Py_ssize_t i)

   Macro form of :cfunc:`PyList_GetItem` without error checking.


.. cfunction:: int PyList_SetItem(PyObject *list, Py_ssize_t index, PyObject *item)

   Set the item at index *index* in list to *item*.  Return ``0`` on success or
   ``-1`` on failure.

   .. note::

      This function "steals" a reference to *item* and discards a reference to an item
      already in the list at the affected position.


.. cfunction:: void PyList_SET_ITEM(PyObject *list, Py_ssize_t i, PyObject *o)

   Macro form of :cfunc:`PyList_SetItem` without error checking. This is normally
   only used to fill in new lists where there is no previous content.

   .. note::

      This function "steals" a reference to *item*, and, unlike
      :cfunc:`PyList_SetItem`, does *not* discard a reference to any item that it
      being replaced; any reference in *list* at position *i* will be leaked.


.. cfunction:: int PyList_Insert(PyObject *list, Py_ssize_t index, PyObject *item)

   Insert the item *item* into list *list* in front of index *index*.  Return ``0``
   if successful; return ``-1`` and set an exception if unsuccessful.  Analogous to
   ``list.insert(index, item)``.


.. cfunction:: int PyList_Append(PyObject *list, PyObject *item)

   Append the object *item* at the end of list *list*. Return ``0`` if successful;
   return ``-1`` and set an exception if unsuccessful.  Analogous to
   ``list.append(item)``.


.. cfunction:: PyObject* PyList_GetSlice(PyObject *list, Py_ssize_t low, Py_ssize_t high)

   Return a list of the objects in *list* containing the objects *between* *low*
   and *high*.  Return *NULL* and set an exception if unsuccessful. Analogous to
   ``list[low:high]``.


.. cfunction:: int PyList_SetSlice(PyObject *list, Py_ssize_t low, Py_ssize_t high, PyObject *itemlist)

   Set the slice of *list* between *low* and *high* to the contents of *itemlist*.
   Analogous to ``list[low:high] = itemlist``. The *itemlist* may be *NULL*,
   indicating the assignment of an empty list (slice deletion). Return ``0`` on
   success, ``-1`` on failure.


.. cfunction:: int PyList_Sort(PyObject *list)

   Sort the items of *list* in place.  Return ``0`` on success, ``-1`` on failure.
   This is equivalent to ``list.sort()``.


.. cfunction:: int PyList_Reverse(PyObject *list)

   Reverse the items of *list* in place.  Return ``0`` on success, ``-1`` on
   failure.  This is the equivalent of ``list.reverse()``.


.. cfunction:: PyObject* PyList_AsTuple(PyObject *list)

   .. index:: builtin: tuple

   Return a new tuple object containing the contents of *list*; equivalent to
   ``tuple(list)``.


.. _mapobjects:

Mapping Objects
===============

.. index:: object: mapping


.. _dictobjects:

Dictionary Objects
------------------

.. index:: object: dictionary


.. ctype:: PyDictObject

   This subtype of :ctype:`PyObject` represents a Python dictionary object.


.. cvar:: PyTypeObject PyDict_Type

   .. index::
      single: DictType (in module types)
      single: DictionaryType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python dictionary type.
   This is exposed to Python programs as ``dict`` and ``types.DictType``.


.. cfunction:: int PyDict_Check(PyObject *p)

   Return true if *p* is a dict object or an instance of a subtype of the dict
   type.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyDict_CheckExact(PyObject *p)

   Return true if *p* is a dict object, but not an instance of a subtype of the
   dict type.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyDict_New()

   Return a new empty dictionary, or *NULL* on failure.


.. cfunction:: PyObject* PyDictProxy_New(PyObject *dict)

   Return a proxy object for a mapping which enforces read-only behavior.  This is
   normally used to create a proxy to prevent modification of the dictionary for
   non-dynamic class types.

   .. versionadded:: 2.2


.. cfunction:: void PyDict_Clear(PyObject *p)

   Empty an existing dictionary of all key-value pairs.


.. cfunction:: int PyDict_Contains(PyObject *p, PyObject *key)

   Determine if dictionary *p* contains *key*.  If an item in *p* is matches *key*,
   return ``1``, otherwise return ``0``.  On error, return ``-1``.  This is
   equivalent to the Python expression ``key in p``.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyDict_Copy(PyObject *p)

   Return a new dictionary that contains the same key-value pairs as *p*.

   .. versionadded:: 1.6


.. cfunction:: int PyDict_SetItem(PyObject *p, PyObject *key, PyObject *val)

   Insert *value* into the dictionary *p* with a key of *key*.  *key* must be
   hashable; if it isn't, :exc:`TypeError` will be raised. Return ``0`` on success
   or ``-1`` on failure.


.. cfunction:: int PyDict_SetItemString(PyObject *p, const char *key, PyObject *val)

   .. index:: single: PyString_FromString()

   Insert *value* into the dictionary *p* using *key* as a key. *key* should be a
   :ctype:`char\*`.  The key object is created using ``PyString_FromString(key)``.
   Return ``0`` on success or ``-1`` on failure.


.. cfunction:: int PyDict_DelItem(PyObject *p, PyObject *key)

   Remove the entry in dictionary *p* with key *key*. *key* must be hashable; if it
   isn't, :exc:`TypeError` is raised.  Return ``0`` on success or ``-1`` on
   failure.


.. cfunction:: int PyDict_DelItemString(PyObject *p, char *key)

   Remove the entry in dictionary *p* which has a key specified by the string
   *key*.  Return ``0`` on success or ``-1`` on failure.


.. cfunction:: PyObject* PyDict_GetItem(PyObject *p, PyObject *key)

   Return the object from dictionary *p* which has a key *key*.  Return *NULL* if
   the key *key* is not present, but *without* setting an exception.


.. cfunction:: PyObject* PyDict_GetItemString(PyObject *p, const char *key)

   This is the same as :cfunc:`PyDict_GetItem`, but *key* is specified as a
   :ctype:`char\*`, rather than a :ctype:`PyObject\*`.


.. cfunction:: PyObject* PyDict_Items(PyObject *p)

   Return a :ctype:`PyListObject` containing all the items from the dictionary, as
   in the dictionary method :meth:`dict.items`.


.. cfunction:: PyObject* PyDict_Keys(PyObject *p)

   Return a :ctype:`PyListObject` containing all the keys from the dictionary, as
   in the dictionary method :meth:`dict.keys`.


.. cfunction:: PyObject* PyDict_Values(PyObject *p)

   Return a :ctype:`PyListObject` containing all the values from the dictionary
   *p*, as in the dictionary method :meth:`dict.values`.


.. cfunction:: Py_ssize_t PyDict_Size(PyObject *p)

   .. index:: builtin: len

   Return the number of items in the dictionary.  This is equivalent to ``len(p)``
   on a dictionary.


.. cfunction:: int PyDict_Next(PyObject *p, Py_ssize_t *ppos, PyObject **pkey, PyObject **pvalue)

   Iterate over all key-value pairs in the dictionary *p*.  The :ctype:`int`
   referred to by *ppos* must be initialized to ``0`` prior to the first call to
   this function to start the iteration; the function returns true for each pair in
   the dictionary, and false once all pairs have been reported.  The parameters
   *pkey* and *pvalue* should either point to :ctype:`PyObject\*` variables that
   will be filled in with each key and value, respectively, or may be *NULL*.  Any
   references returned through them are borrowed.  *ppos* should not be altered
   during iteration. Its value represents offsets within the internal dictionary
   structure, and since the structure is sparse, the offsets are not consecutive.

   For example::

      PyObject *key, *value;
      Py_ssize_t pos = 0;

      while (PyDict_Next(self->dict, &pos, &key, &value)) {
          /* do something interesting with the values... */
          ...
      }

   The dictionary *p* should not be mutated during iteration.  It is safe (since
   Python 2.1) to modify the values of the keys as you iterate over the dictionary,
   but only so long as the set of keys does not change.  For example::

      PyObject *key, *value;
      Py_ssize_t pos = 0;

      while (PyDict_Next(self->dict, &pos, &key, &value)) {
          int i = PyInt_AS_LONG(value) + 1;
          PyObject *o = PyInt_FromLong(i);
          if (o == NULL)
              return -1;
          if (PyDict_SetItem(self->dict, key, o) < 0) {
              Py_DECREF(o);
              return -1;
          }
          Py_DECREF(o);
      }


.. cfunction:: int PyDict_Merge(PyObject *a, PyObject *b, int override)

   Iterate over mapping object *b* adding key-value pairs to dictionary *a*. *b*
   may be a dictionary, or any object supporting :func:`PyMapping_Keys` and
   :func:`PyObject_GetItem`. If *override* is true, existing pairs in *a* will be
   replaced if a matching key is found in *b*, otherwise pairs will only be added
   if there is not a matching key in *a*. Return ``0`` on success or ``-1`` if an
   exception was raised.

   .. versionadded:: 2.2


.. cfunction:: int PyDict_Update(PyObject *a, PyObject *b)

   This is the same as ``PyDict_Merge(a, b, 1)`` in C, or ``a.update(b)`` in
   Python.  Return ``0`` on success or ``-1`` if an exception was raised.

   .. versionadded:: 2.2


.. cfunction:: int PyDict_MergeFromSeq2(PyObject *a, PyObject *seq2, int override)

   Update or merge into dictionary *a*, from the key-value pairs in *seq2*.  *seq2*
   must be an iterable object producing iterable objects of length 2, viewed as
   key-value pairs.  In case of duplicate keys, the last wins if *override* is
   true, else the first wins. Return ``0`` on success or ``-1`` if an exception was
   raised. Equivalent Python (except for the return value)::

      def PyDict_MergeFromSeq2(a, seq2, override):
          for key, value in seq2:
              if override or key not in a:
                  a[key] = value

   .. versionadded:: 2.2


.. _otherobjects:

Other Objects
=============


.. _classobjects:

Class Objects
-------------

.. index:: object: class

Note that the class objects described here represent old-style classes, which
will go away in Python 3. When creating new types for extension modules, you
will want to work with type objects (section :ref:`typeobjects`).


.. ctype:: PyClassObject

   The C structure of the objects used to describe built-in classes.


.. cvar:: PyObject* PyClass_Type

   .. index:: single: ClassType (in module types)

   This is the type object for class objects; it is the same object as
   ``types.ClassType`` in the Python layer.


.. cfunction:: int PyClass_Check(PyObject *o)

   Return true if the object *o* is a class object, including instances of types
   derived from the standard class object.  Return false in all other cases.


.. cfunction:: int PyClass_IsSubclass(PyObject *klass, PyObject *base)

   Return true if *klass* is a subclass of *base*. Return false in all other cases.


.. _fileobjects:

File Objects
------------

.. index:: object: file

Python's built-in file objects are implemented entirely on the :ctype:`FILE\*`
support from the C standard library.  This is an implementation detail and may
change in future releases of Python.


.. ctype:: PyFileObject

   This subtype of :ctype:`PyObject` represents a Python file object.


.. cvar:: PyTypeObject PyFile_Type

   .. index:: single: FileType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python file type.  This is
   exposed to Python programs as ``file`` and ``types.FileType``.


.. cfunction:: int PyFile_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyFileObject` or a subtype of
   :ctype:`PyFileObject`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyFile_CheckExact(PyObject *p)

   Return true if its argument is a :ctype:`PyFileObject`, but not a subtype of
   :ctype:`PyFileObject`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyFile_FromString(char *filename, char *mode)

   .. index:: single: fopen()

   On success, return a new file object that is opened on the file given by
   *filename*, with a file mode given by *mode*, where *mode* has the same
   semantics as the standard C routine :cfunc:`fopen`.  On failure, return *NULL*.


.. cfunction:: PyObject* PyFile_FromFile(FILE *fp, char *name, char *mode, int (*close)(FILE*))

   Create a new :ctype:`PyFileObject` from the already-open standard C file
   pointer, *fp*.  The function *close* will be called when the file should be
   closed.  Return *NULL* on failure.


.. cfunction:: FILE* PyFile_AsFile(PyObject *p)

   Return the file object associated with *p* as a :ctype:`FILE\*`.


.. cfunction:: PyObject* PyFile_GetLine(PyObject *p, int n)

   .. index:: single: EOFError (built-in exception)

   Equivalent to ``p.readline([n])``, this function reads one line from the
   object *p*.  *p* may be a file object or any object with a :meth:`readline`
   method.  If *n* is ``0``, exactly one line is read, regardless of the length of
   the line.  If *n* is greater than ``0``, no more than *n* bytes will be read
   from the file; a partial line can be returned.  In both cases, an empty string
   is returned if the end of the file is reached immediately.  If *n* is less than
   ``0``, however, one line is read regardless of length, but :exc:`EOFError` is
   raised if the end of the file is reached immediately.


.. cfunction:: PyObject* PyFile_Name(PyObject *p)

   Return the name of the file specified by *p* as a string object.


.. cfunction:: void PyFile_SetBufSize(PyFileObject *p, int n)

   .. index:: single: setvbuf()

   Available on systems with :cfunc:`setvbuf` only.  This should only be called
   immediately after file object creation.


.. cfunction:: int PyFile_Encoding(PyFileObject *p, char *enc)

   Set the file's encoding for Unicode output to *enc*. Return 1 on success and 0
   on failure.

   .. versionadded:: 2.3


.. cfunction:: int PyFile_SoftSpace(PyObject *p, int newflag)

   .. index:: single: softspace (file attribute)

   This function exists for internal use by the interpreter.  Set the
   :attr:`softspace` attribute of *p* to *newflag* and return the previous value.
   *p* does not have to be a file object for this function to work properly; any
   object is supported (thought its only interesting if the :attr:`softspace`
   attribute can be set).  This function clears any errors, and will return ``0``
   as the previous value if the attribute either does not exist or if there were
   errors in retrieving it.  There is no way to detect errors from this function,
   but doing so should not be needed.


.. cfunction:: int PyFile_WriteObject(PyObject *obj, PyObject *p, int flags)

   .. index:: single: Py_PRINT_RAW

   Write object *obj* to file object *p*.  The only supported flag for *flags* is
   :const:`Py_PRINT_RAW`; if given, the :func:`str` of the object is written
   instead of the :func:`repr`.  Return ``0`` on success or ``-1`` on failure; the
   appropriate exception will be set.


.. cfunction:: int PyFile_WriteString(const char *s, PyObject *p)

   Write string *s* to file object *p*.  Return ``0`` on success or ``-1`` on
   failure; the appropriate exception will be set.


.. _instanceobjects:

Instance Objects
----------------

.. index:: object: instance

There are very few functions specific to instance objects.


.. cvar:: PyTypeObject PyInstance_Type

   Type object for class instances.


.. cfunction:: int PyInstance_Check(PyObject *obj)

   Return true if *obj* is an instance.


.. cfunction:: PyObject* PyInstance_New(PyObject *class, PyObject *arg, PyObject *kw)

   Create a new instance of a specific class.  The parameters *arg* and *kw* are
   used as the positional and keyword parameters to the object's constructor.


.. cfunction:: PyObject* PyInstance_NewRaw(PyObject *class, PyObject *dict)

   Create a new instance of a specific class without calling its constructor.
   *class* is the class of new object.  The *dict* parameter will be used as the
   object's :attr:`__dict__`; if *NULL*, a new dictionary will be created for the
   instance.


.. _function-objects:

Function Objects
----------------

.. index:: object: function

There are a few functions specific to Python functions.


.. ctype:: PyFunctionObject

   The C structure used for functions.


.. cvar:: PyTypeObject PyFunction_Type

   .. index:: single: MethodType (in module types)

   This is an instance of :ctype:`PyTypeObject` and represents the Python function
   type.  It is exposed to Python programmers as ``types.FunctionType``.


.. cfunction:: int PyFunction_Check(PyObject *o)

   Return true if *o* is a function object (has type :cdata:`PyFunction_Type`).
   The parameter must not be *NULL*.


.. cfunction:: PyObject* PyFunction_New(PyObject *code, PyObject *globals)

   Return a new function object associated with the code object *code*. *globals*
   must be a dictionary with the global variables accessible to the function.

   The function's docstring, name and *__module__* are retrieved from the code
   object, the argument defaults and closure are set to *NULL*.


.. cfunction:: PyObject* PyFunction_GetCode(PyObject *op)

   Return the code object associated with the function object *op*.


.. cfunction:: PyObject* PyFunction_GetGlobals(PyObject *op)

   Return the globals dictionary associated with the function object *op*.


.. cfunction:: PyObject* PyFunction_GetModule(PyObject *op)

   Return the *__module__* attribute of the function object *op*. This is normally
   a string containing the module name, but can be set to any other object by
   Python code.


.. cfunction:: PyObject* PyFunction_GetDefaults(PyObject *op)

   Return the argument default values of the function object *op*. This can be a
   tuple of arguments or *NULL*.


.. cfunction:: int PyFunction_SetDefaults(PyObject *op, PyObject *defaults)

   Set the argument default values for the function object *op*. *defaults* must be
   *Py_None* or a tuple.

   Raises :exc:`SystemError` and returns ``-1`` on failure.


.. cfunction:: PyObject* PyFunction_GetClosure(PyObject *op)

   Return the closure associated with the function object *op*. This can be *NULL*
   or a tuple of cell objects.


.. cfunction:: int PyFunction_SetClosure(PyObject *op, PyObject *closure)

   Set the closure associated with the function object *op*. *closure* must be
   *Py_None* or a tuple of cell objects.

   Raises :exc:`SystemError` and returns ``-1`` on failure.


.. _method-objects:

Method Objects
--------------

.. index:: object: method

There are some useful functions that are useful for working with method objects.


.. cvar:: PyTypeObject PyMethod_Type

   .. index:: single: MethodType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python method type.  This
   is exposed to Python programs as ``types.MethodType``.


.. cfunction:: int PyMethod_Check(PyObject *o)

   Return true if *o* is a method object (has type :cdata:`PyMethod_Type`).  The
   parameter must not be *NULL*.


.. cfunction:: PyObject* PyMethod_New(PyObject *func, PyObject *self, PyObject *class)

   Return a new method object, with *func* being any callable object; this is the
   function that will be called when the method is called.  If this method should
   be bound to an instance, *self* should be the instance and *class* should be the
   class of *self*, otherwise *self* should be *NULL* and *class* should be the
   class which provides the unbound method..


.. cfunction:: PyObject* PyMethod_Class(PyObject *meth)

   Return the class object from which the method *meth* was created; if this was
   created from an instance, it will be the class of the instance.


.. cfunction:: PyObject* PyMethod_GET_CLASS(PyObject *meth)

   Macro version of :cfunc:`PyMethod_Class` which avoids error checking.


.. cfunction:: PyObject* PyMethod_Function(PyObject *meth)

   Return the function object associated with the method *meth*.


.. cfunction:: PyObject* PyMethod_GET_FUNCTION(PyObject *meth)

   Macro version of :cfunc:`PyMethod_Function` which avoids error checking.


.. cfunction:: PyObject* PyMethod_Self(PyObject *meth)

   Return the instance associated with the method *meth* if it is bound, otherwise
   return *NULL*.


.. cfunction:: PyObject* PyMethod_GET_SELF(PyObject *meth)

   Macro version of :cfunc:`PyMethod_Self` which avoids error checking.


.. _moduleobjects:

Module Objects
--------------

.. index:: object: module

There are only a few functions special to module objects.


.. cvar:: PyTypeObject PyModule_Type

   .. index:: single: ModuleType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python module type.  This
   is exposed to Python programs as ``types.ModuleType``.


.. cfunction:: int PyModule_Check(PyObject *p)

   Return true if *p* is a module object, or a subtype of a module object.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyModule_CheckExact(PyObject *p)

   Return true if *p* is a module object, but not a subtype of
   :cdata:`PyModule_Type`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyModule_New(const char *name)

   .. index::
      single: __name__ (module attribute)
      single: __doc__ (module attribute)
      single: __file__ (module attribute)

   Return a new module object with the :attr:`__name__` attribute set to *name*.
   Only the module's :attr:`__doc__` and :attr:`__name__` attributes are filled in;
   the caller is responsible for providing a :attr:`__file__` attribute.


.. cfunction:: PyObject* PyModule_GetDict(PyObject *module)

   .. index:: single: __dict__ (module attribute)

   Return the dictionary object that implements *module*'s namespace; this object
   is the same as the :attr:`__dict__` attribute of the module object.  This
   function never fails.  It is recommended extensions use other
   :cfunc:`PyModule_\*` and :cfunc:`PyObject_\*` functions rather than directly
   manipulate a module's :attr:`__dict__`.


.. cfunction:: char* PyModule_GetName(PyObject *module)

   .. index::
      single: __name__ (module attribute)
      single: SystemError (built-in exception)

   Return *module*'s :attr:`__name__` value.  If the module does not provide one,
   or if it is not a string, :exc:`SystemError` is raised and *NULL* is returned.


.. cfunction:: char* PyModule_GetFilename(PyObject *module)

   .. index::
      single: __file__ (module attribute)
      single: SystemError (built-in exception)

   Return the name of the file from which *module* was loaded using *module*'s
   :attr:`__file__` attribute.  If this is not defined, or if it is not a string,
   raise :exc:`SystemError` and return *NULL*.


.. cfunction:: int PyModule_AddObject(PyObject *module, const char *name, PyObject *value)

   Add an object to *module* as *name*.  This is a convenience function which can
   be used from the module's initialization function.  This steals a reference to
   *value*.  Return ``-1`` on error, ``0`` on success.

   .. versionadded:: 2.0


.. cfunction:: int PyModule_AddIntConstant(PyObject *module, const char *name, long value)

   Add an integer constant to *module* as *name*.  This convenience function can be
   used from the module's initialization function. Return ``-1`` on error, ``0`` on
   success.

   .. versionadded:: 2.0


.. cfunction:: int PyModule_AddStringConstant(PyObject *module, const char *name, const char *value)

   Add a string constant to *module* as *name*.  This convenience function can be
   used from the module's initialization function.  The string *value* must be
   null-terminated.  Return ``-1`` on error, ``0`` on success.

   .. versionadded:: 2.0


.. _iterator-objects:

Iterator Objects
----------------

Python provides two general-purpose iterator objects.  The first, a sequence
iterator, works with an arbitrary sequence supporting the :meth:`__getitem__`
method.  The second works with a callable object and a sentinel value, calling
the callable for each item in the sequence, and ending the iteration when the
sentinel value is returned.


.. cvar:: PyTypeObject PySeqIter_Type

   Type object for iterator objects returned by :cfunc:`PySeqIter_New` and the
   one-argument form of the :func:`iter` built-in function for built-in sequence
   types.

   .. versionadded:: 2.2


.. cfunction:: int PySeqIter_Check(op)

   Return true if the type of *op* is :cdata:`PySeqIter_Type`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PySeqIter_New(PyObject *seq)

   Return an iterator that works with a general sequence object, *seq*.  The
   iteration ends when the sequence raises :exc:`IndexError` for the subscripting
   operation.

   .. versionadded:: 2.2


.. cvar:: PyTypeObject PyCallIter_Type

   Type object for iterator objects returned by :cfunc:`PyCallIter_New` and the
   two-argument form of the :func:`iter` built-in function.

   .. versionadded:: 2.2


.. cfunction:: int PyCallIter_Check(op)

   Return true if the type of *op* is :cdata:`PyCallIter_Type`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyCallIter_New(PyObject *callable, PyObject *sentinel)

   Return a new iterator.  The first parameter, *callable*, can be any Python
   callable object that can be called with no parameters; each call to it should
   return the next item in the iteration.  When *callable* returns a value equal to
   *sentinel*, the iteration will be terminated.

   .. versionadded:: 2.2


.. _descriptor-objects:

Descriptor Objects
------------------

"Descriptors" are objects that describe some attribute of an object. They are
found in the dictionary of type objects.


.. cvar:: PyTypeObject PyProperty_Type

   The type object for the built-in descriptor types.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewGetSet(PyTypeObject *type, struct PyGetSetDef *getset)

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewMember(PyTypeObject *type, struct PyMemberDef *meth)

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewMethod(PyTypeObject *type, struct PyMethodDef *meth)

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *wrapper, void *wrapped)

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewClassMethod(PyTypeObject *type, PyMethodDef *method)

   .. versionadded:: 2.3


.. cfunction:: int PyDescr_IsData(PyObject *descr)

   Return true if the descriptor objects *descr* describes a data attribute, or
   false if it describes a method.  *descr* must be a descriptor object; there is
   no error checking.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyWrapper_New(PyObject *, PyObject *)

   .. versionadded:: 2.2


.. _slice-objects:

Slice Objects
-------------


.. cvar:: PyTypeObject PySlice_Type

   .. index:: single: SliceType (in module types)

   The type object for slice objects.  This is the same as ``slice`` and
   ``types.SliceType``.


.. cfunction:: int PySlice_Check(PyObject *ob)

   Return true if *ob* is a slice object; *ob* must not be *NULL*.


.. cfunction:: PyObject* PySlice_New(PyObject *start, PyObject *stop, PyObject *step)

   Return a new slice object with the given values.  The *start*, *stop*, and
   *step* parameters are used as the values of the slice object attributes of the
   same names.  Any of the values may be *NULL*, in which case the ``None`` will be
   used for the corresponding attribute.  Return *NULL* if the new object could not
   be allocated.


.. cfunction:: int PySlice_GetIndices(PySliceObject *slice, Py_ssize_t length, Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step)

   Retrieve the start, stop and step indices from the slice object *slice*,
   assuming a sequence of length *length*. Treats indices greater than *length* as
   errors.

   Returns 0 on success and -1 on error with no exception set (unless one of the
   indices was not :const:`None` and failed to be converted to an integer, in which
   case -1 is returned with an exception set).

   You probably do not want to use this function.  If you want to use slice objects
   in versions of Python prior to 2.3, you would probably do well to incorporate
   the source of :cfunc:`PySlice_GetIndicesEx`, suitably renamed, in the source of
   your extension.


.. cfunction:: int PySlice_GetIndicesEx(PySliceObject *slice, Py_ssize_t length, Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step, Py_ssize_t *slicelength)

   Usable replacement for :cfunc:`PySlice_GetIndices`.  Retrieve the start, stop,
   and step indices from the slice object *slice* assuming a sequence of length
   *length*, and store the length of the slice in *slicelength*.  Out of bounds
   indices are clipped in a manner consistent with the handling of normal slices.

   Returns 0 on success and -1 on error with exception set.

   .. versionadded:: 2.3


.. _weakrefobjects:

Weak Reference Objects
----------------------

Python supports *weak references* as first-class objects.  There are two
specific object types which directly implement weak references.  The first is a
simple reference object, and the second acts as a proxy for the original object
as much as it can.


.. cfunction:: int PyWeakref_Check(ob)

   Return true if *ob* is either a reference or proxy object.

   .. versionadded:: 2.2


.. cfunction:: int PyWeakref_CheckRef(ob)

   Return true if *ob* is a reference object.

   .. versionadded:: 2.2


.. cfunction:: int PyWeakref_CheckProxy(ob)

   Return true if *ob* is a proxy object.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyWeakref_NewRef(PyObject *ob, PyObject *callback)

   Return a weak reference object for the object *ob*.  This will always return
   a new reference, but is not guaranteed to create a new object; an existing
   reference object may be returned.  The second parameter, *callback*, can be a
   callable object that receives notification when *ob* is garbage collected; it
   should accept a single parameter, which will be the weak reference object
   itself. *callback* may also be ``None`` or *NULL*.  If *ob* is not a
   weakly-referencable object, or if *callback* is not callable, ``None``, or
   *NULL*, this will return *NULL* and raise :exc:`TypeError`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyWeakref_NewProxy(PyObject *ob, PyObject *callback)

   Return a weak reference proxy object for the object *ob*.  This will always
   return a new reference, but is not guaranteed to create a new object; an
   existing proxy object may be returned.  The second parameter, *callback*, can
   be a callable object that receives notification when *ob* is garbage
   collected; it should accept a single parameter, which will be the weak
   reference object itself. *callback* may also be ``None`` or *NULL*.  If *ob*
   is not a weakly-referencable object, or if *callback* is not callable,
   ``None``, or *NULL*, this will return *NULL* and raise :exc:`TypeError`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyWeakref_GetObject(PyObject *ref)

   Return the referenced object from a weak reference, *ref*.  If the referent is
   no longer live, returns ``None``.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyWeakref_GET_OBJECT(PyObject *ref)

   Similar to :cfunc:`PyWeakref_GetObject`, but implemented as a macro that does no
   error checking.

   .. versionadded:: 2.2


.. _cobjects:

CObjects
--------

.. index:: object: CObject

Refer to *Extending and Embedding the Python Interpreter*, section 1.12,
"Providing a C API for an Extension Module," for more information on using these
objects.


.. ctype:: PyCObject

   This subtype of :ctype:`PyObject` represents an opaque value, useful for C
   extension modules who need to pass an opaque value (as a :ctype:`void\*`
   pointer) through Python code to other C code.  It is often used to make a C
   function pointer defined in one module available to other modules, so the
   regular import mechanism can be used to access C APIs defined in dynamically
   loaded modules.


.. cfunction:: int PyCObject_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyCObject`.


.. cfunction:: PyObject* PyCObject_FromVoidPtr(void* cobj, void (*destr)(void *))

   Create a :ctype:`PyCObject` from the ``void *`` *cobj*.  The *destr* function
   will be called when the object is reclaimed, unless it is *NULL*.


.. cfunction:: PyObject* PyCObject_FromVoidPtrAndDesc(void* cobj, void* desc, void (*destr)(void *, void *))

   Create a :ctype:`PyCObject` from the :ctype:`void \*` *cobj*.  The *destr*
   function will be called when the object is reclaimed. The *desc* argument can
   be used to pass extra callback data for the destructor function.


.. cfunction:: void* PyCObject_AsVoidPtr(PyObject* self)

   Return the object :ctype:`void \*` that the :ctype:`PyCObject` *self* was
   created with.


.. cfunction:: void* PyCObject_GetDesc(PyObject* self)

   Return the description :ctype:`void \*` that the :ctype:`PyCObject` *self* was
   created with.


.. cfunction:: int PyCObject_SetVoidPtr(PyObject* self, void* cobj)

   Set the void pointer inside *self* to *cobj*. The :ctype:`PyCObject` must not
   have an associated destructor. Return true on success, false on failure.


.. _cell-objects:

Cell Objects
------------

"Cell" objects are used to implement variables referenced by multiple scopes.
For each such variable, a cell object is created to store the value; the local
variables of each stack frame that references the value contains a reference to
the cells from outer scopes which also use that variable.  When the value is
accessed, the value contained in the cell is used instead of the cell object
itself.  This de-referencing of the cell object requires support from the
generated byte-code; these are not automatically de-referenced when accessed.
Cell objects are not likely to be useful elsewhere.


.. ctype:: PyCellObject

   The C structure used for cell objects.


.. cvar:: PyTypeObject PyCell_Type

   The type object corresponding to cell objects.


.. cfunction:: int PyCell_Check(ob)

   Return true if *ob* is a cell object; *ob* must not be *NULL*.


.. cfunction:: PyObject* PyCell_New(PyObject *ob)

   Create and return a new cell object containing the value *ob*. The parameter may
   be *NULL*.


.. cfunction:: PyObject* PyCell_Get(PyObject *cell)

   Return the contents of the cell *cell*.


.. cfunction:: PyObject* PyCell_GET(PyObject *cell)

   Return the contents of the cell *cell*, but without checking that *cell* is
   non-*NULL* and a cell object.


.. cfunction:: int PyCell_Set(PyObject *cell, PyObject *value)

   Set the contents of the cell object *cell* to *value*.  This releases the
   reference to any current content of the cell. *value* may be *NULL*.  *cell*
   must be non-*NULL*; if it is not a cell object, ``-1`` will be returned.  On
   success, ``0`` will be returned.


.. cfunction:: void PyCell_SET(PyObject *cell, PyObject *value)

   Sets the value of the cell object *cell* to *value*.  No reference counts are
   adjusted, and no checks are made for safety; *cell* must be non-*NULL* and must
   be a cell object.


.. _gen-objects:

Generator Objects
-----------------

Generator objects are what Python uses to implement generator iterators. They
are normally created by iterating over a function that yields values, rather
than explicitly calling :cfunc:`PyGen_New`.


.. ctype:: PyGenObject

   The C structure used for generator objects.


.. cvar:: PyTypeObject PyGen_Type

   The type object corresponding to generator objects


.. cfunction:: int PyGen_Check(ob)

   Return true if *ob* is a generator object; *ob* must not be *NULL*.


.. cfunction:: int PyGen_CheckExact(ob)

   Return true if *ob*'s type is *PyGen_Type* is a generator object; *ob* must not
   be *NULL*.


.. cfunction:: PyObject* PyGen_New(PyFrameObject *frame)

   Create and return a new generator object based on the *frame* object. A
   reference to *frame* is stolen by this function. The parameter must not be
   *NULL*.


.. _datetimeobjects:

DateTime Objects
----------------

Various date and time objects are supplied by the :mod:`datetime` module.
Before using any of these functions, the header file :file:`datetime.h` must be
included in your source (note that this is not included by :file:`Python.h`),
and the macro :cfunc:`PyDateTime_IMPORT` must be invoked.  The macro puts a
pointer to a C structure into a static variable,  ``PyDateTimeAPI``, that is
used by the following macros.

Type-check macros:


.. cfunction:: int PyDate_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DateType` or a subtype of
   :cdata:`PyDateTime_DateType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDate_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DateType`. *ob* must not be
   *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DateTimeType` or a subtype of
   :cdata:`PyDateTime_DateTimeType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DateTimeType`. *ob* must not
   be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyTime_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_TimeType` or a subtype of
   :cdata:`PyDateTime_TimeType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyTime_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_TimeType`. *ob* must not be
   *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDelta_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DeltaType` or a subtype of
   :cdata:`PyDateTime_DeltaType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyDelta_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_DeltaType`. *ob* must not be
   *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyTZInfo_Check(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_TZInfoType` or a subtype of
   :cdata:`PyDateTime_TZInfoType`.  *ob* must not be *NULL*.

   .. versionadded:: 2.4


.. cfunction:: int PyTZInfo_CheckExact(PyObject *ob)

   Return true if *ob* is of type :cdata:`PyDateTime_TZInfoType`. *ob* must not be
   *NULL*.

   .. versionadded:: 2.4

Macros to create objects:


.. cfunction:: PyObject* PyDate_FromDate(int year, int month, int day)

   Return a ``datetime.date`` object with the specified year, month and day.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyDateTime_FromDateAndTime(int year, int month, int day, int hour, int minute, int second, int usecond)

   Return a ``datetime.datetime`` object with the specified year, month, day, hour,
   minute, second and microsecond.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyTime_FromTime(int hour, int minute, int second, int usecond)

   Return a ``datetime.time`` object with the specified hour, minute, second and
   microsecond.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyDelta_FromDSU(int days, int seconds, int useconds)

   Return a ``datetime.timedelta`` object representing the given number of days,
   seconds and microseconds.  Normalization is performed so that the resulting
   number of microseconds and seconds lie in the ranges documented for
   ``datetime.timedelta`` objects.

   .. versionadded:: 2.4

Macros to extract fields from date objects.  The argument must be an instance of
:cdata:`PyDateTime_Date`, including subclasses (such as
:cdata:`PyDateTime_DateTime`).  The argument must not be *NULL*, and the type is
not checked:


.. cfunction:: int PyDateTime_GET_YEAR(PyDateTime_Date *o)

   Return the year, as a positive int.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_GET_MONTH(PyDateTime_Date *o)

   Return the month, as an int from 1 through 12.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_GET_DAY(PyDateTime_Date *o)

   Return the day, as an int from 1 through 31.

   .. versionadded:: 2.4

Macros to extract fields from datetime objects.  The argument must be an
instance of :cdata:`PyDateTime_DateTime`, including subclasses. The argument
must not be *NULL*, and the type is not checked:


.. cfunction:: int PyDateTime_DATE_GET_HOUR(PyDateTime_DateTime *o)

   Return the hour, as an int from 0 through 23.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_DATE_GET_MINUTE(PyDateTime_DateTime *o)

   Return the minute, as an int from 0 through 59.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_DATE_GET_SECOND(PyDateTime_DateTime *o)

   Return the second, as an int from 0 through 59.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_DATE_GET_MICROSECOND(PyDateTime_DateTime *o)

   Return the microsecond, as an int from 0 through 999999.

   .. versionadded:: 2.4

Macros to extract fields from time objects.  The argument must be an instance of
:cdata:`PyDateTime_Time`, including subclasses. The argument must not be *NULL*,
and the type is not checked:


.. cfunction:: int PyDateTime_TIME_GET_HOUR(PyDateTime_Time *o)

   Return the hour, as an int from 0 through 23.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_TIME_GET_MINUTE(PyDateTime_Time *o)

   Return the minute, as an int from 0 through 59.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_TIME_GET_SECOND(PyDateTime_Time *o)

   Return the second, as an int from 0 through 59.

   .. versionadded:: 2.4


.. cfunction:: int PyDateTime_TIME_GET_MICROSECOND(PyDateTime_Time *o)

   Return the microsecond, as an int from 0 through 999999.

   .. versionadded:: 2.4

Macros for the convenience of modules implementing the DB API:


.. cfunction:: PyObject* PyDateTime_FromTimestamp(PyObject *args)

   Create and return a new ``datetime.datetime`` object given an argument tuple
   suitable for passing to ``datetime.datetime.fromtimestamp()``.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyDate_FromTimestamp(PyObject *args)

   Create and return a new ``datetime.date`` object given an argument tuple
   suitable for passing to ``datetime.date.fromtimestamp()``.

   .. versionadded:: 2.4


.. _setobjects:

Set Objects
-----------

.. sectionauthor:: Raymond D. Hettinger <python@rcn.com>


.. index::
   object: set
   object: frozenset

.. versionadded:: 2.5

This section details the public API for :class:`set` and :class:`frozenset`
objects.  Any functionality not listed below is best accessed using the either
the abstract object protocol (including :cfunc:`PyObject_CallMethod`,
:cfunc:`PyObject_RichCompareBool`, :cfunc:`PyObject_Hash`,
:cfunc:`PyObject_Repr`, :cfunc:`PyObject_IsTrue`, :cfunc:`PyObject_Print`, and
:cfunc:`PyObject_GetIter`) or the abstract number protocol (including
:cfunc:`PyNumber_And`, :cfunc:`PyNumber_Subtract`, :cfunc:`PyNumber_Or`,
:cfunc:`PyNumber_Xor`, :cfunc:`PyNumber_InPlaceAnd`,
:cfunc:`PyNumber_InPlaceSubtract`, :cfunc:`PyNumber_InPlaceOr`, and
:cfunc:`PyNumber_InPlaceXor`).


.. ctype:: PySetObject

   This subtype of :ctype:`PyObject` is used to hold the internal data for both
   :class:`set` and :class:`frozenset` objects.  It is like a :ctype:`PyDictObject`
   in that it is a fixed size for small sets (much like tuple storage) and will
   point to a separate, variable sized block of memory for medium and large sized
   sets (much like list storage). None of the fields of this structure should be
   considered public and are subject to change.  All access should be done through
   the documented API rather than by manipulating the values in the structure.


.. cvar:: PyTypeObject PySet_Type

   This is an instance of :ctype:`PyTypeObject` representing the Python
   :class:`set` type.


.. cvar:: PyTypeObject PyFrozenSet_Type

   This is an instance of :ctype:`PyTypeObject` representing the Python
   :class:`frozenset` type.

The following type check macros work on pointers to any Python object. Likewise,
the constructor functions work with any iterable Python object.


.. cfunction:: int PyAnySet_Check(PyObject *p)

   Return true if *p* is a :class:`set` object, a :class:`frozenset` object, or an
   instance of a subtype.


.. cfunction:: int PyAnySet_CheckExact(PyObject *p)

   Return true if *p* is a :class:`set` object or a :class:`frozenset` object but
   not an instance of a subtype.


.. cfunction:: int PyFrozenSet_CheckExact(PyObject *p)

   Return true if *p* is a :class:`frozenset` object but not an instance of a
   subtype.


.. cfunction:: PyObject* PySet_New(PyObject *iterable)

   Return a new :class:`set` containing objects returned by the *iterable*.  The
   *iterable* may be *NULL* to create a new empty set.  Return the new set on
   success or *NULL* on failure.  Raise :exc:`TypeError` if *iterable* is not
   actually iterable.  The constructor is also useful for copying a set
   (``c=set(s)``).


.. cfunction:: PyObject* PyFrozenSet_New(PyObject *iterable)

   Return a new :class:`frozenset` containing objects returned by the *iterable*.
   The *iterable* may be *NULL* to create a new empty frozenset.  Return the new
   set on success or *NULL* on failure.  Raise :exc:`TypeError` if *iterable* is
   not actually iterable.

The following functions and macros are available for instances of :class:`set`
or :class:`frozenset` or instances of their subtypes.


.. cfunction:: int PySet_Size(PyObject *anyset)

   .. index:: builtin: len

   Return the length of a :class:`set` or :class:`frozenset` object. Equivalent to
   ``len(anyset)``.  Raises a :exc:`PyExc_SystemError` if *anyset* is not a
   :class:`set`, :class:`frozenset`, or an instance of a subtype.


.. cfunction:: int PySet_GET_SIZE(PyObject *anyset)

   Macro form of :cfunc:`PySet_Size` without error checking.


.. cfunction:: int PySet_Contains(PyObject *anyset, PyObject *key)

   Return 1 if found, 0 if not found, and -1 if an error is encountered.  Unlike
   the Python :meth:`__contains__` method, this function does not automatically
   convert unhashable sets into temporary frozensets.  Raise a :exc:`TypeError` if
   the *key* is unhashable. Raise :exc:`PyExc_SystemError` if *anyset* is not a
   :class:`set`, :class:`frozenset`, or an instance of a subtype.

The following functions are available for instances of :class:`set` or its
subtypes but not for instances of :class:`frozenset` or its subtypes.


.. cfunction:: int PySet_Add(PyObject *set, PyObject *key)

   Add *key* to a :class:`set` instance.  Does not apply to :class:`frozenset`
   instances.  Return 0 on success or -1 on failure. Raise a :exc:`TypeError` if
   the *key* is unhashable. Raise a :exc:`MemoryError` if there is no room to grow.
   Raise a :exc:`SystemError` if *set* is an not an instance of :class:`set` or its
   subtype.


.. cfunction:: int PySet_Discard(PyObject *set, PyObject *key)

   Return 1 if found and removed, 0 if not found (no action taken), and -1 if an
   error is encountered.  Does not raise :exc:`KeyError` for missing keys.  Raise a
   :exc:`TypeError` if the *key* is unhashable.  Unlike the Python :meth:`discard`
   method, this function does not automatically convert unhashable sets into
   temporary frozensets. Raise :exc:`PyExc_SystemError` if *set* is an not an
   instance of :class:`set` or its subtype.


.. cfunction:: PyObject* PySet_Pop(PyObject *set)

   Return a new reference to an arbitrary object in the *set*, and removes the
   object from the *set*.  Return *NULL* on failure.  Raise :exc:`KeyError` if the
   set is empty. Raise a :exc:`SystemError` if *set* is an not an instance of
   :class:`set` or its subtype.


.. cfunction:: int PySet_Clear(PyObject *set)

   Empty an existing set of all elements.

