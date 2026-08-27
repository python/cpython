Pending removal in Python 3.20
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :c:func:`!_PyObject_CallMethodId`, :c:func:`!_PyObject_GetAttrId` and
  :c:func:`!_PyUnicode_FromId` are deprecated since 3.15 and will be removed in
  3.20. Instead, use :c:func:`PyUnicode_InternFromString()` and cache the result in
  the module state, then call :c:func:`PyObject_CallMethod` or
  :c:func:`PyObject_GetAttr`.
  (Contributed by Victor Stinner in :gh:`141049`.)

* The ``cval`` field in :c:type:`PyComplexObject` (:gh:`128813`).
  Use :c:func:`PyComplex_AsCComplex` and :c:func:`PyComplex_FromCComplex`
  to convert a Python complex number to/from the C :c:type:`Py_complex`
  representation.

* Macros :c:macro:`!Py_MATH_PIl` and :c:macro:`!Py_MATH_El`.
