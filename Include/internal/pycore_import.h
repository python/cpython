#ifndef Py_LIMITED_API
#ifndef Py_INTERNAL_IMPORT_H
#define Py_INTERNAL_IMPORT_H
#ifdef __cplusplus
extern "C" {
#endif


struct _import_runtime_state {
    /* A dict mapping (filename, name) to PyModuleDef for modules.
       Only legacy (single-phase init) extension modules are added
       and only if they support multiple initialization (m_size >- 0)
       or are imported in the main interpreter.
       This is initialized lazily in _PyImport_FixupExtensionObject().
       Modules are added there and looked up in _imp.find_extension(). */
    PyObject *extensions;
};


#ifdef HAVE_FORK
extern PyStatus _PyImport_ReInitLock(void);
#endif
extern PyObject* _PyImport_BootstrapImp(PyThreadState *tstate);

struct _module_alias {
    const char *name;                 /* ASCII encoded string */
    const char *orig;                 /* ASCII encoded string */
};

PyAPI_DATA(const struct _frozen *) _PyImport_FrozenBootstrap;
PyAPI_DATA(const struct _frozen *) _PyImport_FrozenStdlib;
PyAPI_DATA(const struct _frozen *) _PyImport_FrozenTest;
extern const struct _module_alias * _PyImport_FrozenAliases;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_IMPORT_H */
#endif /* !Py_LIMITED_API */
