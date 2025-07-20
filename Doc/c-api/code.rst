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
   :ref:`code object <code-objects>`.


.. c:function:: int PyCode_Check(PyObject *co)

   Return true if *co* is a :ref:`code object <code-objects>`.
   This function always succeeds.

.. c:function:: Py_ssize_t PyCode_GetNumFree(PyCodeObject *co)

   Return the number of :term:`free (closure) variables <closure variable>`
   in a code object.

.. c:function:: int PyUnstable_Code_GetFirstFree(PyCodeObject *co)

   Return the position of the first :term:`free (closure) variable <closure variable>`
   in a code object.

   .. versionchanged:: 3.13

      Renamed from ``PyCode_GetFirstFree`` as part of :ref:`unstable-c-api`.
      The old name is deprecated, but will remain available until the
      signature changes again.

.. c:function:: PyCodeObject* PyUnstable_Code_New(int argcount, int kwonlyargcount, int nlocals, int stacksize, int flags, PyObject *code, PyObject *consts, PyObject *names, PyObject *varnames, PyObject *freevars, PyObject *cellvars, PyObject *filename, PyObject *name, PyObject *qualname, int firstlineno, PyObject *linetable, PyObject *exceptiontable)

   Return a new code object.  If you need a dummy code object to create a frame,
   use :c:func:`PyCode_NewEmpty` instead.

   Since the definition of the bytecode changes often, calling
   :c:func:`PyUnstable_Code_New` directly can bind you to a precise Python version.

   The many arguments of this function are inter-dependent in complex
   ways, meaning that subtle changes to values are likely to result in incorrect
   execution or VM crashes. Use this function only with extreme care.

   .. versionchanged:: 3.11
      Added ``qualname`` and ``exceptiontable`` parameters.

   .. index:: single: PyCode_New (C function)

   .. versionchanged:: 3.12

      Renamed from ``PyCode_New`` as part of :ref:`unstable-c-api`.
      The old name is deprecated, but will remain available until the
      signature changes again.

.. c:function:: PyCodeObject* PyUnstable_Code_NewWithPosOnlyArgs(int argcount, int posonlyargcount, int kwonlyargcount, int nlocals, int stacksize, int flags, PyObject *code, PyObject *consts, PyObject *names, PyObject *varnames, PyObject *freevars, PyObject *cellvars, PyObject *filename, PyObject *name, PyObject *qualname, int firstlineno, PyObject *linetable, PyObject *exceptiontable)

   Similar to :c:func:`PyUnstable_Code_New`, but with an extra "posonlyargcount" for positional-only arguments.
   The same caveats that apply to ``PyUnstable_Code_New`` also apply to this function.

   .. index:: single: PyCode_NewWithPosOnlyArgs (C function)

   .. versionadded:: 3.8 as ``PyCode_NewWithPosOnlyArgs``

   .. versionchanged:: 3.11
      Added ``qualname`` and  ``exceptiontable`` parameters.

   .. versionchanged:: 3.12

      Renamed to ``PyUnstable_Code_NewWithPosOnlyArgs``.
      The old name is deprecated, but will remain available until the
      signature changes again.

.. c:function:: PyCodeObject* PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno)

   Return a new empty code object with the specified filename,
   function name, and first line number. The resulting code
   object will raise an ``Exception`` if executed.

.. c:function:: int PyCode_Addr2Line(PyCodeObject *co, int byte_offset)

    Return the line number of the instruction that occurs on or before ``byte_offset`` and ends after it.
    If you just need the line number of a frame, use :c:func:`PyFrame_GetLineNumber` instead.

    For efficiently iterating over the line numbers in a code object, use :pep:`the API described in PEP 626
    <0626#out-of-process-debuggers-and-profilers>`.

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
   the :term:`free (closure) variables <closure variable>`. On error, ``NULL`` is returned
   and an exception is raised.

   .. versionadded:: 3.11

