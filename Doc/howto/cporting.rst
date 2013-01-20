.. highlightlang:: c

.. _cporting-howto:

*************************************
Porting Extension Modules to Python 3
*************************************

:author: Benjamin Peterson


.. topic:: Abstract

   Although changing the C-API was not one of Python 3's objectives,
   the many Python-level changes made leaving Python 2's API intact
   impossible.  In fact, some changes such as :func:`int` and
   :func:`long` unification are more obvious on the C level.  This
   document endeavors to document incompatibilities and how they can
   be worked around.


Conditional compilation
=======================

The easiest way to compile only some code for Python 3 is to check
if :c:macro:`PY_MAJOR_VERSION` is greater than or equal to 3. ::

   #if PY_MAJOR_VERSION >= 3
   #define IS_PY3K
   #endif

API functions that are not present can be aliased to their equivalents within
conditional blocks.


Changes to Object APIs
======================

Python 3 merged together some types with similar functions while cleanly
separating others.


str/unicode Unification
-----------------------


Python 3's :func:`str` (``PyString_*`` functions in C) type is equivalent to
Python 2's :func:`unicode` (``PyUnicode_*``).  The old 8-bit string type has
become :func:`bytes`.  Python 2.6 and later provide a compatibility header,
:file:`bytesobject.h`, mapping ``PyBytes`` names to ``PyString`` ones.  For best
compatibility with Python 3, :c:type:`PyUnicode` should be used for textual data and
:c:type:`PyBytes` for binary data.  It's also important to remember that
:c:type:`PyBytes` and :c:type:`PyUnicode` in Python 3 are not interchangeable like
:c:type:`PyString` and :c:type:`PyUnicode` are in Python 2.  The following example
shows best practices with regards to :c:type:`PyUnicode`, :c:type:`PyString`,
and :c:type:`PyBytes`. ::

   #include "stdlib.h"
   #include "Python.h"
   #include "bytesobject.h"

   /* text example */
   static PyObject *
   say_hello(PyObject *self, PyObject *args) {
       PyObject *name, *result;

       if (!PyArg_ParseTuple(args, "U:say_hello", &name))
           return NULL;

       result = PyUnicode_FromFormat("Hello, %S!", name);
       return result;
   }

   /* just a forward */
   static char * do_encode(PyObject *);

   /* bytes example */
   static PyObject *
   encode_object(PyObject *self, PyObject *args) {
       char *encoded;
       PyObject *result, *myobj;

       if (!PyArg_ParseTuple(args, "O:encode_object", &myobj))
           return NULL;

       encoded = do_encode(myobj);
       if (encoded == NULL)
           return NULL;
       result = PyBytes_FromString(encoded);
       free(encoded);
       return result;
   }


long/int Unification
--------------------

Python 3 has only one integer type, :func:`int`.  But it actually
corresponds to Python 2's :func:`long` type--the :func:`int` type
used in Python 2 was removed.  In the C-API, ``PyInt_*`` functions
are replaced by their ``PyLong_*`` equivalents.


Module initialization and state
===============================

