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

   Return true if *co* is a :class:`code` object.  This function always succeeds.

.. c:function:: int PyCode_GetNumFree(PyCodeObject *co)

   Return the number of free variables in *co*.

.. c:function:: PyCodeObject* PyCode_New(int argcount, int kwonlyargcount, int nlocals, int stacksize, int flags, PyObject *code, PyObject *consts, PyObject *names, PyObject *varnames, PyObject *freevars, PyObject *cellvars, PyObject *filename, PyObject *name, int firstlineno, PyObject *lnotab)

   Return a new code object.  If you need a dummy code object to create a frame,
   use :c:func:`PyCode_NewEmpty` instead.  Calling :c:func:`PyCode_New` directly
   can bind you to a precise Python version since the definition of the bytecode
   changes often.

.. c:function:: PyCodeObject* PyCode_NewWithPosOnlyArgs(int argcount, int posonlyargcount, int kwonlyargcount, int nlocals, int stacksize, int flags, PyObject *code, PyObject *consts, PyObject *names, PyObject *varnames, PyObject *freevars, PyObject *cellvars, PyObject *filename, PyObject *name, int firstlineno, PyObject *lnotab)

   Similar to :c:func:`PyCode_New`, but with an extra "posonlyargcount" for positional-only arguments.

   .. versionadded:: 3.8

.. c:function:: PyCodeObject* PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno)

   Return a new empty code object with the specified filename,
   function name, and first line number.  It is illegal to
   :func:`exec` or :func:`eval` the resulting code object.

.. c:function:: int PyCode_Addr2Line(PyCodeObject *co, int byte_offset)

    Return the line number of the instruction that occurs on or before ``byte_offset`` and ends after it.
    If you just need the line number of a frame, use :c:func:`PyFrame_GetLineNumber` instead.

    For efficiently iterating over the line numbers in a code object, use `the API described in PEP 626
    <https://www.python.org/dev/peps/pep-0626/#out-of-process-debuggers-and-profilers>`_.

.. c:function:: int PyCode_Addr2Location(PyObject *co, int byte_offset, int *start_line, int *start_column, int *end_line, int *end_column)

   Sets the passed ``int`` pointers to the source code line and column numbers
   for the instruction at ``byte_offset``. Sets the value to ``0`` when
   information is not available for any particular element.

   Returns ``1`` if the function succeeds and 0 otherwise.