.. c:function:: int PyCode_AddWatcher(PyCode_WatchCallback callback)

   Register *callback* as a code object watcher for the current interpreter.
   Return an ID which may be passed to :c:func:`PyCode_ClearWatcher`.
   In case of error (e.g. no more watcher IDs available),
   return ``-1`` and set an exception.

   .. versionadded:: 3.12

.. c:function:: int PyCode_ClearWatcher(int watcher_id)

   Clear watcher identified by *watcher_id* previously returned from
   :c:func:`PyCode_AddWatcher` for the current interpreter.
   Return ``0`` on success, or ``-1`` and set an exception on error
   (e.g. if the given *watcher_id* was never registered.)

   .. versionadded:: 3.12

.. c:type:: PyCodeEvent

   Enumeration of possible code object watcher events:
   - ``PY_CODE_EVENT_CREATE``
   - ``PY_CODE_EVENT_DESTROY``

   .. versionadded:: 3.12

.. c:type:: int (*PyCode_WatchCallback)(PyCodeEvent event, PyCodeObject* co)

   Type of a code object watcher callback function.

   If *event* is ``PY_CODE_EVENT_CREATE``, then the callback is invoked
   after *co* has been fully initialized. Otherwise, the callback is invoked
   before the destruction of *co* takes place, so the prior state of *co*
   can be inspected.

   If *event* is ``PY_CODE_EVENT_DESTROY``, taking a reference in the callback
   to the about-to-be-destroyed code object will resurrect it and prevent it
   from being freed at this time. When the resurrected object is destroyed
   later, any watcher callbacks active at that time will be called again.

   Users of this API should not rely on internal runtime implementation
   details. Such details may include, but are not limited to, the exact
   order and timing of creation and destruction of code objects. While
   changes in these details may result in differences observable by watchers
   (including whether a callback is invoked or not), it does not change
   the semantics of the Python code being executed.

   If the callback sets an exception, it must return ``-1``; this exception will
   be printed as an unraisable exception using :c:func:`PyErr_WriteUnraisable`.
   Otherwise it should return ``0``.

   There may already be a pending exception set on entry to the callback. In
   this case, the callback should return ``0`` with the same exception still
   set. This means the callback may not call any other API that can set an
   exception unless it saves and clears the exception state first, and restores
   it before returning.

   .. versionadded:: 3.12


.. _c_codeobject_flags:

Code Object Flags
-----------------

Code objects contain a bit-field of flags, which can be retrieved as the
:attr:`~codeobject.co_flags` Python attribute (for example using
:c:func:`PyObject_GetAttrString`), and set using a *flags* argument to
:c:func:`PyUnstable_Code_New` and similar functions.

Flags whose names start with ``CO_FUTURE_`` correspond to features normally
selectable by :ref:`future statements <future>`. These flags can be used in
:c:member:`PyCompilerFlags.cf_flags`.
Note that many ``CO_FUTURE_`` flags are mandatory in current versions of
Python, and setting them has no effect.

The following flags are available.
For their meaning, see the linked documentation of their Python equivalents.