Python 3 has a revamped extension module initialization system.  (See
:pep:`3121`.)  Instead of storing module state in globals, they should
be stored in an interpreter specific structure.  Creating modules that
act correctly in both Python 2 and Python 3 is tricky.  The following
simple example demonstrates how. ::

   #include "Python.h"

   struct module_state {
       PyObject *error;
   };

   #if PY_MAJOR_VERSION >= 3
   #define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
   #else
   #define GETSTATE(m) (&_state)
   static struct module_state _state;
   #endif

   static PyObject *
   error_out(PyObject *m) {
       struct module_state *st = GETSTATE(m);
       PyErr_SetString(st->error, "something bad happened");
       return NULL;
   }

   static PyMethodDef myextension_methods[] = {
       {"error_out", (PyCFunction)error_out, METH_NOARGS, NULL},
       {NULL, NULL}
   };

   #if PY_MAJOR_VERSION >= 3

   static int myextension_traverse(PyObject *m, visitproc visit, void *arg) {
       Py_VISIT(GETSTATE(m)->error);
       return 0;
   }

   static int myextension_clear(PyObject *m) {
       Py_CLEAR(GETSTATE(m)->error);
       return 0;
   }


   static struct PyModuleDef moduledef = {
           PyModuleDef_HEAD_INIT,
           "myextension",
           NULL,
           sizeof(struct module_state),
           myextension_methods,
           NULL,
           myextension_traverse,
           myextension_clear,
           NULL
   };

   #define INITERROR return NULL

   PyObject *
   PyInit_myextension(void)

   #else
   #define INITERROR return

   void
   initmyextension(void)
   #endif
   {
   #if PY_MAJOR_VERSION >= 3
       PyObject *module = PyModule_Create(&moduledef);
   #else
       PyObject *module = Py_InitModule("myextension", myextension_methods);
   #endif

       if (module == NULL)
           INITERROR;
       struct module_state *st = GETSTATE(module);

       st->error = PyErr_NewException("myextension.Error", NULL, NULL);
       if (st->error == NULL) {
           Py_DECREF(module);
           INITERROR;
       }

   #if PY_MAJOR_VERSION >= 3
       return module;
   #endif
   }


CObject replaced with Capsule
=============================

The :c:type:`Capsule` object was introduced in Python 3.1 and 2.7 to replace
:c:type:`CObject`.  CObjects were useful,
but the :c:type:`CObject` API was problematic: it didn't permit distinguishing
between valid CObjects, which allowed mismatched CObjects to crash the
interpreter, and some of its APIs relied on undefined behavior in C.
(For further reading on the rationale behind Capsules, please see :issue:`5630`.)

If you're currently using CObjects, and you want to migrate to 3.1 or newer,
you'll need to switch to Capsules.
:c:type:`CObject` was deprecated in 3.1 and 2.7 and completely removed in
Python 3.2.  If you only support 2.7, or 3.1 and above, you
can simply switch to :c:type:`Capsule`.  If you need to support Python 3.0,
or versions of Python earlier than 2.7,
you'll have to support both CObjects and Capsules.
(Note that Python 3.0 is no longer supported, and it is not recommended
for production use.)

The following example header file :file:`capsulethunk.h` may
solve the problem for you.  Simply write your code against the
:c:type:`Capsule` API and include this header file after
:file:`Python.h`.  Your code will automatically use Capsules
in versions of Python with Capsules, and switch to CObjects
when Capsules are unavailable.

:file:`capsulethunk.h` simulates Capsules using CObjects.  However,
:c:type:`CObject` provides no place to store the capsule's "name".  As a
result the simulated :c:type:`Capsule` objects created by :file:`capsulethunk.h`
behave slightly differently from real Capsules.  Specifically:

  * The name parameter passed in to :c:func:`PyCapsule_New` is ignored.

  * The name parameter passed in to :c:func:`PyCapsule_IsValid` and
    :c:func:`PyCapsule_GetPointer` is ignored, and no error checking
    of the name is performed.

  * :c:func:`PyCapsule_GetName` always returns NULL.

  * :c:func:`PyCapsule_SetName` always raises an exception and
    returns failure.  (Since there's no way to store a name
    in a CObject, noisy failure of :c:func:`PyCapsule_SetName`
    was deemed preferable to silent failure here.  If this is
    inconvenient, feel free to modify your local
    copy as you see fit.)

You can find :file:`capsulethunk.h` in the Python source distribution
as :source:`Doc/includes/capsulethunk.h`.  We also include it here for
your convenience:

.. literalinclude:: ../includes/capsulethunk.h



Other options
=============

If you are writing a new extension module, you might consider `Cython
<http://www.cython.org>`_.  It translates a Python-like language to C.  The
extension modules it creates are compatible with Python 3 and Python 2.

