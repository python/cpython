.. highlightlang:: c

.. _floatobjects:

Floating Point Objects
----------------------

.. index:: object: floating point


.. c:type:: PyFloatObject

   This subtype of :c:type:`PyObject` represents a Python floating point object.


.. c:var:: PyTypeObject PyFloat_Type

   .. index:: single: FloatType (in modules types)

   This instance of :c:type:`PyTypeObject` represents the Python floating point
   type.  This is the same object as ``float`` and ``types.FloatType``.


.. c:function:: int PyFloat_Check(PyObject *p)

   Return true if its argument is a :c:type:`PyFloatObject` or a subtype of
   :c:type:`PyFloatObject`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. c:function:: int PyFloat_CheckExact(PyObject *p)

   Return true if its argument is a :c:type:`PyFloatObject`, but not a subtype of
   :c:type:`PyFloatObject`.

   .. versionadded:: 2.2


.. c:function:: PyObject* PyFloat_FromString(PyObject *str, char **pend)

   Create a :c:type:`PyFloatObject` object based on the string value in *str*, or
   *NULL* on failure.  The *pend* argument is ignored.  It remains only for
   backward compatibility.


.. c:function:: PyObject* PyFloat_FromDouble(double v)

   Create a :c:type:`PyFloatObject` object from *v*, or *NULL* on failure.


.. c:function:: double PyFloat_AsDouble(PyObject *pyfloat)

   Return a C :c:type:`double` representation of the contents of *pyfloat*.  If
   *pyfloat* is not a Python floating point object but has a :meth:`__float__`
   method, this method will first be called to convert *pyfloat* into a float.
   This method returns ``-1.0`` upon failure, so one should call
   :c:func:`PyErr_Occurred` to check for errors.


.. c:function:: double PyFloat_AS_DOUBLE(PyObject *pyfloat)

   Return a C :c:type:`double` representation of the contents of *pyfloat*, but
   without error checking.


.. c:function:: PyObject* PyFloat_GetInfo(void)

   Return a structseq instance which contains information about the
   precision, minimum and maximum values of a float. It's a thin wrapper
   around the header file :file:`float.h`.

   .. versionadded:: 2.6


.. c:function:: double PyFloat_GetMax()

   Return the maximum representable finite float *DBL_MAX* as C :c:type:`double`.

   .. versionadded:: 2.6


.. c:function:: double PyFloat_GetMin()

   Return the minimum normalized positive float *DBL_MIN* as C :c:type:`double`.

   .. versionadded:: 2.6


.. c:function:: int PyFloat_ClearFreeList()

   Clear the float free list. Return the number of items that could not
   be freed.

   .. versionadded:: 2.6


.. c:function:: void PyFloat_AsString(char *buf, PyFloatObject *v)

   Convert the argument *v* to a string, using the same rules as
   :func:`str`. The length of *buf* should be at least 100.

   This function is unsafe to call because it writes to a buffer whose
   length it does not know.

   .. deprecated:: 2.7
      Use :func:`PyObject_Str` or :func:`PyOS_double_to_string` instead.


.. c:function:: void PyFloat_AsReprString(char *buf, PyFloatObject *v)

   Same as PyFloat_AsString, except uses the same rules as
   :func:`repr`.  The length of *buf* should be at least 100.

   This function is unsafe to call because it writes to a buffer whose
   length it does not know.

   .. deprecated:: 2.7
      Use :func:`PyObject_Repr` or :func:`PyOS_double_to_string` instead.
