Pending removal in Python 3.20
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* The ``cval`` field in :c:type:`PyComplexObject` (:gh:`128813`).
  Use :c:func:`PyComplex_AsCComplex` and :c:func:`PyComplex_FromCComplex`
  to convert a Python complex number to/from the C :c:type:`Py_complex`
  representation.

* Macros :c:macro:`!Py_MATH_PIl` and :c:macro:`!Py_MATH_El`.

* :c:func:`!_PyInterpreterState_GetEvalFrameFunc` and
  :c:func:`!_PyInterpreterState_SetEvalFrameFunc` functions are deprecated
  and will be removed in Python 3.20. Use
  :c:func:`PyUnstable_InterpreterState_GetEvalFrameFunc` and
  :c:func:`PyUnstable_InterpreterState_SetEvalFrameFunc` instead.
  (Contributed by Victor Stinner in :gh:`141518`.)
