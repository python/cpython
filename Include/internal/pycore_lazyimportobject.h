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

PyAPI_FUNC(PyObject *) _PyLazyImport_New(
    struct _PyInterpreterFrame *frame, PyObject *builtins,
    PyObject *name, PyObject *fromlist);

extern PyObject *_PyLazyImport_LoadChild(
    PyThreadState *tstate, PyObject *declaration, PyObject *name);
extern int _PyLazyImport_IsActive(PyObject *declaration);

extern int _PyLazyImport_IsResolving(PyThreadState *tstate, PyObject *op);

// Resolve a placeholder and replace its binding if it is unchanged or holds
// the child module published by the normal importer during resolution.
// namespace is the source captured during lookup, before resolution runs.
PyAPI_FUNC(PyObject *) _PyLazyImport_Reify(
    PyThreadState *tstate, PyObject *placeholder,
    PyObject *name, PyObject *ns);

#ifdef __cplusplus
}
#endif
#endif // !Py_INTERNAL_LAZYIMPORTOBJECT_H