.. list-table::
   :widths: auto
   :header-rows: 1

   * * Flag
     * Meaning
   * * .. c:macro:: CO_OPTIMIZED
     * :py:data:`inspect.CO_OPTIMIZED`
   * * .. c:macro:: CO_NEWLOCALS
     * :py:data:`inspect.CO_NEWLOCALS`
   * * .. c:macro:: CO_VARARGS
     * :py:data:`inspect.CO_VARARGS`
   * * .. c:macro:: CO_VARKEYWORDS
     * :py:data:`inspect.CO_VARKEYWORDS`
   * * .. c:macro:: CO_NESTED
     * :py:data:`inspect.CO_NESTED`
   * * .. c:macro:: CO_GENERATOR
     * :py:data:`inspect.CO_GENERATOR`
   * * .. c:macro:: CO_COROUTINE
     * :py:data:`inspect.CO_COROUTINE`
   * * .. c:macro:: CO_ITERABLE_COROUTINE
     * :py:data:`inspect.CO_ITERABLE_COROUTINE`
   * * .. c:macro:: CO_ASYNC_GENERATOR
     * :py:data:`inspect.CO_ASYNC_GENERATOR`
   * * .. c:macro:: CO_HAS_DOCSTRING
     * :py:data:`inspect.CO_HAS_DOCSTRING`
   * * .. c:macro:: CO_METHOD
     * :py:data:`inspect.CO_METHOD`

   * * .. c:macro:: CO_FUTURE_DIVISION
     * no effect (:py:data:`__future__.division`)
   * * .. c:macro:: CO_FUTURE_ABSOLUTE_IMPORT
     * no effect (:py:data:`__future__.absolute_import`)
   * * .. c:macro:: CO_FUTURE_WITH_STATEMENT
     * no effect (:py:data:`__future__.with_statement`)
   * * .. c:macro:: CO_FUTURE_PRINT_FUNCTION
     * no effect (:py:data:`__future__.print_function`)
   * * .. c:macro:: CO_FUTURE_UNICODE_LITERALS
     * no effect (:py:data:`__future__.unicode_literals`)
   * * .. c:macro:: CO_FUTURE_GENERATOR_STOP
     * no effect (:py:data:`__future__.generator_stop`)
   * * .. c:macro:: CO_FUTURE_ANNOTATIONS
     * :py:data:`__future__.annotations`


Extra information
-----------------

To support low-level extensions to frame evaluation, such as external
just-in-time compilers, it is possible to attach arbitrary extra data to
code objects.

These functions are part of the unstable C API tier:
this functionality is a CPython implementation detail, and the API
may change without deprecation warnings.

.. c:function:: Py_ssize_t PyUnstable_Eval_RequestCodeExtraIndex(freefunc free)

   Return a new an opaque index value used to adding data to code objects.

   You generally call this function once (per interpreter) and use the result
   with ``PyCode_GetExtra`` and ``PyCode_SetExtra`` to manipulate
   data on individual code objects.

   If *free* is not ``NULL``: when a code object is deallocated,
   *free* will be called on non-``NULL`` data stored under the new index.
   Use :c:func:`Py_DecRef` when storing :c:type:`PyObject`.

   .. index:: single: _PyEval_RequestCodeExtraIndex (C function)

   .. versionadded:: 3.6 as ``_PyEval_RequestCodeExtraIndex``

   .. versionchanged:: 3.12

     Renamed to ``PyUnstable_Eval_RequestCodeExtraIndex``.
     The old private name is deprecated, but will be available until the API
     changes.

.. c:function:: int PyUnstable_Code_GetExtra(PyObject *code, Py_ssize_t index, void **extra)

   Set *extra* to the extra data stored under the given index.
   Return 0 on success. Set an exception and return -1 on failure.

   If no data was set under the index, set *extra* to ``NULL`` and return
   0 without setting an exception.

   .. index:: single: _PyCode_GetExtra (C function)

   .. versionadded:: 3.6 as ``_PyCode_GetExtra``

   .. versionchanged:: 3.12

     Renamed to ``PyUnstable_Code_GetExtra``.
     The old private name is deprecated, but will be available until the API
     changes.

.. c:function:: int PyUnstable_Code_SetExtra(PyObject *code, Py_ssize_t index, void *extra)

   Set the extra data stored under the given index to *extra*.
   Return 0 on success. Set an exception and return -1 on failure.

   .. index:: single: _PyCode_SetExtra (C function)

   .. versionadded:: 3.6 as ``_PyCode_SetExtra``

   .. versionchanged:: 3.12

     Renamed to ``PyUnstable_Code_SetExtra``.
     The old private name is deprecated, but will be available until the API
     changes.
