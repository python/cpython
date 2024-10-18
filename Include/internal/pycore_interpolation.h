#ifndef Py_INTERNAL_INTERPOLATION_H
#define Py_INTERNAL_INTERPOLATION_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject_HEAD
    PyObject *value;
    PyObject *expr;
    PyObject *conv;
    PyObject *format_spec;
} PyInterpolationObject;

extern PyTypeObject PyInterpolation_Type;

PyAPI_FUNC(PyObject *) _PyInterpolation_FromStackRefSteal(_PyStackRef *values);

extern PyStatus _PyInterpolation_InitTypes(PyInterpreterState *);
extern void _PyInterpolation_FiniTypes(PyInterpreterState *);

#ifdef __cplusplus
}
#endif

#endif
