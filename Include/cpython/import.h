#ifndef Py_CPYTHON_IMPORT_H
#  error "this header file must not be included directly"
#endif

struct _inittab {
    const char *name;           /* ASCII encoded string */
    PyObject* (*initfunc)(void);
};
// This is not used after Py_Initialize() is called.
PyAPI_DATA(struct _inittab *) PyImport_Inittab;
PyAPI_FUNC(int) PyImport_ExtendInittab(struct _inittab *newtab);

struct _frozen {
    const char *name;                 /* ASCII encoded string */
    const unsigned char *code;
    int size;
    int is_package;
};

/* Embedding apps may change this pointer to point to their favorite
   collection of frozen modules: */

PyAPI_DATA(const struct _frozen *) PyImport_FrozenModules;

PyAPI_FUNC(PyObject*) PyImport_ImportModuleAttr(
    PyObject *mod_name,
    PyObject *attr_name);
PyAPI_FUNC(PyObject*) PyImport_ImportModuleAttrString(
    const char *mod_name,
    const char *attr_name);
