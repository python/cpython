.. highlight:: c


.. _countingrefs:

******************
Reference Counting
******************

The macros in this section are used for managing reference counts of Python
objects.


.. c:function:: void Py_INCREF(PyObject *o)

   Increment the reference count for object *o*.

   This function is usually used to convert a :term:`borrowed reference` to a
   :term:`strong reference` in-place. The :c:func:`Py_NewRef` function can be
   used to create a new :term:`strong reference`.

   The object must not be ``NULL``; if you aren't sure that it isn't
   ``NULL``, use :c:func:`Py_XINCREF`.


.. c:function:: void Py_XINCREF(PyObject *o)

   Increment the reference count for object *o*.  The object can be ``NULL``, in
   which case the macro has no effect.

   See also :c:func:`Py_XNewRef`.


.. c:function:: PyObject* Py_NewRef(PyObject *o)

   Create a new :term:`strong reference` to an object: increment the reference
   count of the object *o* and return the object *o*.

   When the :term:`strong reference` is no longer needed, :c:func:`Py_DECREF`
   should be called on it to decrement the object reference count.

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

   Decrement the reference count for object *o*.

   If the reference count reaches zero, the object's type's deallocation
   function (which must not be ``NULL``) is invoked.

   This function is usually used to delete a :term:`strong reference` before
   exiting its scope.

   The object must not be ``NULL``; if you aren't sure that it isn't ``NULL``,
   use :c:func:`Py_XDECREF`.

   Use :c:func:`Py_CLEAR` to set a variable to ``NULL``. See also
   :c:func:`Py_SetRef`.

   .. warning::

      The deallocation function can cause arbitrary Python code to be invoked (e.g.
      when a class instance with a :meth:`__del__` method is deallocated).  While
      exceptions in such code are not propagated, the executed code has free access to
      all Python global variables.  This means that any object that is reachable from
      a global variable should be in a consistent state before :c:func:`Py_DECREF` is
      invoked.  For example, code to delete an object from a list should copy a
      reference to the deleted object in a temporary variable, update the list data
      structure, and then call :c:func:`Py_DECREF` for the temporary variable.


.. c:function:: void Py_XDECREF(PyObject *o)

   Decrement the reference count for object *o*.  The object can be ``NULL``, in
   which case the macro has no effect; otherwise the effect is the same as for
   :c:func:`Py_DECREF`, and the same warning applies.

   Use :c:func:`Py_CLEAR` to set a variable to ``NULL``. See also
   :c:func:`Py_XSetRef`.


.. c:function:: void Py_SetRef(PyObject **ref, PyObject *new_obj)

   Set the :term:`strong reference` ``*ref`` to *new_obj* object (without
   increasing its reference count), and then decrement the reference count of
   the object previously pointed by ``*ref``.

   *ref* and ``*ref`` must not be ``NULL``.

   This function avoids creating a temporary dangling pointer. Example::

       Py_DECREF(global_var);   // unsafe
       global_var = new_obj;

   If the :c:func:`Py_DECREF` call destroys the object previously pointed by
   *global_var*, the object finalizer can run arbitrary code which can use the
   *global_var* variable. Since the *global_var* became a dangling pointer,
   using it crashs Python. The example can be fixed by using
   :c:func:`Py_SetRef`:

       Py_SetRef(&global_var, new_obj);   // safe

   See also the :c:func:`Py_CLEAR` macro.

   .. versionadded:: 3.10


.. c:function:: void Py_XSetRef(PyObject **ref, PyObject *new_obj)

   Similar to :c:func:`Py_SetRef`, but ``*ref`` can be ``NULL``.

   If ``*ref`` is ``NULL``, do nothing.

   See also :c:func:`Py_CLEAR` macro.

   .. versionadded:: 3.10


.. c:function:: void Py_CLEAR(PyObject *op)

   Clear a :term:`strong reference`: decrement the reference count for object
   *op* and set *op* to ``NULL`.

   If *op* is ``NULL``, do nothing.

   The ``Py_CLEAR(var)`` macro is implemented with the :c:func:`Py_XSetRef`
   function as ``Py_XSetRef(&var, NULL)`` to avoid creating a temporary
   dangling pointer.

   It is a good idea to use this macro whenever decrementing the reference
   count of an object that might be traversed during garbage collection.


The following functions are for runtime dynamic embedding of Python:
``Py_IncRef(PyObject *o)``, ``Py_DecRef(PyObject *o)``. They are
simply exported function versions of :c:func:`Py_XINCREF` and
:c:func:`Py_XDECREF`, respectively.

The following functions or macros are only for use within the interpreter core:
:c:func:`_Py_Dealloc`, :c:func:`_Py_ForgetReference`, :c:func:`_Py_NewReference`,
as well as the global variable :c:data:`_Py_RefTotal`.

