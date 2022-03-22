.. highlight:: c

Frame Objects
-------------

.. c:type:: PyFrameObject

   The C structure of the objects used to describe frame objects.

   The structure is only part of the internal C API: fields should not be
   access directly. Use getter functions like :c:func:`PyFrame_GetCode` and
   :c:func:`PyFrame_GetBack`.

   Debuggers and profilers can use the internal C API to access this structure
   without calling functions, but the internal C API doesn't provide any
   backward compatibility warranty.

   .. versionchanged:: 3.11
      The structure moved to the internal C API headers.

Public members:

* ``f_back`` (read only): Next outer frame object (this frame's caller).
  See also: :c:func:`PyFrame_GetBack`.
* ``f_builtins`` (read only): Builtins namespace seen by this frame.
* ``f_code`` (read only): Code object being executed in this frame.
  See also :c:func:`PyFrame_GetCode`.
* ``f_globals`` (read only): Global namespace seen by this frame.
* ``f_lasti`` (read only): Index of last attempted instruction in bytecode.
* ``f_lineno``: Current line number in Python source code.
  See also :c:func:`PyFrame_GetLineNumber`.
* ``f_locals`` (read only): Local namespace seen by this frame.
* ``f_trace_lines``: Emit ``PyTrace_LINE`` trace events?
* ``f_trace_opcodes``: Emit ``PyTrace_OPCODE`` trace events?
* ``f_trace``: Tracing function for this frame, or ``None``

The :c:func:`PyObject_GetAttrString` and :c:func:`PyObject_SetAttrString`
functions can be used to get and set these members. For example,
``PyObject_GetAttrString((PyObject*)frame, "f_builtins")`` gets the frame
builtins namespace.

The :c:func:`PyEval_GetFrame` and :c:func:`PyThreadState_GetFrame` functions
can be used to get a frame object.

See also :ref:`Reflection <reflection>`.


.. c:function:: PyFrameObject* PyFrame_GetBack(PyFrameObject *frame)

   Get the *frame* next outer frame.

   Return a :term:`strong reference`, or ``NULL`` if *frame* has no outer
   frame.

   *frame* must not be ``NULL``.

   .. versionadded:: 3.9


.. c:function:: PyCodeObject* PyFrame_GetCode(PyFrameObject *frame)

   Get the *frame* code.

   Return a :term:`strong reference`.

   *frame* must not be ``NULL``. The result (frame code) cannot be ``NULL``.

   .. versionadded:: 3.9


.. c:function:: int PyFrame_GetLineNumber(PyFrameObject *frame)

   Return the line number that *frame* is currently executing.

   *frame* must not be ``NULL``.
