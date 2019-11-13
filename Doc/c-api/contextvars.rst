.. highlight:: c

.. _contextvarsobjects:

Context Variables Objects
-------------------------

.. _contextvarsobjects_pointertype_change:
.. versionchanged:: 3.7.1

   .. note::

      In Python 3.7.1 the signatures of all context variables
      C APIs were **changed** to use :c:type:`PyObject` pointers instead
      of :c:type:`PyContext`, :c:type:`PyContextVar`, and
      :c:type:`PyContextToken`, e.g.::

         // in 3.7.0:
         PyContext *PyContext_New(void);

         // in 3.7.1+:
         PyObject *PyContext_New(void);

      See :issue:`34762` for more details.


.. versionadded:: 3.7

This section details the public C API for the :mod:`contextvars` module.

.. c:type:: PyContext

   The C structure used to represent a :class:`contextvars.Context`
   object.

.. c:type:: PyContextVar

   The C structure used to represent a :class:`contextvars.ContextVar`
   object.

.. c:type:: PyContextToken

   The C structure used to represent a :class:`contextvars.Token` object.

.. c:var:: PyTypeObject PyContext_Type

   The type object representing the *context* type.

.. c:var:: PyTypeObject PyContextVar_Type

   The type object representing the *context variable* type.

.. c:var:: PyTypeObject PyContextToken_Type

   The type object representing the *context variable token* type.


Type-check macros:

.. c:function:: int PyContext_CheckExact(PyObject *o)

   Return true if *o* is of type :c:data:`PyContext_Type`. *o* must not be
   ``NULL``.  This function always succeeds.

.. c:function:: int PyContextVar_CheckExact(PyObject *o)

   Return true if *o* is of type :c:data:`PyContextVar_Type`. *o* must not be
   ``NULL``.  This function always succeeds.

.. c:function:: int PyContextToken_CheckExact(PyObject *o)

   Return true if *o* is of type :c:data:`PyContextToken_Type`.
   *o* must not be ``NULL``.  This function always succeeds.


Context object management functions:

.. c:function:: PyObject *PyContext_New(void)

   Create a new empty context object.  Returns ``NULL`` if an error
   has occurred.

.. c:function:: PyObject *PyContext_Copy(PyObject *ctx)

   Create a shallow copy of the passed *ctx* context object.
   Returns ``NULL`` if an error has occurred.

.. c:function:: PyObject *PyContext_CopyCurrent(void)

   Create a shallow copy of the current thread context.
   Returns ``NULL`` if an error has occurred.

.. c:function:: int PyContext_Enter(PyObject *ctx)

   Set *ctx* as the current context for the current thread.
   Returns ``0`` on success, and ``-1`` on error.

.. c:function:: int PyContext_Exit(PyObject *ctx)

   Deactivate the *ctx* context and restore the previous context as the
   current context for the current thread.  Returns ``0`` on success,
   and ``-1`` on error.

.. c:function:: int PyContext_ClearFreeList()

   Clear the context variable free list. Return the total number of
   freed items.  This function always succeeds.


Context variable functions:

.. c:function:: PyObject *PyContextVar_New(const char *name, PyObject *def)

   Create a new ``ContextVar`` object.  The *name* parameter is used
   for introspection and debug purposes.  The *def* parameter may optionally
   specify the default value for the context variable.  If an error has
   occurred, this function returns ``NULL``.

.. c:function:: int PyContextVar_Get(PyObject *var, PyObject *default_value, PyObject **value)

   Get the value of a context variable.  Returns ``-1`` if an error has
   occurred during lookup, and ``0`` if no error occurred, whether or not
   a value was found.

   If the context variable was found, *value* will be a pointer to it.
   If the context variable was *not* found, *value* will point to:

   - *default_value*, if not ``NULL``;
   - the default value of *var*, if not ``NULL``;
   - ``NULL``

   If the value was found, the function will create a new reference to it.

.. c:function:: PyObject *PyContextVar_Set(PyObject *var, PyObject *value)

   Set the value of *var* to *value* in the current context.  Returns a
   pointer to a :c:type:`PyObject` object, or ``NULL`` if an error
   has occurred.

.. c:function:: int PyContextVar_Reset(PyObject *var, PyObject *token)

   Reset the state of the *var* context variable to that it was in before
   :c:func:`PyContextVar_Set` that returned the *token* was called.
   This function returns ``0`` on success and ``-1`` on error.
