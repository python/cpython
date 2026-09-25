.. highlight:: c


.. _error-handling:


***************************
Error handling in the C API
***************************

This chapter covers the details about how Python's C API expresses errors
and how to interact with Python exceptions.

The exception indicator
=======================

Python has a thread-local indicator for the state of the current exception.
This indicator is a ``PyObject *`` referencing an instance of
:class:`BaseException`. You can think of this like the ``errno`` variable in C.

If a C API function fails, it may set the exception indicator to a Python
exception object. For example, creating a new object may fail and set the
exception indicator to a :class:`MemoryError` object to denote that an
allocation failed.

Generally speaking, you must not call functions with the exception indicator
set. This is explained in more detail later on.


The failure protocol
====================

In the C API, ``NULL`` is never a valid ``PyObject *``, so it is used as a
sentinel to indicate failure for functions that return a ``PyObject *``.
In fact, we've already used this! Going back to our ``system`` function,
we can see this in action:

.. code-block:: c

   :emphasize-lines: 6

   static PyObject *
   spam_system(PyObject *self, PyObject *arg)
   {
      const char *command = PyUnicode_AsUTF8(arg);
      if (command == NULL) {
         return NULL;
      }
      int status = system(command);
      PyObject *result = PyLong_FromLong(status);
      return result;
   }


``spam_system`` returns a ``PyObject *``, so we indicate failure by returning
``NULL``.

.. note::

   Some functions in the C API return an ``int`` instead of a reference, so they
   cannot use ``NULL`` for failure. These functions will usually return ``-1``
   for failure, and ``0`` otherwise.

To expand on this, let's try to modify ``spam_system`` to raise an
exception if the result is non-zero:

.. code-block:: c
   :emphasize-lines: 6

   static PyObject *
   spam_system(PyObject *self, PyObject *arg)
   {
      const char *command = PyUnicode_AsUTF8(arg);
      if (command == NULL) {
         return NULL;
      }
      int status = system(command);
      if (status != 0) {
         return NULL;
      }

      // We don't know how to return None yet, so let's do this for now.
      return PyLong_FromLong(status);
   }

Because ``system`` is not from Python's C API, it has no knowledge of Python's
exception indicator, and thus does not set any exceptions. So, if we were to
run this code with an invalid command, the interpreter would raise a
:class:`SystemError`:

.. code-block:: pycon

   >>> import spam
   >>> result = spam.system('noexist')
   SystemError: <built-in function system> returned NULL without setting an exception

To manually raise an exception, we can use :c:func:`PyErr_SetString`, which
will take a reference to an exception class and a C string to use as the
message. All of Python's built-in exceptions are available as global C
variables prefixed with ``PyExc_`` followed by their name in Python.
For example, :class:`RuntimeError` is available as :c:var:`PyExc_RuntimeError`.
The full list is available at :ref:`standardexceptions`.

With this knowledge, let's make our function raise a ``RuntimeError`` upon
failure:

.. code-block:: c
   :emphasize-lines: 10

   static PyObject *
   spam_system(PyObject *self, PyObject *arg)
   {
      const char *command = PyUnicode_AsUTF8(arg);
      if (command == NULL) {
         return NULL;
      }
      int status = system(command);
      if (status != 0) {
         PyErr_SetString(PyExc_RuntimeError, "system() call failed");
         return NULL;
      }

      // We don't know how to return None yet, so let's do this for now.
      return PyLong_FromLong(status);
   }

Now, if we run this:

.. code-block:: pycon

   >>> import spam
   >>> result = spam.system('noexist')
   RuntimeError: system() call failed


Yay! But, this isn't a very descriptive error message. It'd be nice if users
of ``system`` knew exactly what went wrong when invoking their command.

We can provide do this by using :c:func:`PyErr_Format`, which takes a format
string following by variadic arguments instead of a single constant string.
This is similar to ``printf`` in C. Let's try it:

.. code-block:: c
   :emphasize-lines: 10-11

   static PyObject *
   spam_system(PyObject *self, PyObject *arg)
   {
      const char *command = PyUnicode_AsUTF8(arg);
      if (command == NULL) {
         return NULL;
      }
      int status = system(command);
      if (status != 0) {
         PyErr_Format(PyExc_RuntimeError,
                      "system() returned non-zero exit code %d", status);
         return NULL;
      }

      // We don't know how to return None yet, so let's do this for now.
      return PyLong_FromLong(status);
   }


And if we try it, everything works as expected:


.. code-block:: pycon

   >>> import spam
   >>> result = spam.system('noexist')
   RuntimeError: system() returned non-zero exit code 127


But, our function still returns ``0`` if it succeeds, which is now useless.
Ideally, we should return ``None``, like a normal Python function would.
Our first instinct might be to return ``NULL``, so let's try it:

.. code-block:: c
   :emphasize-lines: 15

   static PyObject *
   spam_system(PyObject *self, PyObject *arg)
   {
      const char *command = PyUnicode_AsUTF8(arg);
      if (command == NULL) {
         return NULL;
      }
      int status = system(command);
      if (status != 0) {
         PyErr_Format(PyExc_RuntimeError,
                      "system() returned non-zero exit code %d", status);
         return NULL;
      }

      return NULL;
   }

.. code-block:: pycon

   >>> import spam
   >>> spam.system('true')
   SystemError: <built-in function system> returned NULL without setting an exception


Nope -- again, ``NULL`` is reserved for exceptions. In Python, ``None`` is still
an object, so we have to return a reference to it. We can do this by returning
a strong reference to :c:var:`Py_None`:


.. code-block:: c
   :emphasize-lines: 16

   static PyObject *
   spam_system(PyObject *self, PyObject *arg)
   {
      const char *command = PyUnicode_AsUTF8(arg);
      if (command == NULL) {
         return NULL;
      }
      int status = system(command);
      if (status != 0) {
         PyErr_Format(PyExc_RuntimeError,
                      "system() returned non-zero exit code %d", status);
         return NULL;
      }

      // Py_NewRef() is just a shorthand for Py_INCREF() with an expression
      return Py_NewRef(Py_None);
   }

.. note::

   In CPython, :const:`None` is actually an :term:`immortal` object, meaning
   that it has a fixed reference count and is never deallocated, and thus
   ``Py_INCREF`` has no real effect here.


In fact, this is so common that the C API has a macro for it:


.. code-block:: c
   :emphasize-lines: 15

   static PyObject *
   spam_system(PyObject *self, PyObject *arg)
   {
      const char *command = PyUnicode_AsUTF8(arg);
      if (command == NULL) {
         return NULL;
      }
      int status = system(command);
      if (status != 0) {
         PyErr_Format(PyExc_RuntimeError,
                      "system() returned non-zero exit code %d", status);
         return NULL;
      }

      Py_RETURN_NONE;
   }
