#ifndef Py_CPYTHON_CEVAL_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(void) PyEval_SetProfile(Py_tracefunc, PyObject *);
PyAPI_FUNC(void) PyEval_SetTrace(Py_tracefunc, PyObject *);

/* Helper to look up a builtin object */
PyAPI_FUNC(PyObject *) _PyEval_GetBuiltin(PyObject *);
PyAPI_FUNC(PyObject *) _PyEval_GetBuiltinId(_Py_Identifier *);

/* Look at the current frame's (if any) code's co_flags, and turn on
   the corresponding compiler flags in cf->cf_flags.  Return 1 if any
   flag was set, else return 0. */
PyAPI_FUNC(int) PyEval_MergeCompilerFlags(PyCompilerFlags *cf);

PyAPI_FUNC(int) _PyEval_SliceIndex(PyObject *, Py_ssize_t *);
PyAPI_FUNC(int) _PyEval_SliceIndexNotNone(PyObject *, Py_ssize_t *);
