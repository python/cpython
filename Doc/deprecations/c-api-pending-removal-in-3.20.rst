Pending removal in Python 3.20
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* The ``cval`` field in :c:type:`PyComplexObject` (:gh:`128813`).
  Use :c:func:`PyComplex_AsCComplex` and :c:func:`PyComplex_FromCComplex`
  to convert a Python complex number to/from the C :c:type:`Py_complex`
  representation.
