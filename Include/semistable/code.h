#ifndef Py_SEMISTABLE_CODE_H
#  error "this header file must not be included directly"
#endif

/* Functions added in PEP-523 */

_Py_NEWLY_SEMISTABLE(3.11)
PyAPI_FUNC(Py_ssize_t) PyEval_RequestCodeExtraIndex(freefunc);

_Py_NEWLY_SEMISTABLE(3.11)
PyAPI_FUNC(int) PyCode_GetExtra(PyObject *code, Py_ssize_t index, void **extra);

_Py_NEWLY_SEMISTABLE(3.11)
PyAPI_FUNC(int) PyCode_SetExtra(PyObject *code, Py_ssize_t index, void *extra);

/* Underscore-prefixed names for the above.
 * To be removed when the API changes.
 */
#define _PyEval_RequestCodeExtraIndex PyEval_RequestCodeExtraIndex
#define _PyCode_GetExtra PyCode_GetExtra
#define _PyCode_SetExtra PyCode_SetExtra


/* PyCodeObject constructors */

_Py_NEWLY_SEMISTABLE(3.11)
PyAPI_FUNC(PyCodeObject *) PyCode_New(
        int, int, int, int, int, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, int, PyObject *,
        PyObject *);

_Py_NEWLY_SEMISTABLE(3.11)
PyAPI_FUNC(PyCodeObject *) PyCode_NewWithPosOnlyArgs(
        int, int, int, int, int, int, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, int, PyObject *,
        PyObject *);
        /* same as struct PyCodeObject */
