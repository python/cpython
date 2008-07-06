.. highlightlang:: c

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


.. cfunction:: PyObject* PyFloat_FromString(PyObject *str, char **pend)

   Create a :ctype:`PyFloatObject` object based on the string value in *str*, or
   *NULL* on failure.  The *pend* argument is ignored.  It remains only for
   backward compatibility.


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

   .. versionadded:: 2.6


.. cfunction:: double PyFloat_GetMax(void)

   Return the maximum representable finite float *DBL_MAX* as C :ctype:`double`.

   .. versionadded:: 2.6


.. cfunction:: double PyFloat_GetMin(void)

   Return the minimum normalized positive float *DBL_MIN* as C :ctype:`double`.

   .. versionadded:: 2.6


.. cfunction:: int PyFloat_ClearFreeList(void)

   Clear the float free list. Return the number of items that could not
   be freed.

   .. versionadded:: 2.6
