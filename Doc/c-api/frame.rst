.. highlight:: c

Frame Objects
-------------

.. c:type:: PyFrameObject

   The C structure of the objects used to describe frame objects.

   The structure is not part of the C API.

   .. versionchanged:: 3.11
      The structure moved to the internal C API headers.

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


.. c:function:: PyObject* PyFrame_GetLocals(PyFrameObject *frame)

   Get the *frame*'s ``f_locals`` attribute (:class:`dict`).

   Return a :term:`strong reference`.

   *frame* must not be ``NULL``.

   .. versionadded:: 3.11


.. c:function:: int PyFrame_GetLineNumber(PyFrameObject *frame)

   Return the line number that *frame* is currently executing.

   *frame* must not be ``NULL``.
