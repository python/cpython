// Lazy object interface.

#ifndef Py_INTERNAL_LAZYIMPORTOBJECT_H
#define Py_INTERNAL_LAZYIMPORTOBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

PyAPI_DATA(PyTypeObject) PyLazyImport_Type;
#define PyLazyImport_CheckExact(op) Py_IS_TYPE((op), &PyLazyImport_Type)

typedef struct {
    PyObject_HEAD
    PyObject *lz_builtins;
    PyObject *lz_from;
    PyObject *lz_attr;
    // Frame information for the original import location.
    PyCodeObject *lz_code;     // Code object where the lazy import was created.
    int lz_instr_offset;       // Instruction offset where the lazy import was created.
} PyLazyImportObject;


PyAPI_FUNC(PyObject *) _PyLazyImport_GetName(PyObject *lazy_import);
PyAPI_FUNC(PyObject *) _PyLazyImport_New(
    struct _PyInterpreterFrame *frame, PyObject *import_func, PyObject *from, PyObject *attr);

#ifdef __cplusplus
}
#endif
#endif // !Py_INTERNAL_LAZYIMPORTOBJECT_H
