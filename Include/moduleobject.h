
/* Module object interface */

#ifndef Py_MODULEOBJECT_H
#define Py_MODULEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

extern DL_IMPORT(PyTypeObject) PyModule_Type;

#define PyModule_Check(op) ((op)->ob_type == &PyModule_Type)

extern DL_IMPORT(PyObject *) PyModule_New(char *);
extern DL_IMPORT(PyObject *) PyModule_GetDict(PyObject *);
extern DL_IMPORT(char *) PyModule_GetName(PyObject *);
extern DL_IMPORT(char *) PyModule_GetFilename(PyObject *);
extern DL_IMPORT(void) _PyModule_Clear(PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODULEOBJECT_H */
