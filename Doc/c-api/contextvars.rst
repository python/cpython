.. highlight:: c

.. _contextvarsobjects:

Context Variables Objects
-------------------------

.. _contextvarsobjects_pointertype_change:
.. versionadded:: 3.7

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

.. c:function:: int PyContext_AddWatcher(PyObject *callback)

   Registers the callable object *callback* as a context object watcher for the
   current interpreter.  When a context event occurs, *callback* is called with
   two arguments:

   #. An event type ID from :c:type:`PyContextEvent`.
   #. An object containing event-specific supplemental data; see
      :c:type:`PyContextEvent` for details.

   Any exception raised by *callback* will be printed as an unraisable
   exception as if by a call to :c:func:`PyErr_FormatUnraisable`, then
   discarded.

   On success, this function returns a non-negative ID which may be passed to
   :c:func:`PyContext_ClearWatcher` to unregister the callback and remove the
   reference this function adds to *callback*.  Sets an exception and returns
   ``-1`` on error (e.g., no more watcher IDs available).

   Example using a C function as the callback::

      static PyObject *
      my_callback(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
      {
          if (PyVectorcall_NARGS(nargs) != 2) {
              PyErr_Format(PyExc_TypeError, "want 2 args, got %zd", nargs);
              return NULL;
          }
          int event = PyLong_AsInt(args[0]);
          if (event == -1 && PyErr_Occurred()) {
              return NULL;
          }
          if (event != Py_CONTEXT_SWITCHED) {
              Py_RETURN_NONE;
          }
          PyObject *ctx = args[1];

          // Do something interesting with self and ctx here.

          Py_RETURN_NONE;
      }

      PyMethodDef my_callback_md = {
          .ml_name = "my_callback",
          .ml_meth = (PyCFunction)(void *)&my_callback,
          .ml_flags = METH_FASTCALL,
          .ml_doc = NULL,
      };

      int
      register_my_callback(PyObject *callback_state)
      {
          PyObject *cb = PyCFunction_New(&my_callback_md, callback_state);
          if (cb == NULL) {
              return -1;
          }
          int id = PyContext_AddWatcher(cb);
          Py_CLEAR(cb);
          return id;
      }

   .. versionadded:: 3.14

.. c:function:: int PyContext_ClearWatcher(int watcher_id)

   Clears the watcher identified by *watcher_id* previously returned from
   :c:func:`PyContext_AddWatcher` for the current interpreter, and removes the
   reference created for the registered callback object.  Returns ``0`` on
   success, or sets an exception and returns ``-1`` on error (e.g., if the
   given *watcher_id* was never registered).

   .. versionadded:: 3.14

.. c:type:: PyContextEvent

   Enumeration of possible context object watcher events:

   - ``Py_CONTEXT_SWITCHED``: The :term:`current context` has switched to a
     different context.  The object passed to the watch callback is the
     now-current :class:`contextvars.Context` object, or None if no context is
     current.

   .. versionadded:: 3.14


Context variable functions:

.. c:function:: PyObject *PyContextVar_New(const char *name, PyObject *def)

   Create a new ``ContextVar`` object.  The *name* parameter is used
   for introspection and debug purposes.  The *def* parameter specifies
   a default value for the context variable, or ``NULL`` for no default.
   If an error has occurred, this function returns ``NULL``.

.. c:function:: int PyContextVar_Get(PyObject *var, PyObject *default_value, PyObject **value)

   Get the value of a context variable.  Returns ``-1`` if an error has
   occurred during lookup, and ``0`` if no error occurred, whether or not
   a value was found.

   If the context variable was found, *value* will be a pointer to it.
   If the context variable was *not* found, *value* will point to:

   - *default_value*, if not ``NULL``;
   - the default value of *var*, if not ``NULL``;
   - ``NULL``

   Except for ``NULL``, the function returns a new reference.

.. c:function:: PyObject *PyContextVar_Set(PyObject *var, PyObject *value)

   Set the value of *var* to *value* in the current context.  Returns
   a new token object for this change, or ``NULL`` if an error has occurred.

.. c:function:: int PyContextVar_Reset(PyObject *var, PyObject *token)

   Reset the state of the *var* context variable to that it was in before
   :c:func:`PyContextVar_Set` that returned the *token* was called.
   This function returns ``0`` on success and ``-1`` on error.
