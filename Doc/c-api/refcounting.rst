.. highlight:: c


.. _countingrefs:

******************
Reference Counting
******************

The macros in this section are used for managing reference counts of Python
objects.


.. c:function:: void Py_INCREF(PyObject *o)

   Indicate taking a new :term:`strong reference` to object *o*,
   indicating it is in use and should not be destroyed.

   This function is usually used to convert a :term:`borrowed reference` to a
   :term:`strong reference` in-place. The :c:func:`Py_NewRef` function can be
   used to create a new :term:`strong reference`.

   When done using the object, release it by calling :c:func:`Py_DECREF`.

   The object must not be ``NULL``; if you aren't sure that it isn't
   ``NULL``, use :c:func:`Py_XINCREF`.

   Do not expect this function to actually modify *o* in any way.


.. c:function:: void Py_XINCREF(PyObject *o)

   Similar to :c:func:`Py_INCREF`, but the object *o* can be ``NULL``,
   in which case this has no effect.

   See also :c:func:`Py_XNewRef`.


.. c:function:: PyObject* Py_NewRef(PyObject *o)

   Create a new :term:`strong reference` to an object:
   call :c:func:`Py_INCREF` on *o* and return the object *o*.

   When the :term:`strong reference` is no longer needed, :c:func:`Py_DECREF`
   should be called on it to release the reference.

   The object *o* must not be ``NULL``; use :c:func:`Py_XNewRef` if *o* can be
   ``NULL``.

   For example::

       Py_INCREF(obj);
       self->attr = obj;

   can be written as::

       self->attr = Py_NewRef(obj);

   See also :c:func:`Py_INCREF`.

   .. versionadded:: 3.10


.. c:function:: PyObject* Py_XNewRef(PyObject *o)

   Similar to :c:func:`Py_NewRef`, but the object *o* can be NULL.

   If the object *o* is ``NULL``, the function just returns ``NULL``.

   .. versionadded:: 3.10


.. c:function:: void Py_DECREF(PyObject *o)

   Release a :term:`strong reference` to object *o*, indicating the
   reference is no longer used.

   Once the last :term:`strong reference` is released
   (i.e. the object's reference count reaches 0),
   the object's type's deallocation
   function (which must not be ``NULL``) is invoked.

   This function is usually used to delete a :term:`strong reference` before
   exiting its scope.

   The object must not be ``NULL``; if you aren't sure that it isn't ``NULL``,
   use :c:func:`Py_XDECREF`.

   Do not expect this function to actually modify *o* in any way.

   .. warning::

      The deallocation function can cause arbitrary Python code to be invoked (e.g.
      when a class instance with a :meth:`~object.__del__` method is deallocated).  While
      exceptions in such code are not propagated, the executed code has free access to
      all Python global variables.  This means that any object that is reachable from
      a global variable should be in a consistent state before :c:func:`Py_DECREF` is
      invoked.  For example, code to delete an object from a list should copy a
      reference to the deleted object in a temporary variable, update the list data
      structure, and then call :c:func:`Py_DECREF` for the temporary variable.


.. c:function:: void Py_XDECREF(PyObject *o)

   Similar to :c:func:`Py_DECREF`, but the object *o* can be ``NULL``,
   in which case this has no effect.
   The same warning from :c:func:`Py_DECREF` applies here as well.


.. c:function:: void Py_CLEAR(PyObject *o)

   Release a :term:`strong reference` for object *o*.
   The object may be ``NULL``, in
   which case the macro has no effect; otherwise the effect is the same as for
   :c:func:`Py_DECREF`, except that the argument is also set to ``NULL``.  The warning
   for :c:func:`Py_DECREF` does not apply with respect to the object passed because
   the macro carefully uses a temporary variable and sets the argument to ``NULL``
   before releasing the reference.

   It is a good idea to use this macro whenever releasing a reference
   to an object that might be traversed during garbage collection.

.. c:function:: void Py_IncRef(PyObject *o)

   Indicate taking a new :term:`strong reference` to object *o*.
   A function version of :c:func:`Py_XINCREF`.
   It can be used for runtime dynamic embedding of Python.


.. c:function:: void Py_DecRef(PyObject *o)

   Release a :term:`strong reference` to object *o*.
   A function version of :c:func:`Py_XDECREF`.
   It can be used for runtime dynamic embedding of Python.


The following functions or macros are only for use within the interpreter core:
:c:func:`_Py_Dealloc`, :c:func:`_Py_ForgetReference`, :c:func:`_Py_NewReference`,
as well as the global variable :c:data:`_Py_RefTotal`.

