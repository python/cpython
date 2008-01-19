.. highlightlang:: c

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
