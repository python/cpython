.. highlight:: c

.. _codeobjects:

.. index:: object; code, code object

Code Objects
------------

.. sectionauthor:: Jeffrey Yasskin <jyasskin@gmail.com>

Code objects are a low-level detail of the CPython implementation.
Each one represents a chunk of executable code that hasn't yet been
bound into a function.

.. c:type:: PyCodeObject

   The C structure of the objects used to describe code objects.  The
   fields of this type are subject to change at any time.


.. c:var:: PyTypeObject PyCode_Type

   This is an instance of :c:type:`PyTypeObject` representing the Python
   :class:`code` type.


.. c:function:: int PyCode_Check(PyObject *co)

   Return true if *co* is a :class:`code` object.

.. c:function:: int PyCode_GetNumFree(PyCodeObject *co)

   Return the number of free variables in *co*.

.. c:function:: PyCodeObject* PyCode_New(int argcount, int posonlyargcount, int kwonlyargcount, int nlocals, int stacksize, int flags, PyObject *code, PyObject *consts, PyObject *names, PyObject *varnames, PyObject *freevars, PyObject *cellvars, PyObject *filename, PyObject *name, int firstlineno, PyObject *lnotab)

   Return a new code object.  If you need a dummy code object to
   create a frame, use :c:func:`PyCode_NewEmpty` instead.  Calling
   :c:func:`PyCode_New` directly can bind you to a precise Python
   version since the definition of the bytecode changes often.

   .. versionchanged:: 3.8
      An extra parameter is required (*posonlyargcount*) to support :PEP:`570`.
      The first parameter (*argcount*) now represents the total number of positional arguments,
      including positional-only.

   .. audit-event:: code.__new__ "code filename name argcount posonlyargcount kwonlyargcount nlocals stacksize flags"

.. c:function:: PyCodeObject* PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno)

   Return a new empty code object with the specified filename,
   function name, and first line number.  It is illegal to
   :func:`exec` or :func:`eval` the resulting code object.
