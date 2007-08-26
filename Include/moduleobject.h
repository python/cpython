
/* Module object interface */

#ifndef Py_MODULEOBJECT_H
#define Py_MODULEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyModule_Type;

#define PyModule_Check(op) PyObject_TypeCheck(op, &PyModule_Type)
#define PyModule_CheckExact(op) (Py_Type(op) == &PyModule_Type)

PyAPI_FUNC(PyObject *) PyModule_New(const char *);
PyAPI_FUNC(PyObject *) PyModule_GetDict(PyObject *);
PyAPI_FUNC(const char *) PyModule_GetName(PyObject *);
PyAPI_FUNC(const char *) PyModule_GetFilename(PyObject *);
PyAPI_FUNC(void) _PyModule_Clear(PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODULEOBJECT_H */
