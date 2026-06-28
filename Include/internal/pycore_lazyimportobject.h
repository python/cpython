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
    // Protected by the import lock. lz_owner_* records the first module
    // global that resolve() may replace. Pending owners are being stored;
    // finalized owners cannot be claimed again.
    PyObject *lz_owner_globals;
    PyObject *lz_owner_key;
    PyObject *lz_resolved;
    // Frame information for the original import location.
    PyCodeObject *lz_code;     // Code object where the lazy import was created.
    int lz_owner_pending;
    int lz_owner_finalized;
    int lz_instr_offset;       // Instruction offset where the lazy import was created.
} PyLazyImportObject;


PyAPI_FUNC(PyObject *) _PyLazyImport_GetName(PyObject *lazy_import);
PyAPI_FUNC(PyObject *) _PyLazyImport_New(
    struct _PyInterpreterFrame *frame, PyObject *import_func, PyObject *from, PyObject *attr);
PyAPI_FUNC(int) _PyLazyImport_BindGlobal(
    PyThreadState *tstate,
    PyObject *lazy_import,
    PyObject *globals,
    PyObject *name);
PyAPI_FUNC(void) _PyLazyImport_FinishGlobalBinding(
    PyThreadState *tstate,
    PyObject *lazy_import,
    PyObject *globals,
    PyObject *name,
    int stored);
PyAPI_FUNC(int) _PyLazyImport_CommitIfCurrent(
    PyThreadState *tstate,
    PyObject *lazy_import,
    PyObject *globals,
    PyObject *name,
    PyObject *resolved);
PyAPI_FUNC(int) _PyLazyImport_CommitStoredIfCurrent(
    PyThreadState *tstate,
    PyObject *lazy_import,
    PyObject *resolved);

#ifdef __cplusplus
}
#endif
#endif // !Py_INTERNAL_LAZYIMPORTOBJECT_H
