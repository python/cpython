#ifndef Py_LIMITED_API
#ifndef Py_INTERNAL_IMPORT_H
#define Py_INTERNAL_IMPORT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyObject *) _PyImport_FindBuiltin(
    PyThreadState *tstate,
    const char *name             /* UTF-8 encoded string */
    );

#ifdef HAVE_FORK
extern PyStatus _PyImport_ReInitLock(void);
#endif
extern void _PyImport_Cleanup(PyThreadState *tstate);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_IMPORT_H */
#endif /* !Py_LIMITED_API */
