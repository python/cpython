/* File added for Lazy Imports */

/* Lazy object interface */

#ifndef Py_LAZYIMPORTOBJECT_H
#define Py_LAZYIMPORTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PyLazyImport_Type;
#define PyLazyImport_CheckExact(op) Py_IS_TYPE((op), &PyLazyImport_Type)

typedef struct {
    PyObject_HEAD
    PyObject *lz_builtins;
    PyObject *lz_from;
    PyObject *lz_attr;
    /* Frame information for the original import location */
    PyCodeObject *lz_code;     /* code object where the lazy import was created */
    int lz_instr_offset;       /* instruction offset where the lazy import was created */
} PyLazyImportObject;


PyAPI_FUNC(PyObject *) _PyLazyImport_GetName(PyObject *lazy_import);
PyAPI_FUNC(PyObject *) _PyLazyImport_New(PyObject *import_func, PyObject *from, PyObject *attr);

#ifdef __cplusplus
}
#endif
#endif /* !Py_LAZYIMPORTOBJECT_H */
