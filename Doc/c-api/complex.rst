.. highlight:: c

.. _complexobjects:

Complex Number Objects
----------------------

.. index:: pair: object; complex number

Python's complex number objects are implemented as two distinct types when
viewed from the C API:  one is the Python object exposed to Python programs, and
the other is a C structure which represents the actual complex number value.
The API provides functions for working with both.


Complex Numbers as C Structures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note that the functions which accept these structures as parameters and return
them as results do so *by value* rather than dereferencing them through
pointers.  This is consistent throughout the API.


.. c:type:: Py_complex

   The C structure which corresponds to the value portion of a Python complex
   number object.  Most of the functions for dealing with complex number objects
   use structures of this type as input or output values, as appropriate.

   .. c:member:: double real
                 double imag

   The structure is defined as::

      typedef struct {
          double real;
          double imag;
      } Py_complex;


.. c:function:: Py_complex _Py_c_sum(Py_complex left, Py_complex right)

   Return the sum of two complex numbers, using the C :c:type:`Py_complex`
   representation.


.. c:function:: Py_complex _Py_c_diff(Py_complex left, Py_complex right)

   Return the difference between two complex numbers, using the C
   :c:type:`Py_complex` representation.


.. c:function:: Py_complex _Py_c_neg(Py_complex num)

   Return the negation of the complex number *num*, using the C
   :c:type:`Py_complex` representation.


.. c:function:: Py_complex _Py_c_prod(Py_complex left, Py_complex right)

   Return the product of two complex numbers, using the C :c:type:`Py_complex`
   representation.


.. c:function:: Py_complex _Py_c_quot(Py_complex dividend, Py_complex divisor)

   Return the quotient of two complex numbers, using the C :c:type:`Py_complex`
   representation.

   If *divisor* is null, this method returns zero and sets
   :c:data:`errno` to :c:macro:`!EDOM`.


.. c:function:: Py_complex _Py_c_pow(Py_complex num, Py_complex exp)

   Return the exponentiation of *num* by *exp*, using the C :c:type:`Py_complex`
   representation.

   If *num* is null and *exp* is not a positive real number,
   this method returns zero and sets :c:data:`errno` to :c:macro:`!EDOM`.


Complex Numbers as Python Objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. c:type:: PyComplexObject

   This subtype of :c:type:`PyObject` represents a Python complex number object.


.. c:var:: PyTypeObject PyComplex_Type

   This instance of :c:type:`PyTypeObject` represents the Python complex number
   type. It is the same object as :class:`complex` in the Python layer.


.. c:function:: int PyComplex_Check(PyObject *p)

   Return true if its argument is a :c:type:`PyComplexObject` or a subtype of
   :c:type:`PyComplexObject`.  This function always succeeds.


.. c:function:: int PyComplex_CheckExact(PyObject *p)

   Return true if its argument is a :c:type:`PyComplexObject`, but not a subtype of
   :c:type:`PyComplexObject`.  This function always succeeds.


.. c:function:: PyObject* PyComplex_FromCComplex(Py_complex v)

   Create a new Python complex number object from a C :c:type:`Py_complex` value.
   Return ``NULL`` with an exception set on error.


.. c:function:: PyObject* PyComplex_FromDoubles(double real, double imag)

   Return a new :c:type:`PyComplexObject` object from *real* and *imag*.
   Return ``NULL`` with an exception set on error.


.. c:function:: double PyComplex_RealAsDouble(PyObject *op)

   Return the real part of *op* as a C :c:expr:`double`.

   If *op* is not a Python complex number object but has a
   :meth:`~object.__complex__` method, this method will first be called to
   convert *op* to a Python complex number object.  If :meth:`!__complex__` is
   not defined then it falls back to call :c:func:`PyFloat_AsDouble` and
   returns its result.

   Upon failure, this method returns ``-1.0`` with an exception set, so one
   should call :c:func:`PyErr_Occurred` to check for errors.

   .. versionchanged:: 3.13
      Use :meth:`~object.__complex__` if available.

.. c:function:: double PyComplex_ImagAsDouble(PyObject *op)

   Return the imaginary part of *op* as a C :c:expr:`double`.

   If *op* is not a Python complex number object but has a
   :meth:`~object.__complex__` method, this method will first be called to
   convert *op* to a Python complex number object.  If :meth:`!__complex__` is
   not defined then it falls back to call :c:func:`PyFloat_AsDouble` and
   returns ``0.0`` on success.

   Upon failure, this method returns ``-1.0`` with an exception set, so one
   should call :c:func:`PyErr_Occurred` to check for errors.

   .. versionchanged:: 3.13
      Use :meth:`~object.__complex__` if available.

.. c:function:: Py_complex PyComplex_AsCComplex(PyObject *op)

   Return the :c:type:`Py_complex` value of the complex number *op*.

   If *op* is not a Python complex number object but has a :meth:`~object.__complex__`
   method, this method will first be called to convert *op* to a Python complex
   number object.  If :meth:`!__complex__` is not defined then it falls back to
   :meth:`~object.__float__`.  If :meth:`!__float__` is not defined then it falls back
   to :meth:`~object.__index__`.

   Upon failure, this method returns :c:type:`Py_complex`
   with :c:member:`~Py_complex.real` set to ``-1.0`` and with an exception set, so one
   should call :c:func:`PyErr_Occurred` to check for errors.

   .. versionchanged:: 3.8
      Use :meth:`~object.__index__` if available.
