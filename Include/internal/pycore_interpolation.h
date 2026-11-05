#ifndef Py_INTERNAL_INTERPOLATION_H
#define Py_INTERNAL_INTERPOLATION_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyTypeObject _PyInterpolation_Type;

#define _PyInterpolation_CheckExact(op) Py_IS_TYPE((op), &_PyInterpolation_Type)

PyAPI_FUNC(PyObject *) _PyInterpolation_Build(PyObject *value, PyObject *str,
                                              int conversion, PyObject *format_spec);

extern PyStatus _PyInterpolation_InitTypes(PyInterpreterState *interp);
extern PyObject *_PyInterpolation_GetValueRef(PyObject *interpolation);

#ifdef __cplusplus
}
#endif

#endif
