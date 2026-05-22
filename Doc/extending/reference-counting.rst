.. highlight:: c


.. _reference-counting-intro:


*************************************
An introduction to reference counting
*************************************

This chapter covers the basics of CPython's garbage collection scheme.

What is reference counting?
===========================

In CPython, objects are garbage collected through a scheme known as
"reference counting". This means that all objects keep count of the number
of references to them.

For example, take the following code:

.. code-block:: python

   a = object()  # refcount: 1

In the above code, the ``object()`` has a single reference (``a``), so it has
reference count of 1. If we add more references, the reference count will
increase:

.. code-block:: python

   a = object()  # refcount: 1
   b = a  # refcount: 2
   c = b  # refcount: 3


When a name is unbinded, the reference count is decremented. If the reference
count of an object reaches zero, the object is immediately deallocated.

We can visualize this using the :meth:`~object.__del__` method:

.. code-block:: pycon

   >>> class Test:
   ...     def __del__(self):
   ...         print("Deleting")
   >>> a = Test()  # refcount: 1
   >>> del a  # refcount: 0
   Deleting


Object references in the C API
==============================

In the C API, all objects are represented by a pointer to a :c:type:`PyObject`.
This is known as a "reference".
For our purposes, the ``PyObject`` structure contains two important pieces of
information:

1. The object's type, accessible through :c:macro:`Py_TYPE`.
2. The object's :term:`reference count`, accessible through :c:macro:`Py_REFCNT`.

When using the C API, we need to manage the reference count of an object on our
own. Or, in other words, we need to tell Python where and when we are using an
object. This is done through two macros:

1. :c:macro:`Py_INCREF`, which increments the object's reference count.
2. :c:macro:`Py_DECREF`, which decrements the object's reference count.
   If the object's reference count becomes zero, the object's destructor is
   invoked.

To understand how this works in practice, let's go back to our ``system``
function, taking note of ``PyObject *`` uses this time:

.. code-block:: c
   :emphasize-lines: 1-2, 9

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

Again, each ``PyObject *`` is a reference. There are two types of references
in the C API:

1. :term:`Strong references <strong reference>`, in which you are responsible
   for calling :c:macro:`Py_DECREF` (or otherwise handing off the reference).
   At the end of a function, all strong references should have either been
   destroyed or handed off (such as by returning it).
2. :term:`Borrowed references <borrowed reference>`, in which you are *not*
   responsible for destroying the reference.

In the ``spam_system`` function, ``self`` and ``arg`` are borrowed references
(meaning we must not decrement their reference count), but ``result`` is a
strong reference. ``result`` is returned, so the strong reference is given to
the caller.
The caller is now responsible for making sure :c:macro:`Py_DECREF` is called --
either by calling it, or by delegating this responsibility.


Reference counting patterns
===========================

In Python's C API, most functions will return a strong reference, and as such,
you need to release those references when you are done with them. For example,
let's say that we wanted to change our ``system`` function to only accept ASCII
strings as an input. We would first call :c:func:`PyUnicode_AsASCIIString` to
convert the string to a Python :class:`bytes` object, and then use
:c:macro:`PyBytes_AS_STRING` to extract the internal ``const char *`` buffer
from it.

To visualize:

.. code-block:: c
   :emphasize-lines: 4-8, 10

   static PyObject *
   spam_system(PyObject *self, PyObject *arg)
   {
      PyObject *bytes = PyUnicode_AsASCIIString(arg); // Strong reference
      if (bytes == NULL) {
         return NULL;
      }
      const char *command = PyBytes_AS_STRING(bytes);
      int status = system(command);
      Py_DECREF(bytes); // Release the strong reference
      PyObject *result = PyLong_FromLong(status);
      return result;
   }

Note that we have to call ``Py_DECREF(bytes)`` *after* we call ``system``.
If we did it before, then the string returned by ``PyBytes_AS_STRING``
might be freed and cause a crash upon trying to use it in ``system``.


The pitfalls of reference counting
==================================

As mentioned previously, *most* functions will return a strong reference, but not
all of them! In the above example, if ``PyUnicode_AsASCIIString`` were to
return a borrowed reference, then there would be a use-after-free somewhere
down the call stack.

Unfortunately, there is no way to determine whether a reference is strong or
borrowed just by looking at it. This can lead to many memory-safety bugs,
and to make matters worse, debugging bugs of this nature is often very difficult.

For example, let's add a bug to ``spam_system`` where we release a borrowed
reference:

.. code-block:: c
   :emphasize-lines: 5

   static PyObject *
   spam_system(PyObject *self, PyObject *arg)
   {
      const char *command = PyUnicode_AsUTF8(arg);
      Py_DECREF(arg); // refcount: 0!!!!
      if (command == NULL) {
         return NULL;
      }
      int status = system(command);
      PyObject *result = PyLong_FromLong(status);
      return result;
   }


Running the above code will result in a crash, but *not* in the
``spam_system`` function. In fact, ``spam_system`` won't even show up in the
stack trace. The crash occurs after ``spam_system`` returns and the *caller*
tries to release its reference to ``arg``, but since we stole the reference,
``arg`` is now invalid. This can make it very difficult to track down where
a reference counting error was made.

Another common error is forgetting to release a strong reference, in which case
the object will leak its memory. This is known as a "reference leak".
In this case, tools such as `Memray <https://bloomberg.github.io/memray/>`_
are able to identify which objects are leaking, which does make debugging
a little bit easier, but objects often hold references to many other objects,
which will *also* leak, making it even harder to find the cause of the leak.

Because CPython does not track where reference counts are incremented and
decremented, reference counting bugs are notoriously difficult to identify and
fix. This is one of the reasons many developers choose to use other programming
languages and tools when interfacing with Python's C API.
