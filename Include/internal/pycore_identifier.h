/* String Literals: _Py_Identifier API */

#ifndef Py_INTERNAL_IDENTIFIER_H
#define Py_INTERNAL_IDENTIFIER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyObject* _PyType_LookupId(PyTypeObject *, _Py_Identifier *);
extern PyObject* _PyObject_LookupSpecialId(PyObject *, _Py_Identifier *);
extern int _PyObject_SetAttrId(PyObject *, _Py_Identifier *, PyObject *);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_IDENTIFIER_H
