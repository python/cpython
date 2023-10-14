.. highlight:: c

.. _boolobjects:

Boolean Objects
---------------

Booleans in Python are implemented as a subclass of integers.  There are only
two booleans, :c:data:`Py_False` and :c:data:`Py_True`.  As such, the normal
creation and deletion functions don't apply to booleans.  The following macros
are available, however.


.. c:var:: PyTypeObject PyBool_Type

   This instance of :c:type:`PyTypeObject` represents the Python boolean type; it
   is the same object as :class:`bool` in the Python layer.


.. c:function:: int PyBool_Check(PyObject *o)

   Return true if *o* is of type :c:data:`PyBool_Type`.  This function always
   succeeds.


.. c:var:: PyObject* Py_False

   The Python ``False`` object.  This object has no methods and is
   `immortal <https://peps.python.org/pep-0683/>`_.

.. versionchanged:: 3.12
   :c:data:`Py_False` is immortal.


.. c:var:: PyObject* Py_True

   The Python ``True`` object.  This object has no methods and is
   `immortal <https://peps.python.org/pep-0683/>`_.

.. versionchanged:: 3.12
   :c:data:`Py_True` is immortal.


.. c:macro:: Py_RETURN_FALSE

   Return :c:data:`Py_False` from a function.


.. c:macro:: Py_RETURN_TRUE

   Return :c:data:`Py_True` from a function.


.. c:function:: PyObject* PyBool_FromLong(long v)

   Return :c:data:`Py_True` or :c:data:`Py_False`, depending on the truth value of *v*.
