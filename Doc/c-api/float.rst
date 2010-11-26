.. highlightlang:: c

.. _floatobjects:

Floating Point Objects
----------------------

.. index:: object: floating point


.. ctype:: PyFloatObject

   This subtype of :ctype:`PyObject` represents a Python floating point object.


.. cvar:: PyTypeObject PyFloat_Type

   This instance of :ctype:`PyTypeObject` represents the Python floating point
   type.  This is the same object as :class:`float` in the Python layer.


.. cfunction:: int PyFloat_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyFloatObject` or a subtype of
   :ctype:`PyFloatObject`.


.. cfunction:: int PyFloat_CheckExact(PyObject *p)

   Return true if its argument is a :ctype:`PyFloatObject`, but not a subtype of
   :ctype:`PyFloatObject`.


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


.. cfunction:: PyObject* PyFloat_GetInfo(void)

   Return a structseq instance which contains information about the
   precision, minimum and maximum values of a float. It's a thin wrapper
   around the header file :file:`float.h`.


.. cfunction:: double PyFloat_GetMax()

   Return the maximum representable finite float *DBL_MAX* as C :ctype:`double`.


.. cfunction:: double PyFloat_GetMin()

   Return the minimum normalized positive float *DBL_MIN* as C :ctype:`double`.

.. cfunction:: int PyFloat_ClearFreeList()

   Clear the float free list. Return the number of items that could not
   be freed.
