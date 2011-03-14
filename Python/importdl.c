
/* Support for dynamic loading of extension modules */

#include "Python.h"

/* ./configure sets HAVE_DYNAMIC_LOADING if dynamic loading of modules is
   supported on this platform. configure will then compile and link in one
   of the dynload_*.c files, as appropriate. We will call a function in
   those modules to get a function pointer to the module's init function.
*/
#ifdef HAVE_DYNAMIC_LOADING

#include "importdl.h"

extern dl_funcptr _PyImport_GetDynLoadFunc(const char *shortname,
                                           const char *pathname, FILE *fp);

/* name should be ASCII only because the C language doesn't accept non-ASCII
   identifiers, and dynamic modules are written in C. */

PyObject *
_PyImport_LoadDynamicModule(PyObject *name, PyObject *path, FILE *fp)
{
    PyObject *m;
    PyObject *pathbytes;
    char *namestr, *lastdot, *shortname, *packagecontext, *oldcontext;
    dl_funcptr p0;
    PyObject* (*p)(void);
    struct PyModuleDef *def;

    namestr = _PyUnicode_AsString(name);
    if (namestr == NULL)
        return NULL;

    m = _PyImport_FindExtensionObject(name, path);
    if (m != NULL) {
        Py_INCREF(m);
        return m;
    }

    lastdot = strrchr(namestr, '.');
    if (lastdot == NULL) {
        packagecontext = NULL;
        shortname = namestr;
    }
    else {
        packagecontext = namestr;
        shortname = lastdot+1;
    }

    pathbytes = PyUnicode_EncodeFSDefault(path);
    if (pathbytes == NULL)
        return NULL;
    p0 = _PyImport_GetDynLoadFunc(shortname,
                                  PyBytes_AS_STRING(pathbytes), fp);
    Py_DECREF(pathbytes);
    p = (PyObject*(*)(void))p0;
    if (PyErr_Occurred())
        return NULL;
    if (p == NULL) {
        PyErr_Format(PyExc_ImportError,
                     "dynamic module does not define init function"
                     " (PyInit_%s)",
                     shortname);
        return NULL;
    }
    oldcontext = _Py_PackageContext;
    _Py_PackageContext = packagecontext;
    m = (*p)();
    _Py_PackageContext = oldcontext;
    if (m == NULL)
        return NULL;

    if (PyErr_Occurred()) {
        Py_DECREF(m);
        PyErr_Format(PyExc_SystemError,
                     "initialization of %s raised unreported exception",
                     shortname);
        return NULL;
    }

    /* Remember pointer to module init function. */
    def = PyModule_GetDef(m);
    def->m_base.m_init = p;

    /* Remember the filename as the __file__ attribute */
    if (PyModule_AddObject(m, "__file__", path) < 0)
        PyErr_Clear(); /* Not important enough to report */
    else
        Py_INCREF(path);

    if (_PyImport_FixupExtensionObject(m, name, path) < 0)
        return NULL;
    if (Py_VerboseFlag)
        PySys_FormatStderr(
            "import %U # dynamically loaded from %R\n",
            name, path);
    return m;
}

#endif /* HAVE_DYNAMIC_LOADING */
