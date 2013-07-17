
/* Support for dynamic loading of extension modules */

#include "Python.h"

/* ./configure sets HAVE_DYNAMIC_LOADING if dynamic loading of modules is
   supported on this platform. configure will then compile and link in one
   of the dynload_*.c files, as appropriate. We will call a function in
   those modules to get a function pointer to the module's init function.
*/
#ifdef HAVE_DYNAMIC_LOADING

#include "importdl.h"

#ifdef MS_WINDOWS
extern dl_funcptr _PyImport_GetDynLoadWindows(const char *shortname,
                                              PyObject *pathname, FILE *fp);
#else
extern dl_funcptr _PyImport_GetDynLoadFunc(const char *shortname,
                                           const char *pathname, FILE *fp);
#endif

PyObject *
_PyImport_LoadDynamicModule(PyObject *name, PyObject *path, FILE *fp)
{
    PyObject *m = NULL;
#ifndef MS_WINDOWS
    PyObject *pathbytes;
#endif
    PyObject *nameascii;
    char *namestr, *lastdot, *shortname, *packagecontext, *oldcontext;
    dl_funcptr p0;
    PyObject* (*p)(void);
    struct PyModuleDef *def;

    m = _PyImport_FindExtensionObject(name, path);
    if (m != NULL) {
        Py_INCREF(m);
        return m;
    }

    /* name must be encodable to ASCII because dynamic module must have a
       function called "PyInit_NAME", they are written in C, and the C language
       doesn't accept non-ASCII identifiers. */
    nameascii = PyUnicode_AsEncodedString(name, "ascii", NULL);
    if (nameascii == NULL)
        return NULL;

    namestr = PyBytes_AS_STRING(nameascii);
    if (namestr == NULL)
        goto error;

    lastdot = strrchr(namestr, '.');
    if (lastdot == NULL) {
        packagecontext = NULL;
        shortname = namestr;
    }
    else {
        packagecontext = namestr;
        shortname = lastdot+1;
    }

#ifdef MS_WINDOWS
    p0 = _PyImport_GetDynLoadWindows(shortname, path, fp);
#else
    pathbytes = PyUnicode_EncodeFSDefault(path);
    if (pathbytes == NULL)
        goto error;
    p0 = _PyImport_GetDynLoadFunc(shortname,
                                  PyBytes_AS_STRING(pathbytes), fp);
    Py_DECREF(pathbytes);
#endif
    p = (PyObject*(*)(void))p0;
    if (PyErr_Occurred())
        goto error;
    if (p == NULL) {
        PyObject *msg = PyUnicode_FromFormat("dynamic module does not define "
                                             "init function (PyInit_%s)",
                                             shortname);
        if (msg == NULL)
            goto error;
        PyErr_SetImportError(msg, name, path);
        Py_DECREF(msg);
        goto error;
    }
    oldcontext = _Py_PackageContext;
    _Py_PackageContext = packagecontext;
    m = (*p)();
    _Py_PackageContext = oldcontext;
    if (m == NULL)
        goto error;

    if (PyErr_Occurred()) {
        PyErr_Format(PyExc_SystemError,
                     "initialization of %s raised unreported exception",
                     shortname);
        goto error;
    }

    /* Remember pointer to module init function. */
    def = PyModule_GetDef(m);
    if (def == NULL) {
        PyErr_Format(PyExc_SystemError,
                     "initialization of %s did not return an extension "
                     "module", shortname);
        goto error;
    }
    def->m_base.m_init = p;

    /* Remember the filename as the __file__ attribute */
    if (PyModule_AddObject(m, "__file__", path) < 0)
        PyErr_Clear(); /* Not important enough to report */
    else
        Py_INCREF(path);

    if (_PyImport_FixupExtensionObject(m, name, path) < 0)
        goto error;
    Py_DECREF(nameascii);
    return m;

error:
    Py_DECREF(nameascii);
    Py_XDECREF(m);
    return NULL;
}

#endif /* HAVE_DYNAMIC_LOADING */
