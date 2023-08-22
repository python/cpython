.. highlight:: c

.. index:: object; code, code object

.. _codeobjects:

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

.. c:function:: PyCodeObject* PyCode_New(int argcount, int kwonlyargcount, int nlocals, int stacksize, int flags, PyObject *code, PyObject *consts, PyObject *names, PyObject *varnames, PyObject *freevars, PyObject *cellvars, PyObject *filename, PyObject *name, PyObject *qualname, int firstlineno, PyObject *linetable, PyObject *exceptiontable)

   Return a new code object.  If you need a dummy code object to create a frame,
   use :c:func:`PyCode_NewEmpty` instead.  Calling :c:func:`PyCode_New` directly
   will bind you to a precise Python version since the definition of the bytecode
   changes often. The many arguments of this function are inter-dependent in complex
   ways, meaning that subtle changes to values are likely to result in incorrect
   execution or VM crashes. Use this function only with extreme care.

   .. versionchanged:: 3.11
      Added ``qualname`` and ``exceptiontable`` parameters.

.. c:function:: PyCodeObject* PyCode_NewWithPosOnlyArgs(int argcount, int posonlyargcount, int kwonlyargcount, int nlocals, int stacksize, int flags, PyObject *code, PyObject *consts, PyObject *names, PyObject *varnames, PyObject *freevars, PyObject *cellvars, PyObject *filename, PyObject *name, PyObject *qualname, int firstlineno, PyObject *linetable, PyObject *exceptiontable)

   Similar to :c:func:`PyCode_New`, but with an extra "posonlyargcount" for positional-only arguments.
   The same caveats that apply to ``PyCode_New`` also apply to this function.

   .. versionadded:: 3.8

   .. versionchanged:: 3.11
      Added ``qualname`` and  ``exceptiontable`` parameters.

.. c:function:: PyCodeObject* PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno)

   Return a new empty code object with the specified filename,
   function name, and first line number. The resulting code
   object will raise an ``Exception`` if executed.

.. c:function:: int PyCode_Addr2Line(PyCodeObject *co, int byte_offset)

    Return the line number of the instruction that occurs on or before ``byte_offset`` and ends after it.
    If you just need the line number of a frame, use :c:func:`PyFrame_GetLineNumber` instead.

    For efficiently iterating over the line numbers in a code object, use `the API described in PEP 626
    <https://peps.python.org/pep-0626/#out-of-process-debuggers-and-profilers>`_.

.. c:function:: int PyCode_Addr2Location(PyObject *co, int byte_offset, int *start_line, int *start_column, int *end_line, int *end_column)

   Sets the passed ``int`` pointers to the source code line and column numbers
   for the instruction at ``byte_offset``. Sets the value to ``0`` when
   information is not available for any particular element.

   Returns ``1`` if the function succeeds and 0 otherwise.

   .. versionadded:: 3.11

.. c:function:: PyObject* PyCode_GetCode(PyCodeObject *co)

   Equivalent to the Python code ``getattr(co, 'co_code')``.
   Returns a strong reference to a :c:type:`PyBytesObject` representing the
   bytecode in a code object. On error, ``NULL`` is returned and an exception
   is raised.

   This ``PyBytesObject`` may be created on-demand by the interpreter and does
   not necessarily represent the bytecode actually executed by CPython. The
   primary use case for this function is debuggers and profilers.

   .. versionadded:: 3.11

.. c:function:: PyObject* PyCode_GetVarnames(PyCodeObject *co)

   Equivalent to the Python code ``getattr(co, 'co_varnames')``.
   Returns a new reference to a :c:type:`PyTupleObject` containing the names of
   the local variables. On error, ``NULL`` is returned and an exception
   is raised.

   .. versionadded:: 3.11

.. c:function:: PyObject* PyCode_GetCellvars(PyCodeObject *co)

   Equivalent to the Python code ``getattr(co, 'co_cellvars')``.
   Returns a new reference to a :c:type:`PyTupleObject` containing the names of
   the local variables that are referenced by nested functions. On error, ``NULL``
   is returned and an exception is raised.

   .. versionadded:: 3.11

.. c:function:: PyObject* PyCode_GetFreevars(PyCodeObject *co)

   Equivalent to the Python code ``getattr(co, 'co_freevars')``.
   Returns a new reference to a :c:type:`PyTupleObject` containing the names of
   the free variables. On error, ``NULL`` is returned and an exception is raised.

   .. versionadded:: 3.11
