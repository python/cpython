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
   use :c:func:`PyCode_NewEmpty` instead.

   Since the definition of the bytecode changes often, calling
   :c:func:`PyCode_New` directly can bind you to a precise Python version.
   This function is  part of the semi-stable C API.
   See :c:macro:`Py_USING_SEMI_STABLE_API` for usage.

   .. versionchanged:: 3.11

     Use without ``Py_USING_SEMI_STABLE_API`` is deprecated.

.. c:function:: PyCodeObject* PyCode_NewWithPosOnlyArgs(int argcount, int posonlyargcount, int kwonlyargcount, int nlocals, int stacksize, int flags, PyObject *code, PyObject *consts, PyObject *names, PyObject *varnames, PyObject *freevars, PyObject *cellvars, PyObject *filename, PyObject *name, int firstlineno, PyObject *lnotab)

   Similar to :c:func:`PyCode_New`, but with an extra "posonlyargcount" for positional-only arguments.

   .. versionadded:: 3.8

   .. versionchanged:: 3.11

     Use without ``Py_USING_SEMI_STABLE_API`` is deprecated.

.. c:function:: PyCodeObject* PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno)

   Return a new empty code object with the specified filename,
   function name, and first line number.  It is illegal to
   :func:`exec` or :func:`eval` the resulting code object.

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


Extra information
-----------------

To support low-level extensions to frame evaluation, such as external
just-in-time compilers, it is possible to attach arbitrary extra data to
code objects.

This functionality is a CPython implementation detail, and the API
may change without deprecation warnings.
These functions are part of the semi-stable C API.
See :c:macro:`Py_USING_SEMI_STABLE_API` for details.

See :pep:`523` for motivation and initial specification behind this API.


.. c:function:: Py_ssize_t PyEval_RequestCodeExtraIndex(freefunc free)

   Return a new an opaque index value used to adding data to code objects.

   You generally call this function once (per interpreter) and use the result
   with ``PyCode_GetExtra`` and ``PyCode_SetExtra`` to manipulate
   data on individual code objects.

   If *free* is not ``NULL``: when a code object is deallocated,
   *free* will be called on non-``NULL`` data stored under the new index.
   Use :c:func:`Py_DecRef` when storing :c:type:`PyObject`.

   Part of the semi-stable API, see :c:macro:`Py_USING_SEMI_STABLE_API`
   for usage.

   .. versionadded:: 3.6 as ``_PyEval_RequestCodeExtraIndex``

   .. versionchanged:: 3.11

     Renamed to ``PyEval_RequestCodeExtraIndex`` (without the leading
     undersecore). The old name is available as an alias.

     Use without ``Py_USING_SEMI_STABLE_API`` is deprecated.

.. c:function:: int PyCode_GetExtra(PyObject *code, Py_ssize_t index, void **extra)

   Set *extra* to the extra data stored under the given index.
   Return 0 on success. Set an exception and return -1 on failure.

   If no data was set under the index, set *extra* to ``NULL`` and return
   0 without setting an exception.

   Part of the semi-stable API, see :c:macro:`Py_USING_SEMI_STABLE_API`
   for usage.

   .. versionadded:: 3.6 as ``_PyCode_GetExtra``

   .. versionchanged:: 3.11

     Renamed to ``PyCode_GetExtra`` (without the leading undersecore).
     The old name is available as an alias.

     Use without ``Py_USING_SEMI_STABLE_API`` is deprecated.

.. c:function:: int PyCode_SetExtra(PyObject *code, Py_ssize_t index, void *extra)

   Set the extra data stored under the given index to *extra*.
   Return 0 on success. Set an exception and return -1 on failure.

   Part of the semi-stable API, see :c:macro:`Py_USING_SEMI_STABLE_API`
   for usage.

   .. versionadded:: 3.6 3.6 as ``_PyCode_SetExtra``

   .. versionchanged:: 3.11

     Renamed to ``PyCode_SetExtra`` (without the leading undersecore).
     The old name is available as an alias.

     Use without ``Py_USING_SEMI_STABLE_API`` is deprecated.
