.. highlight:: c

Frame Objects
-------------

.. c:type:: PyFrameObject

   The C structure of the objects used to describe frame objects.

   There are no public members in this structure.

   .. versionchanged:: 3.11
      The members of this structure were removed from the public C API.
      Refer to the :ref:`What's New entry <pyframeobject-3.11-hiding>`
      for details.

The :c:func:`PyEval_GetFrame` and :c:func:`PyThreadState_GetFrame` functions
can be used to get a frame object.

See also :ref:`Reflection <reflection>`.

.. c:var:: PyTypeObject PyFrame_Type

   The type of frame objects.
   It is the same object as :py:class:`types.FrameType` in the Python layer.

   .. versionchanged:: 3.11

      Previously, this type was only available after including
      ``<frameobject.h>``.

.. c:function:: PyFrameObject *PyFrame_New(PyThreadState *tstate, PyCodeObject *code, PyObject *globals, PyObject *locals)

   Create a new frame object. This function returns a :term:`strong reference`
   to the new frame object on success, and returns ``NULL`` with an exception
   set on failure.

.. c:function:: int PyFrame_Check(PyObject *obj)

   Return non-zero if *obj* is a frame object.

   .. versionchanged:: 3.11

      Previously, this function was only available after including
      ``<frameobject.h>``.

.. c:function:: PyFrameObject* PyFrame_GetBack(PyFrameObject *frame)

   Get the *frame* next outer frame.

   Return a :term:`strong reference`, or ``NULL`` if *frame* has no outer
   frame.

   .. versionadded:: 3.9


.. c:function:: PyObject* PyFrame_GetBuiltins(PyFrameObject *frame)

   Get the *frame*'s :attr:`~frame.f_builtins` attribute.

   Return a :term:`strong reference`. The result cannot be ``NULL``.

   .. versionadded:: 3.11


.. c:function:: PyCodeObject* PyFrame_GetCode(PyFrameObject *frame)

   Get the *frame* code.

   Return a :term:`strong reference`.

   The result (frame code) cannot be ``NULL``.

   .. versionadded:: 3.9


.. c:function:: PyObject* PyFrame_GetGenerator(PyFrameObject *frame)

   Get the generator, coroutine, or async generator that owns this frame,
   or ``NULL`` if this frame is not owned by a generator.
   Does not raise an exception, even if the return value is ``NULL``.

   Return a :term:`strong reference`, or ``NULL``.

   .. versionadded:: 3.11


.. c:function:: PyObject* PyFrame_GetGlobals(PyFrameObject *frame)

   Get the *frame*'s :attr:`~frame.f_globals` attribute.

   Return a :term:`strong reference`. The result cannot be ``NULL``.

   .. versionadded:: 3.11


.. c:function:: int PyFrame_GetLasti(PyFrameObject *frame)

   Get the *frame*'s :attr:`~frame.f_lasti` attribute.

   Returns -1 if ``frame.f_lasti`` is ``None``.

   .. versionadded:: 3.11


.. c:function:: PyObject* PyFrame_GetVar(PyFrameObject *frame, PyObject *name)

   Get the variable *name* of *frame*.

   * Return a :term:`strong reference` to the variable value on success.
   * Raise :exc:`NameError` and return ``NULL`` if the variable does not exist.
   * Raise an exception and return ``NULL`` on error.

   *name* type must be a :class:`str`.

   .. versionadded:: 3.12


.. c:function:: PyObject* PyFrame_GetVarString(PyFrameObject *frame, const char *name)

   Similar to :c:func:`PyFrame_GetVar`, but the variable name is a C string
   encoded in UTF-8.

   .. versionadded:: 3.12


.. c:function:: PyObject* PyFrame_GetLocals(PyFrameObject *frame)

   Get the *frame*'s :attr:`~frame.f_locals` attribute.
   If the frame refers to an :term:`optimized scope`, this returns a
   write-through proxy object that allows modifying the locals.
   In all other cases (classes, modules, :func:`exec`, :func:`eval`) it returns
   the mapping representing the frame locals directly (as described for
   :func:`locals`).

   Return a :term:`strong reference`.

   .. versionadded:: 3.11

   .. versionchanged:: 3.13
      As part of :pep:`667`, return an instance of :c:var:`PyFrameLocalsProxy_Type`.


.. c:function:: int PyFrame_GetLineNumber(PyFrameObject *frame)

   Return the line number that *frame* is currently executing.


Frame Locals Proxies
^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 3.13

The :attr:`~frame.f_locals` attribute on a :ref:`frame object <frame-objects>`
is an instance of a "frame-locals proxy". The proxy object exposes a
write-through view of the underlying locals dictionary for the frame. This
ensures that the variables exposed by ``f_locals`` are always up to date with
the live local variables in the frame itself.

See :pep:`667` for more information.

.. c:var:: PyTypeObject PyFrameLocalsProxy_Type

   The type of frame :func:`locals` proxy objects.

.. c:function:: int PyFrameLocalsProxy_Check(PyObject *obj)

   Return non-zero if *obj* is a frame :func:`locals` proxy.


Legacy Local Variable APIs
^^^^^^^^^^^^^^^^^^^^^^^^^^

These APIs are :term:`soft deprecated`. As of Python 3.13, they do nothing.
They exist solely for backwards compatibility.


.. c:function:: void PyFrame_LocalsToFast(PyFrameObject *f, int clear)

   This function is :term:`soft deprecated` and does nothing.

   Prior to Python 3.13, this function would copy the :attr:`~frame.f_locals`
   attribute of *f* to the internal "fast" array of local variables, allowing
   changes in frame objects to be visible to the interpreter. If *clear* was
   true, this function would process variables that were unset in the locals
   dictionary.

   .. versionchanged:: 3.13
      This function now does nothing.


.. c:function:: void PyFrame_FastToLocals(PyFrameObject *f)

   This function is :term:`soft deprecated` and does nothing.

   Prior to Python 3.13, this function would copy the internal "fast" array
   of local variables (which is used by the interpreter) to the
   :attr:`~frame.f_locals` attribute of *f*, allowing changes in local
   variables to be visible to frame objects.

   .. versionchanged:: 3.13
      This function now does nothing.


.. c:function:: int PyFrame_FastToLocalsWithError(PyFrameObject *f)

   This function is :term:`soft deprecated` and does nothing.

   Prior to Python 3.13, this function was similar to
   :c:func:`PyFrame_FastToLocals`, but would return ``0`` on success, and
   ``-1`` with an exception set on failure.

   .. versionchanged:: 3.13
      This function now does nothing.


.. seealso::
   :pep:`667`


Internal Frames
^^^^^^^^^^^^^^^

Unless using :pep:`523`, you will not need this.

.. c:struct:: _PyInterpreterFrame

   The interpreter's internal frame representation.

   .. versionadded:: 3.11

.. c:function:: PyObject* PyUnstable_InterpreterFrame_GetCode(struct _PyInterpreterFrame *frame);

    Return a :term:`strong reference` to the code object for the frame.

   .. versionadded:: 3.12


.. c:function:: int PyUnstable_InterpreterFrame_GetLasti(struct _PyInterpreterFrame *frame);

   Return the byte offset into the last executed instruction.

   .. versionadded:: 3.12


.. c:function:: int PyUnstable_InterpreterFrame_GetLine(struct _PyInterpreterFrame *frame);

   Return the currently executing line number, or -1 if there is no line number.

   .. versionadded:: 3.12


