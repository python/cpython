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
    // Protected by the lazy object lock.
    PyObject *lz_globals;
    PyObject *lz_key;
    PyObject *lz_resolved;
    // Frame information for the original import location.
    PyCodeObject *lz_code;     // Code object where the lazy import was created.
    int lz_instr_offset;       // Instruction offset where the lazy import was created.
} PyLazyImportObject;


PyAPI_FUNC(PyObject *) _PyLazyImport_GetName(PyObject *lazy_import);
extern PyObject * _PyLazyImport_GetResolved(PyObject *lazy_import);
extern int _PyLazyImport_FinishResolve(
    PyObject *lazy_import, PyObject *resolved);
PyAPI_FUNC(int) _PyLazyImport_ReplaceDictItemIfCurrent(
    PyObject *lazy_import, PyObject *dict, PyObject *name,
    PyObject *resolved);
PyAPI_FUNC(int) _PyLazyImport_SetGlobalBindingAndDictItem(
    PyObject *lazy_import, PyObject *globals, PyObject *name);
PyAPI_FUNC(PyObject *) _PyLazyImport_New(
    struct _PyInterpreterFrame *frame, PyObject *import_func, PyObject *from, PyObject *attr);

#ifdef __cplusplus
}
#endif
#endif // !Py_INTERNAL_LAZYIMPORTOBJECT_H
