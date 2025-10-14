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
#define _PyTemplateIter_CheckExact(op) Py_IS_TYPE((op), &_PyTemplateIter_Type)

extern PyObject *_PyTemplate_Concat(PyObject *self, PyObject *other);

PyAPI_FUNC(PyObject *) _PyTemplate_Build(PyObject *strings, PyObject *interpolations);

#ifdef __cplusplus
}
#endif

#endif
