#ifndef Py_INTERNAL_TEMPLATE_H
#define Py_INTERNAL_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject_HEAD
    PyObject *args;
} PyTemplateObject;

extern PyTypeObject PyTemplate_Type;

PyAPI_FUNC(PyObject *) _PyTemplate_Create(PyObject **values, Py_ssize_t n);

extern PyStatus _PyTemplate_InitTypes(PyInterpreterState *);
extern void _PyTemplate_FiniTypes(PyInterpreterState *);

#ifdef __cplusplus
}
#endif

#endif
