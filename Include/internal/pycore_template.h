#ifndef Py_INTERNAL_TEMPLATE_H
#define Py_INTERNAL_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyTypeObject _PyTemplate_Type;
extern PyTypeObject _PyTemplateIter_Type;

#define _PyTemplate_CheckExact(op) Py_IS_TYPE((op), &_PyTemplate_Type)

extern PyObject *_PyTemplate_Concat(PyObject *self, PyObject *other);

PyAPI_FUNC(PyObject *) _PyTemplate_FromValues(PyObject **values, Py_ssize_t n);
PyAPI_FUNC(PyObject *) _PyTemplate_FromList(PyObject *list);

#ifdef __cplusplus
}
#endif

#endif
