.. highlightlang:: c

.. _intobjects:

Plain Integer Objects
---------------------

.. index:: object: integer


.. c:type:: PyIntObject

   This subtype of :c:type:`PyObject` represents a Python integer object.


.. c:var:: PyTypeObject PyInt_Type

   .. index:: single: IntType (in modules types)

   This instance of :c:type:`PyTypeObject` represents the Python plain integer type.
   This is the same object as ``int`` and ``types.IntType``.


.. c:function:: int PyInt_Check(PyObject *o)

   Return true if *o* is of type :c:data:`PyInt_Type` or a subtype of
   :c:data:`PyInt_Type`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. c:function:: int PyInt_CheckExact(PyObject *o)

   Return true if *o* is of type :c:data:`PyInt_Type`, but not a subtype of
   :c:data:`PyInt_Type`.

   .. versionadded:: 2.2


.. c:function:: PyObject* PyInt_FromString(char *str, char **pend, int base)

   Return a new :c:type:`PyIntObject` or :c:type:`PyLongObject` based on the string
   value in *str*, which is interpreted according to the radix in *base*.  If
   *pend* is non-*NULL*, ``*pend`` will point to the first character in *str* which
   follows the representation of the number.  If *base* is ``0``, the radix will be
   determined based on the leading characters of *str*: if *str* starts with
   ``'0x'`` or ``'0X'``, radix 16 will be used; if *str* starts with ``'0'``, radix
   8 will be used; otherwise radix 10 will be used.  If *base* is not ``0``, it
   must be between ``2`` and ``36``, inclusive.  Leading spaces are ignored.  If
   there are no digits, :exc:`ValueError` will be raised.  If the string represents
   a number too large to be contained within the machine's :c:type:`long int` type
   and overflow warnings are being suppressed, a :c:type:`PyLongObject` will be
   returned.  If overflow warnings are not being suppressed, *NULL* will be
   returned in this case.


.. c:function:: PyObject* PyInt_FromLong(long ival)

   Create a new integer object with a value of *ival*.

   The current implementation keeps an array of integer objects for all integers
   between ``-5`` and ``256``, when you create an int in that range you actually
   just get back a reference to the existing object. So it should be possible to
   change the value of ``1``.  I suspect the behaviour of Python in this case is
   undefined. :-)


.. c:function:: PyObject* PyInt_FromSsize_t(Py_ssize_t ival)

   Create a new integer object with a value of *ival*. If the value is larger
   than ``LONG_MAX`` or smaller than ``LONG_MIN``, a long integer object is
   returned.

   .. versionadded:: 2.5


.. c:function:: PyObject* PyInt_FromSize_t(size_t ival)

   Create a new integer object with a value of *ival*. If the value exceeds
   ``LONG_MAX``, a long integer object is returned.

   .. versionadded:: 2.5


.. c:function:: long PyInt_AsLong(PyObject *io)

   Will first attempt to cast the object to a :c:type:`PyIntObject`, if it is not
   already one, and then return its value. If there is an error, ``-1`` is
   returned, and the caller should check ``PyErr_Occurred()`` to find out whether
   there was an error, or whether the value just happened to be ``-1``.


.. c:function:: long PyInt_AS_LONG(PyObject *io)

   Return the value of the object *io*.  No error checking is performed.


.. c:function:: unsigned long PyInt_AsUnsignedLongMask(PyObject *io)

   Will first attempt to cast the object to a :c:type:`PyIntObject` or
   :c:type:`PyLongObject`, if it is not already one, and then return its value as
   unsigned long.  This function does not check for overflow.

   .. versionadded:: 2.3


.. c:function:: unsigned PY_LONG_LONG PyInt_AsUnsignedLongLongMask(PyObject *io)

   Will first attempt to cast the object to a :c:type:`PyIntObject` or
   :c:type:`PyLongObject`, if it is not already one, and then return its value as
   unsigned long long, without checking for overflow.

   .. versionadded:: 2.3


.. c:function:: Py_ssize_t PyInt_AsSsize_t(PyObject *io)

   Will first attempt to cast the object to a :c:type:`PyIntObject` or
   :c:type:`PyLongObject`, if it is not already one, and then return its value as
   :c:type:`Py_ssize_t`.

   .. versionadded:: 2.5


.. c:function:: long PyInt_GetMax()

   .. index:: single: LONG_MAX

   Return the system's idea of the largest integer it can handle
   (:const:`LONG_MAX`, as defined in the system header files).


.. c:function:: int PyInt_ClearFreeList()

   Clear the integer free list. Return the number of items that could not
   be freed.

   .. versionadded:: 2.6
