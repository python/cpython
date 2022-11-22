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

   Get the *frame*'s ``f_builtins`` attribute.

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

   Get the *frame*'s ``f_globals`` attribute.

   Return a :term:`strong reference`. The result cannot be ``NULL``.

   .. versionadded:: 3.11


.. c:function:: int PyFrame_GetLasti(PyFrameObject *frame)

   Get the *frame*'s ``f_lasti`` attribute.

   Returns -1 if ``frame.f_lasti`` is ``None``.

   .. versionadded:: 3.11


.. c:function:: PyObject* PyFrame_GetLocals(PyFrameObject *frame)

   Get the *frame*'s ``f_locals`` attribute (:class:`dict`).

   Return a :term:`strong reference`.

   .. versionadded:: 3.11


.. c:function:: int PyFrame_GetLineNumber(PyFrameObject *frame)

   Return the line number that *frame* is currently executing.
