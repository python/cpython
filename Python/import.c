
/* Module definition and import implementation */

#include "Python.h"

#include "Python-ast.h"
#undef Yield /* undefine macro conflicting with winbase.h */
#include "errcode.h"
#include "marshal.h"
#include "code.h"
#include "frameobject.h"
#include "osdefs.h"
#include "importdl.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define CACHEDIR "__pycache__"

/* See _PyImport_FixupExtensionObject() below */
static PyObject *extensions = NULL;

/* This table is defined in config.c: */
extern struct _inittab _PyImport_Inittab[];

struct _inittab *PyImport_Inittab = _PyImport_Inittab;

static PyObject *initstr = NULL;

/*[clinic input]
module _imp
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=9c332475d8686284]*/

#include "clinic/import.c.h"

/* Initialize things */

void
_PyImport_Init(void)
{
    PyInterpreterState *interp = PyThreadState_Get()->interp;
    initstr = PyUnicode_InternFromString("__init__");
    if (initstr == NULL)
        Py_FatalError("Can't initialize import variables");
    interp->builtins_copy = PyDict_Copy(interp->builtins);
    if (interp->builtins_copy == NULL)
        Py_FatalError("Can't backup builtins dict");
}

void
_PyImportHooks_Init(void)
{
    PyObject *v, *path_hooks = NULL;
    int err = 0;

    /* adding sys.path_hooks and sys.path_importer_cache */
    v = PyList_New(0);
    if (v == NULL)
        goto error;
    err = PySys_SetObject("meta_path", v);
    Py_DECREF(v);
    if (err)
        goto error;
    v = PyDict_New();
    if (v == NULL)
        goto error;
    err = PySys_SetObject("path_importer_cache", v);
    Py_DECREF(v);
    if (err)
        goto error;
    path_hooks = PyList_New(0);
    if (path_hooks == NULL)
        goto error;
    err = PySys_SetObject("path_hooks", path_hooks);
    if (err) {
  error:
    PyErr_Print();
    Py_FatalError("initializing sys.meta_path, sys.path_hooks, "
                  "or path_importer_cache failed");
    }
    Py_DECREF(path_hooks);
}

void
_PyImportZip_Init(void)
{
    PyObject *path_hooks, *zimpimport;
    int err = 0;

    path_hooks = PySys_GetObject("path_hooks");
    if (path_hooks == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "unable to get sys.path_hooks");
        goto error;
    }

    if (Py_VerboseFlag)
        PySys_WriteStderr("# installing zipimport hook\n");

    zimpimport = PyImport_ImportModule("zipimport");
    if (zimpimport == NULL) {
        PyErr_Clear(); /* No zip import module -- okay */
        if (Py_VerboseFlag)
            PySys_WriteStderr("# can't import zipimport\n");
    }
    else {
        _Py_IDENTIFIER(zipimporter);
        PyObject *zipimporter = _PyObject_GetAttrId(zimpimport,
                                                    &PyId_zipimporter);
        Py_DECREF(zimpimport);
        if (zipimporter == NULL) {
            PyErr_Clear(); /* No zipimporter object -- okay */
            if (Py_VerboseFlag)
                PySys_WriteStderr(
                    "# can't import zipimport.zipimporter\n");
        }
        else {
            /* sys.path_hooks.insert(0, zipimporter) */
            err = PyList_Insert(path_hooks, 0, zipimporter);
            Py_DECREF(zipimporter);
            if (err < 0) {
                goto error;
            }
            if (Py_VerboseFlag)
                PySys_WriteStderr(
                    "# installed zipimport hook\n");
        }
    }

    return;

  error:
    PyErr_Print();
    Py_FatalError("initializing zipimport failed");
}

/* Locking primitives to prevent parallel imports of the same module
   in different threads to return with a partially loaded module.
   These calls are serialized by the global interpreter lock. */

#ifdef WITH_THREAD

#include "pythread.h"

static PyThread_type_lock import_lock = 0;
static long import_lock_thread = -1;
static int import_lock_level = 0;

void
_PyImport_AcquireLock(void)
{
    long me = PyThread_get_thread_ident();
    if (me == -1)
        return; /* Too bad */
    if (import_lock == NULL) {
        import_lock = PyThread_allocate_lock();
        if (import_lock == NULL)
            return;  /* Nothing much we can do. */
    }
    if (import_lock_thread == me) {
        import_lock_level++;
        return;
    }
    if (import_lock_thread != -1 || !PyThread_acquire_lock(import_lock, 0))
    {
        PyThreadState *tstate = PyEval_SaveThread();
        PyThread_acquire_lock(import_lock, 1);
        PyEval_RestoreThread(tstate);
    }
    assert(import_lock_level == 0);
    import_lock_thread = me;
    import_lock_level = 1;
}

int
_PyImport_ReleaseLock(void)
{
    long me = PyThread_get_thread_ident();
    if (me == -1 || import_lock == NULL)
        return 0; /* Too bad */
    if (import_lock_thread != me)
        return -1;
    import_lock_level--;
    assert(import_lock_level >= 0);
    if (import_lock_level == 0) {
        import_lock_thread = -1;
        PyThread_release_lock(import_lock);
    }
    return 1;
}

/* This function is called from PyOS_AfterFork to ensure that newly
   created child processes do not share locks with the parent.
   We now acquire the import lock around fork() calls but on some platforms
   (Solaris 9 and earlier? see isue7242) that still left us with problems. */

void
_PyImport_ReInitLock(void)
{
    if (import_lock != NULL) {
        import_lock = PyThread_allocate_lock();
        if (import_lock == NULL) {
            Py_FatalError("PyImport_ReInitLock failed to create a new lock");
        }
    }
    if (import_lock_level > 1) {
        /* Forked as a side effect of import */
        long me = PyThread_get_thread_ident();
        /* The following could fail if the lock is already held, but forking as
           a side-effect of an import is a) rare, b) nuts, and c) difficult to
           do thanks to the lock only being held when doing individual module
           locks per import. */
        PyThread_acquire_lock(import_lock, NOWAIT_LOCK);
        import_lock_thread = me;
        import_lock_level--;
    } else {
        import_lock_thread = -1;
        import_lock_level = 0;
    }
}

#endif

/*[clinic input]
_imp.lock_held

Return True if the import lock is currently held, else False.

On platforms without threads, return False.
[clinic start generated code]*/

static PyObject *
_imp_lock_held_impl(PyObject *module)
/*[clinic end generated code: output=8b89384b5e1963fc input=9b088f9b217d9bdf]*/
{
#ifdef WITH_THREAD
    return PyBool_FromLong(import_lock_thread != -1);
#else
    return PyBool_FromLong(0);
#endif
}

/*[clinic input]
_imp.acquire_lock

Acquires the interpreter's import lock for the current thread.

This lock should be used by import hooks to ensure thread-safety when importing
modules. On platforms without threads, this function does nothing.
[clinic start generated code]*/

static PyObject *
_imp_acquire_lock_impl(PyObject *module)
/*[clinic end generated code: output=1aff58cb0ee1b026 input=4a2d4381866d5fdc]*/
{
#ifdef WITH_THREAD
    _PyImport_AcquireLock();
#endif
    Py_INCREF(Py_None);
    return Py_None;
}

/*[clinic input]
_imp.release_lock

Release the interpreter's import lock.

On platforms without threads, this function does nothing.
[clinic start generated code]*/

static PyObject *
_imp_release_lock_impl(PyObject *module)
/*[clinic end generated code: output=7faab6d0be178b0a input=934fb11516dd778b]*/
{
#ifdef WITH_THREAD
    if (_PyImport_ReleaseLock() < 0) {
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
        return NULL;
    }
#endif
    Py_INCREF(Py_None);
    return Py_None;
}

void
_PyImport_Fini(void)
{
    Py_CLEAR(extensions);
#ifdef WITH_THREAD
    if (import_lock != NULL) {
        PyThread_free_lock(import_lock);
        import_lock = NULL;
    }
#endif
}

/* Helper for sys */

PyObject *
PyImport_GetModuleDict(void)
{
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    if (interp->modules == NULL)
        Py_FatalError("PyImport_GetModuleDict: no module dictionary!");
    return interp->modules;
}


/* List of names to clear in sys */
static const char * const sys_deletes[] = {
    "path", "argv", "ps1", "ps2",
    "last_type", "last_value", "last_traceback",
    "path_hooks", "path_importer_cache", "meta_path",
    "__interactivehook__",
    /* misc stuff */
    "flags", "float_info",
    NULL
};

static const char * const sys_files[] = {
    "stdin", "__stdin__",
    "stdout", "__stdout__",
    "stderr", "__stderr__",
    NULL
};

/* Un-initialize things, as good as we can */

void
PyImport_Cleanup(void)
{
    Py_ssize_t pos;
    PyObject *key, *value, *dict;
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    PyObject *modules = interp->modules;
    PyObject *weaklist = NULL;
    const char * const *p;

    if (modules == NULL)
        return; /* Already done */

    /* Delete some special variables first.  These are common
       places where user values hide and people complain when their
       destructors fail.  Since the modules containing them are
       deleted *last* of all, they would come too late in the normal
       destruction order.  Sigh. */

    /* XXX Perhaps these precautions are obsolete. Who knows? */

    if (Py_VerboseFlag)
        PySys_WriteStderr("# clear builtins._\n");
    PyDict_SetItemString(interp->builtins, "_", Py_None);

    for (p = sys_deletes; *p != NULL; p++) {
        if (Py_VerboseFlag)
            PySys_WriteStderr("# clear sys.%s\n", *p);
        PyDict_SetItemString(interp->sysdict, *p, Py_None);
    }
    for (p = sys_files; *p != NULL; p+=2) {
        if (Py_VerboseFlag)
            PySys_WriteStderr("# restore sys.%s\n", *p);
        value = PyDict_GetItemString(interp->sysdict, *(p+1));
        if (value == NULL)
            value = Py_None;
        PyDict_SetItemString(interp->sysdict, *p, value);
    }

    /* We prepare a list which will receive (name, weakref) tuples of
       modules when they are removed from sys.modules.  The name is used
       for diagnosis messages (in verbose mode), while the weakref helps
       detect those modules which have been held alive. */
    weaklist = PyList_New(0);
    if (weaklist == NULL)
        PyErr_Clear();

#define STORE_MODULE_WEAKREF(name, mod) \
    if (weaklist != NULL) { \
        PyObject *wr = PyWeakref_NewRef(mod, NULL); \
        if (name && wr) { \
            PyObject *tup = PyTuple_Pack(2, name, wr); \
            PyList_Append(weaklist, tup); \
            Py_XDECREF(tup); \
        } \
        Py_XDECREF(wr); \
        if (PyErr_Occurred()) \
            PyErr_Clear(); \
    }

    /* Remove all modules from sys.modules, hoping that garbage collection
       can reclaim most of them. */
    pos = 0;
    while (PyDict_Next(modules, &pos, &key, &value)) {
        if (PyModule_Check(value)) {
            if (Py_VerboseFlag && PyUnicode_Check(key))
                PySys_FormatStderr("# cleanup[2] removing %U\n", key);
            STORE_MODULE_WEAKREF(key, value);
            PyDict_SetItem(modules, key, Py_None);
        }
    }

    /* Clear the modules dict. */
    PyDict_Clear(modules);
    /* Restore the original builtins dict, to ensure that any
       user data gets cleared. */
    dict = PyDict_Copy(interp->builtins);
    if (dict == NULL)
        PyErr_Clear();
    PyDict_Clear(interp->builtins);
    if (PyDict_Update(interp->builtins, interp->builtins_copy))
        PyErr_Clear();
    Py_XDECREF(dict);
    /* Clear module dict copies stored in the interpreter state */
    _PyState_ClearModules();
    /* Collect references */
    _PyGC_CollectNoFail();
    /* Dump GC stats before it's too late, since it uses the warnings
       machinery. */
    _PyGC_DumpShutdownStats();

    /* Now, if there are any modules left alive, clear their globals to
       minimize potential leaks.  All C extension modules actually end
       up here, since they are kept alive in the interpreter state.

       The special treatment of "builtins" here is because even
       when it's not referenced as a module, its dictionary is
       referenced by almost every module's __builtins__.  Since
       deleting a module clears its dictionary (even if there are
       references left to it), we need to delete the "builtins"
       module last.  Likewise, we don't delete sys until the very
       end because it is implicitly referenced (e.g. by print). */
    if (weaklist != NULL) {
        Py_ssize_t i, n;
        n = PyList_GET_SIZE(weaklist);
        for (i = 0; i < n; i++) {
            PyObject *tup = PyList_GET_ITEM(weaklist, i);
            PyObject *name = PyTuple_GET_ITEM(tup, 0);
            PyObject *mod = PyWeakref_GET_OBJECT(PyTuple_GET_ITEM(tup, 1));
            if (mod == Py_None)
                continue;
            assert(PyModule_Check(mod));
            dict = PyModule_GetDict(mod);
            if (dict == interp->builtins || dict == interp->sysdict)
                continue;
            Py_INCREF(mod);
            if (Py_VerboseFlag && PyUnicode_Check(name))
                PySys_FormatStderr("# cleanup[3] wiping %U\n", name);
            _PyModule_Clear(mod);
            Py_DECREF(mod);
        }
        Py_DECREF(weaklist);
    }

    /* Next, delete sys and builtins (in that order) */
    if (Py_VerboseFlag)
        PySys_FormatStderr("# cleanup[3] wiping sys\n");
    _PyModule_ClearDict(interp->sysdict);
    if (Py_VerboseFlag)
        PySys_FormatStderr("# cleanup[3] wiping builtins\n");
    _PyModule_ClearDict(interp->builtins);

    /* Clear and delete the modules directory.  Actual modules will
       still be there only if imported during the execution of some
       destructor. */
    interp->modules = NULL;
    Py_DECREF(modules);

    /* Once more */
    _PyGC_CollectNoFail();

#undef STORE_MODULE_WEAKREF
}


/* Helper for pythonrun.c -- return magic number and tag. */

long
PyImport_GetMagicNumber(void)
{
    long res;
    PyInterpreterState *interp = PyThreadState_Get()->interp;
    PyObject *external, *pyc_magic;

    external = PyObject_GetAttrString(interp->importlib, "_bootstrap_external");
    if (external == NULL)
        return -1;
    pyc_magic = PyObject_GetAttrString(external, "_RAW_MAGIC_NUMBER");
    Py_DECREF(external);
    if (pyc_magic == NULL)
        return -1;
    res = PyLong_AsLong(pyc_magic);
    Py_DECREF(pyc_magic);
    return res;
}


extern const char * _PySys_ImplCacheTag;

const char *
PyImport_GetMagicTag(void)
{
    return _PySys_ImplCacheTag;
}


/* Magic for extension modules (built-in as well as dynamically
   loaded).  To prevent initializing an extension module more than
   once, we keep a static dictionary 'extensions' keyed by the tuple
   (module name, module name)  (for built-in modules) or by
   (filename, module name) (for dynamically loaded modules), containing these
   modules.  A copy of the module's dictionary is stored by calling
   _PyImport_FixupExtensionObject() immediately after the module initialization
   function succeeds.  A copy can be retrieved from there by calling
   _PyImport_FindExtensionObject().

   Modules which do support multiple initialization set their m_size
   field to a non-negative number (indicating the size of the
   module-specific state). They are still recorded in the extensions
   dictionary, to avoid loading shared libraries twice.
*/

int
_PyImport_FixupExtensionObject(PyObject *mod, PyObject *name,
                               PyObject *filename)
{
    PyObject *modules, *dict, *key;
    struct PyModuleDef *def;
    int res;
    if (extensions == NULL) {
        extensions = PyDict_New();
        if (extensions == NULL)
            return -1;
    }
    if (mod == NULL || !PyModule_Check(mod)) {
        PyErr_BadInternalCall();
        return -1;
    }
    def = PyModule_GetDef(mod);
    if (!def) {
        PyErr_BadInternalCall();
        return -1;
    }
    modules = PyImport_GetModuleDict();
    if (PyDict_SetItem(modules, name, mod) < 0)
        return -1;
    if (_PyState_AddModule(mod, def) < 0) {
        PyDict_DelItem(modules, name);
        return -1;
    }
    if (def->m_size == -1) {
        if (def->m_base.m_copy) {
            /* Somebody already imported the module,
               likely under a different name.
               XXX this should really not happen. */
            Py_CLEAR(def->m_base.m_copy);
        }
        dict = PyModule_GetDict(mod);
        if (dict == NULL)
            return -1;
        def->m_base.m_copy = PyDict_Copy(dict);
        if (def->m_base.m_copy == NULL)
            return -1;
    }
    key = PyTuple_Pack(2, filename, name);
    if (key == NULL)
        return -1;
    res = PyDict_SetItem(extensions, key, (PyObject *)def);
    Py_DECREF(key);
    if (res < 0)
        return -1;
    return 0;
}

int
_PyImport_FixupBuiltin(PyObject *mod, const char *name)
{
    int res;
    PyObject *nameobj;
    nameobj = PyUnicode_InternFromString(name);
    if (nameobj == NULL)
        return -1;
    res = _PyImport_FixupExtensionObject(mod, nameobj, nameobj);
    Py_DECREF(nameobj);
    return res;
}

PyObject *
_PyImport_FindExtensionObject(PyObject *name, PyObject *filename)
{
    PyObject *mod, *mdict, *key;
    PyModuleDef* def;
    if (extensions == NULL)
        return NULL;
    key = PyTuple_Pack(2, filename, name);
    if (key == NULL)
        return NULL;
    def = (PyModuleDef *)PyDict_GetItem(extensions, key);
    Py_DECREF(key);
    if (def == NULL)
        return NULL;
    if (def->m_size == -1) {
        /* Module does not support repeated initialization */
        if (def->m_base.m_copy == NULL)
            return NULL;
        mod = PyImport_AddModuleObject(name);
        if (mod == NULL)
            return NULL;
        mdict = PyModule_GetDict(mod);
        if (mdict == NULL)
            return NULL;
        if (PyDict_Update(mdict, def->m_base.m_copy))
            return NULL;
    }
    else {
        if (def->m_base.m_init == NULL)
            return NULL;
        mod = def->m_base.m_init();
        if (mod == NULL)
            return NULL;
        if (PyDict_SetItem(PyImport_GetModuleDict(), name, mod) == -1) {
            Py_DECREF(mod);
            return NULL;
        }
        Py_DECREF(mod);
    }
    if (_PyState_AddModule(mod, def) < 0) {
        PyDict_DelItem(PyImport_GetModuleDict(), name);
        Py_DECREF(mod);
        return NULL;
    }
    if (Py_VerboseFlag)
        PySys_FormatStderr("import %U # previously loaded (%R)\n",
                          name, filename);
    return mod;

}

PyObject *
_PyImport_FindBuiltin(const char *name)
{
    PyObject *res, *nameobj;
    nameobj = PyUnicode_InternFromString(name);
    if (nameobj == NULL)
        return NULL;
    res = _PyImport_FindExtensionObject(nameobj, nameobj);
    Py_DECREF(nameobj);
    return res;
}

/* Get the module object corresponding to a module name.
   First check the modules dictionary if there's one there,
   if not, create a new one and insert it in the modules dictionary.
   Because the former action is most common, THIS DOES NOT RETURN A
   'NEW' REFERENCE! */

PyObject *
PyImport_AddModuleObject(PyObject *name)
{
    PyObject *modules = PyImport_GetModuleDict();
    PyObject *m;

    if ((m = PyDict_GetItemWithError(modules, name)) != NULL &&
        PyModule_Check(m)) {
        return m;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    m = PyModule_NewObject(name);
    if (m == NULL)
        return NULL;
    if (PyDict_SetItem(modules, name, m) != 0) {
        Py_DECREF(m);
        return NULL;
    }
    Py_DECREF(m); /* Yes, it still exists, in modules! */

    return m;
}

PyObject *
PyImport_AddModule(const char *name)
{
    PyObject *nameobj, *module;
    nameobj = PyUnicode_FromString(name);
    if (nameobj == NULL)
        return NULL;
    module = PyImport_AddModuleObject(nameobj);
    Py_DECREF(nameobj);
    return module;
}


/* Remove name from sys.modules, if it's there. */
static void
remove_module(PyObject *name)
{
    PyObject *modules = PyImport_GetModuleDict();
    if (PyDict_GetItem(modules, name) == NULL)
        return;
    if (PyDict_DelItem(modules, name) < 0)
        Py_FatalError("import:  deleting existing key in"
                      "sys.modules failed");
}


/* Execute a code object in a module and return the module object
 * WITH INCREMENTED REFERENCE COUNT.  If an error occurs, name is
 * removed from sys.modules, to avoid leaving damaged module objects
 * in sys.modules.  The caller may wish to restore the original
 * module object (if any) in this case; PyImport_ReloadModule is an
 * example.
 *
 * Note that PyImport_ExecCodeModuleWithPathnames() is the preferred, richer
 * interface.  The other two exist primarily for backward compatibility.
 */
PyObject *
PyImport_ExecCodeModule(const char *name, PyObject *co)
{
    return PyImport_ExecCodeModuleWithPathnames(
        name, co, (char *)NULL, (char *)NULL);
}

PyObject *
PyImport_ExecCodeModuleEx(const char *name, PyObject *co, const char *pathname)
{
    return PyImport_ExecCodeModuleWithPathnames(
        name, co, pathname, (char *)NULL);
}

PyObject *
PyImport_ExecCodeModuleWithPathnames(const char *name, PyObject *co,
                                     const char *pathname,
                                     const char *cpathname)
{
    PyObject *m = NULL;
    PyObject *nameobj, *pathobj = NULL, *cpathobj = NULL, *external= NULL;

    nameobj = PyUnicode_FromString(name);
    if (nameobj == NULL)
        return NULL;

    if (cpathname != NULL) {
        cpathobj = PyUnicode_DecodeFSDefault(cpathname);
        if (cpathobj == NULL)
            goto error;
    }
    else
        cpathobj = NULL;

    if (pathname != NULL) {
        pathobj = PyUnicode_DecodeFSDefault(pathname);
        if (pathobj == NULL)
            goto error;
    }
    else if (cpathobj != NULL) {
        PyInterpreterState *interp = PyThreadState_GET()->interp;
        _Py_IDENTIFIER(_get_sourcefile);

        if (interp == NULL) {
            Py_FatalError("PyImport_ExecCodeModuleWithPathnames: "
                          "no interpreter!");
        }

        external= PyObject_GetAttrString(interp->importlib,
                                         "_bootstrap_external");
        if (external != NULL) {
            pathobj = _PyObject_CallMethodIdObjArgs(external,
                                                    &PyId__get_sourcefile, cpathobj,
                                                    NULL);
            Py_DECREF(external);
        }
        if (pathobj == NULL)
            PyErr_Clear();
    }
    else
        pathobj = NULL;

    m = PyImport_ExecCodeModuleObject(nameobj, co, pathobj, cpathobj);
error:
    Py_DECREF(nameobj);
    Py_XDECREF(pathobj);
    Py_XDECREF(cpathobj);
    return m;
}

static PyObject *
module_dict_for_exec(PyObject *name)
{
    PyObject *m, *d = NULL;

    m = PyImport_AddModuleObject(name);
    if (m == NULL)
        return NULL;
    /* If the module is being reloaded, we get the old module back
       and re-use its dict to exec the new code. */
    d = PyModule_GetDict(m);
    if (PyDict_GetItemString(d, "__builtins__") == NULL) {
        if (PyDict_SetItemString(d, "__builtins__",
                                 PyEval_GetBuiltins()) != 0) {
            remove_module(name);
            return NULL;
        }
    }

    return d;  /* Return a borrowed reference. */
}

static PyObject *
exec_code_in_module(PyObject *name, PyObject *module_dict, PyObject *code_object)
{
    PyObject *modules = PyImport_GetModuleDict();
    PyObject *v, *m;

    v = PyEval_EvalCode(code_object, module_dict, module_dict);
    if (v == NULL) {
        remove_module(name);
        return NULL;
    }
    Py_DECREF(v);

    if ((m = PyDict_GetItem(modules, name)) == NULL) {
        PyErr_Format(PyExc_ImportError,
                     "Loaded module %R not found in sys.modules",
                     name);
        return NULL;
    }

    Py_INCREF(m);

    return m;
}

PyObject*
PyImport_ExecCodeModuleObject(PyObject *name, PyObject *co, PyObject *pathname,
                              PyObject *cpathname)
{
    PyObject *d, *external, *res;
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    _Py_IDENTIFIER(_fix_up_module);

    d = module_dict_for_exec(name);
    if (d == NULL) {
        return NULL;
    }

    if (pathname == NULL) {
        pathname = ((PyCodeObject *)co)->co_filename;
    }
    external = PyObject_GetAttrString(interp->importlib, "_bootstrap_external");
    if (external == NULL)
        return NULL;
    res = _PyObject_CallMethodIdObjArgs(external,
                                        &PyId__fix_up_module,
                                        d, name, pathname, cpathname, NULL);
    Py_DECREF(external);
    if (res != NULL) {
        Py_DECREF(res);
        res = exec_code_in_module(name, d, co);
    }
    return res;
}


static void
update_code_filenames(PyCodeObject *co, PyObject *oldname, PyObject *newname)
{
    PyObject *constants, *tmp;
    Py_ssize_t i, n;

    if (PyUnicode_Compare(co->co_filename, oldname))
        return;

    Py_INCREF(newname);
    Py_XSETREF(co->co_filename, newname);

    constants = co->co_consts;
    n = PyTuple_GET_SIZE(constants);
    for (i = 0; i < n; i++) {
        tmp = PyTuple_GET_ITEM(constants, i);
        if (PyCode_Check(tmp))
            update_code_filenames((PyCodeObject *)tmp,
                                  oldname, newname);
    }
}

static void
update_compiled_module(PyCodeObject *co, PyObject *newname)
{
    PyObject *oldname;

    if (PyUnicode_Compare(co->co_filename, newname) == 0)
        return;

    oldname = co->co_filename;
    Py_INCREF(oldname);
    update_code_filenames(co, oldname, newname);
    Py_DECREF(oldname);
}

/*[clinic input]
_imp._fix_co_filename

    code: object(type="PyCodeObject *", subclass_of="&PyCode_Type")
        Code object to change.

    path: unicode
        File path to use.
    /

Changes code.co_filename to specify the passed-in file path.
[clinic start generated code]*/

static PyObject *
_imp__fix_co_filename_impl(PyObject *module, PyCodeObject *code,
                           PyObject *path)
/*[clinic end generated code: output=1d002f100235587d input=895ba50e78b82f05]*/

{
    update_compiled_module(code, path);

    Py_RETURN_NONE;
}


/* Forward */
static const struct _frozen * find_frozen(PyObject *);


/* Helper to test for built-in module */

static int
is_builtin(PyObject *name)
{
    int i;
    for (i = 0; PyImport_Inittab[i].name != NULL; i++) {
        if (_PyUnicode_EqualToASCIIString(name, PyImport_Inittab[i].name)) {
            if (PyImport_Inittab[i].initfunc == NULL)
                return -1;
            else
                return 1;
        }
    }
    return 0;
}


/* Return a finder object for a sys.path/pkg.__path__ item 'p',
   possibly by fetching it from the path_importer_cache dict. If it
   wasn't yet cached, traverse path_hooks until a hook is found
   that can handle the path item. Return None if no hook could;
   this tells our caller that the path based finder could not find
   a finder for this path item. Cache the result in
   path_importer_cache.
   Returns a borrowed reference. */

static PyObject *
get_path_importer(PyObject *path_importer_cache, PyObject *path_hooks,
                  PyObject *p)
{
    PyObject *importer;
    Py_ssize_t j, nhooks;

    /* These conditions are the caller's responsibility: */
    assert(PyList_Check(path_hooks));
    assert(PyDict_Check(path_importer_cache));

    nhooks = PyList_Size(path_hooks);
    if (nhooks < 0)
        return NULL; /* Shouldn't happen */

    importer = PyDict_GetItem(path_importer_cache, p);
    if (importer != NULL)
        return importer;

    /* set path_importer_cache[p] to None to avoid recursion */
    if (PyDict_SetItem(path_importer_cache, p, Py_None) != 0)
        return NULL;

    for (j = 0; j < nhooks; j++) {
        PyObject *hook = PyList_GetItem(path_hooks, j);
        if (hook == NULL)
            return NULL;
        importer = PyObject_CallFunctionObjArgs(hook, p, NULL);
        if (importer != NULL)
            break;

        if (!PyErr_ExceptionMatches(PyExc_ImportError)) {
            return NULL;
        }
        PyErr_Clear();
    }
    if (importer == NULL) {
        return Py_None;
    }
    if (importer != NULL) {
        int err = PyDict_SetItem(path_importer_cache, p, importer);
        Py_DECREF(importer);
        if (err != 0)
            return NULL;
    }
    return importer;
}

PyAPI_FUNC(PyObject *)
PyImport_GetImporter(PyObject *path) {
    PyObject *importer=NULL, *path_importer_cache=NULL, *path_hooks=NULL;

    path_importer_cache = PySys_GetObject("path_importer_cache");
    path_hooks = PySys_GetObject("path_hooks");
    if (path_importer_cache != NULL && path_hooks != NULL) {
        importer = get_path_importer(path_importer_cache,
                                     path_hooks, path);
    }
    Py_XINCREF(importer); /* get_path_importer returns a borrowed reference */
    return importer;
}

/*[clinic input]
_imp.create_builtin

    spec: object
    /

Create an extension module.
[clinic start generated code]*/

static PyObject *
_imp_create_builtin(PyObject *module, PyObject *spec)
/*[clinic end generated code: output=ace7ff22271e6f39 input=37f966f890384e47]*/
{
    struct _inittab *p;
    PyObject *name;
    char *namestr;
    PyObject *mod;

    name = PyObject_GetAttrString(spec, "name");
    if (name == NULL) {
        return NULL;
    }

    mod = _PyImport_FindExtensionObject(name, name);
    if (mod || PyErr_Occurred()) {
        Py_DECREF(name);
        Py_XINCREF(mod);
        return mod;
    }

    namestr = PyUnicode_AsUTF8(name);
    if (namestr == NULL) {
        Py_DECREF(name);
        return NULL;
    }

    for (p = PyImport_Inittab; p->name != NULL; p++) {
        PyModuleDef *def;
        if (_PyUnicode_EqualToASCIIString(name, p->name)) {
            if (p->initfunc == NULL) {
                /* Cannot re-init internal module ("sys" or "builtins") */
                mod = PyImport_AddModule(namestr);
                Py_DECREF(name);
                return mod;
            }
            mod = (*p->initfunc)();
            if (mod == NULL) {
                Py_DECREF(name);
                return NULL;
            }
            if (PyObject_TypeCheck(mod, &PyModuleDef_Type)) {
                Py_DECREF(name);
                return PyModule_FromDefAndSpec((PyModuleDef*)mod, spec);
            } else {
                /* Remember pointer to module init function. */
                def = PyModule_GetDef(mod);
                if (def == NULL) {
                    Py_DECREF(name);
                    return NULL;
                }
                def->m_base.m_init = p->initfunc;
                if (_PyImport_FixupExtensionObject(mod, name, name) < 0) {
                    Py_DECREF(name);
                    return NULL;
                }
                Py_DECREF(name);
                return mod;
            }
        }
    }
    Py_DECREF(name);
    Py_RETURN_NONE;
}


/* Frozen modules */

static const struct _frozen *
find_frozen(PyObject *name)
{
    const struct _frozen *p;

    if (name == NULL)
        return NULL;

    for (p = PyImport_FrozenModules; ; p++) {
        if (p->name == NULL)
            return NULL;
        if (_PyUnicode_EqualToASCIIString(name, p->name))
            break;
    }
    return p;
}

static PyObject *
get_frozen_object(PyObject *name)
{
    const struct _frozen *p = find_frozen(name);
    int size;

    if (p == NULL) {
        PyErr_Format(PyExc_ImportError,
                     "No such frozen object named %R",
                     name);
        return NULL;
    }
    if (p->code == NULL) {
        PyErr_Format(PyExc_ImportError,
                     "Excluded frozen object named %R",
                     name);
        return NULL;
    }
    size = p->size;
    if (size < 0)
        size = -size;
    return PyMarshal_ReadObjectFromString((const char *)p->code, size);
}

static PyObject *
is_frozen_package(PyObject *name)
{
    const struct _frozen *p = find_frozen(name);
    int size;

    if (p == NULL) {
        PyErr_Format(PyExc_ImportError,
                     "No such frozen object named %R",
                     name);
        return NULL;
    }

    size = p->size;

    if (size < 0)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}


/* Initialize a frozen module.
   Return 1 for success, 0 if the module is not found, and -1 with
   an exception set if the initialization failed.
   This function is also used from frozenmain.c */

int
PyImport_ImportFrozenModuleObject(PyObject *name)
{
    const struct _frozen *p;
    PyObject *co, *m, *d;
    int ispackage;
    int size;

    p = find_frozen(name);

    if (p == NULL)
        return 0;
    if (p->code == NULL) {
        PyErr_Format(PyExc_ImportError,
                     "Excluded frozen object named %R",
                     name);
        return -1;
    }
    size = p->size;
    ispackage = (size < 0);
    if (ispackage)
        size = -size;
    co = PyMarshal_ReadObjectFromString((const char *)p->code, size);
    if (co == NULL)
        return -1;
    if (!PyCode_Check(co)) {
        PyErr_Format(PyExc_TypeError,
                     "frozen object %R is not a code object",
                     name);
        goto err_return;
    }
    if (ispackage) {
        /* Set __path__ to the empty list */
        PyObject *l;
        int err;
        m = PyImport_AddModuleObject(name);
        if (m == NULL)
            goto err_return;
        d = PyModule_GetDict(m);
        l = PyList_New(0);
        if (l == NULL) {
            goto err_return;
        }
        err = PyDict_SetItemString(d, "__path__", l);
        Py_DECREF(l);
        if (err != 0)
            goto err_return;
    }
    d = module_dict_for_exec(name);
    if (d == NULL) {
        goto err_return;
    }
    m = exec_code_in_module(name, d, co);
    if (m == NULL)
        goto err_return;
    Py_DECREF(co);
    Py_DECREF(m);
    return 1;
err_return:
    Py_DECREF(co);
    return -1;
}

int
PyImport_ImportFrozenModule(const char *name)
{
    PyObject *nameobj;
    int ret;
    nameobj = PyUnicode_InternFromString(name);
    if (nameobj == NULL)
        return -1;
    ret = PyImport_ImportFrozenModuleObject(nameobj);
    Py_DECREF(nameobj);
    return ret;
}


/* Import a module, either built-in, frozen, or external, and return
   its module object WITH INCREMENTED REFERENCE COUNT */

PyObject *
PyImport_ImportModule(const char *name)
{
    PyObject *pname;
    PyObject *result;

    pname = PyUnicode_FromString(name);
    if (pname == NULL)
        return NULL;
    result = PyImport_Import(pname);
    Py_DECREF(pname);
    return result;
}

/* Import a module without blocking
 *
 * At first it tries to fetch the module from sys.modules. If the module was
 * never loaded before it loads it with PyImport_ImportModule() unless another
 * thread holds the import lock. In the latter case the function raises an
 * ImportError instead of blocking.
 *
 * Returns the module object with incremented ref count.
 */
PyObject *
PyImport_ImportModuleNoBlock(const char *name)
{
    return PyImport_ImportModule(name);
}


/* Remove importlib frames from the traceback,
 * except in Verbose mode. */
static void
remove_importlib_frames(void)
{
    const char *importlib_filename = "<frozen importlib._bootstrap>";
    const char *external_filename = "<frozen importlib._bootstrap_external>";
    const char *remove_frames = "_call_with_frames_removed";
    int always_trim = 0;
    int in_importlib = 0;
    PyObject *exception, *value, *base_tb, *tb;
    PyObject **prev_link, **outer_link = NULL;

    /* Synopsis: if it's an ImportError, we trim all importlib chunks
       from the traceback. We always trim chunks
       which end with a call to "_call_with_frames_removed". */

    PyErr_Fetch(&exception, &value, &base_tb);
    if (!exception || Py_VerboseFlag)
        goto done;
    if (PyType_IsSubtype((PyTypeObject *) exception,
                         (PyTypeObject *) PyExc_ImportError))
        always_trim = 1;

    prev_link = &base_tb;
    tb = base_tb;
    while (tb != NULL) {
        PyTracebackObject *traceback = (PyTracebackObject *)tb;
        PyObject *next = (PyObject *) traceback->tb_next;
        PyFrameObject *frame = traceback->tb_frame;
        PyCodeObject *code = frame->f_code;
        int now_in_importlib;

        assert(PyTraceBack_Check(tb));
        now_in_importlib = _PyUnicode_EqualToASCIIString(code->co_filename, importlib_filename) ||
                           _PyUnicode_EqualToASCIIString(code->co_filename, external_filename);
        if (now_in_importlib && !in_importlib) {
            /* This is the link to this chunk of importlib tracebacks */
            outer_link = prev_link;
        }
        in_importlib = now_in_importlib;

        if (in_importlib &&
            (always_trim ||
             _PyUnicode_EqualToASCIIString(code->co_name, remove_frames))) {
            Py_XINCREF(next);
            Py_XSETREF(*outer_link, next);
            prev_link = outer_link;
        }
        else {
            prev_link = (PyObject **) &traceback->tb_next;
        }
        tb = next;
    }
done:
    PyErr_Restore(exception, value, base_tb);
}


static PyObject *
resolve_name(PyObject *name, PyObject *globals, int level)
{
    _Py_IDENTIFIER(__spec__);
    _Py_IDENTIFIER(__package__);
    _Py_IDENTIFIER(__path__);
    _Py_IDENTIFIER(__name__);
    _Py_IDENTIFIER(parent);
    PyObject *abs_name;
    PyObject *package = NULL;
    PyObject *spec;
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    Py_ssize_t last_dot;
    PyObject *base;
    int level_up;

    if (globals == NULL) {
        PyErr_SetString(PyExc_KeyError, "'__name__' not in globals");
        goto error;
    }
    if (!PyDict_Check(globals)) {
        PyErr_SetString(PyExc_TypeError, "globals must be a dict");
        goto error;
    }
    package = _PyDict_GetItemId(globals, &PyId___package__);
    if (package == Py_None) {
        package = NULL;
    }
    spec = _PyDict_GetItemId(globals, &PyId___spec__);

    if (package != NULL) {
        Py_INCREF(package);
        if (!PyUnicode_Check(package)) {
            PyErr_SetString(PyExc_TypeError, "package must be a string");
            goto error;
        }
        else if (spec != NULL && spec != Py_None) {
            int equal;
            PyObject *parent = _PyObject_GetAttrId(spec, &PyId_parent);
            if (parent == NULL) {
                goto error;
            }

            equal = PyObject_RichCompareBool(package, parent, Py_EQ);
            Py_DECREF(parent);
            if (equal < 0) {
                goto error;
            }
            else if (equal == 0) {
                if (PyErr_WarnEx(PyExc_ImportWarning,
                        "__package__ != __spec__.parent", 1) < 0) {
                    goto error;
                }
            }
        }
    }
    else if (spec != NULL && spec != Py_None) {
        package = _PyObject_GetAttrId(spec, &PyId_parent);
        if (package == NULL) {
            goto error;
        }
        else if (!PyUnicode_Check(package)) {
            PyErr_SetString(PyExc_TypeError,
                    "__spec__.parent must be a string");
            goto error;
        }
    }
    else {
        if (PyErr_WarnEx(PyExc_ImportWarning,
                    "can't resolve package from __spec__ or __package__, "
                    "falling back on __name__ and __path__", 1) < 0) {
            goto error;
        }

        package = _PyDict_GetItemId(globals, &PyId___name__);
        if (package == NULL) {
            PyErr_SetString(PyExc_KeyError, "'__name__' not in globals");
            goto error;
        }

        Py_INCREF(package);
        if (!PyUnicode_Check(package)) {
            PyErr_SetString(PyExc_TypeError, "__name__ must be a string");
            goto error;
        }

        if (_PyDict_GetItemId(globals, &PyId___path__) == NULL) {
            Py_ssize_t dot;

            if (PyUnicode_READY(package) < 0) {
                goto error;
            }

            dot = PyUnicode_FindChar(package, '.',
                                        0, PyUnicode_GET_LENGTH(package), -1);
            if (dot == -2) {
                goto error;
            }

            if (dot >= 0) {
                PyObject *substr = PyUnicode_Substring(package, 0, dot);
                if (substr == NULL) {
                    goto error;
                }
                Py_SETREF(package, substr);
            }
        }
    }

    last_dot = PyUnicode_GET_LENGTH(package);
    if (last_dot == 0) {
        PyErr_SetString(PyExc_ImportError,
                "attempted relative import with no known parent package");
        goto error;
    }
    else if (PyDict_GetItem(interp->modules, package) == NULL) {
        PyErr_Format(PyExc_SystemError,
                "Parent module %R not loaded, cannot perform relative "
                "import", package);
        goto error;
    }

    for (level_up = 1; level_up < level; level_up += 1) {
        last_dot = PyUnicode_FindChar(package, '.', 0, last_dot, -1);
        if (last_dot == -2) {
            goto error;
        }
        else if (last_dot == -1) {
            PyErr_SetString(PyExc_ValueError,
                            "attempted relative import beyond top-level "
                            "package");
            goto error;
        }
    }

    base = PyUnicode_Substring(package, 0, last_dot);
    Py_DECREF(package);
    if (base == NULL || PyUnicode_GET_LENGTH(name) == 0) {
        return base;
    }

    abs_name = PyUnicode_FromFormat("%U.%U", base, name);
    Py_DECREF(base);
    return abs_name;

  error:
    Py_XDECREF(package);
    return NULL;
}

PyObject *
PyImport_ImportModuleLevelObject(PyObject *name, PyObject *globals,
                                 PyObject *locals, PyObject *fromlist,
                                 int level)
{
    _Py_IDENTIFIER(_find_and_load);
    _Py_IDENTIFIER(_handle_fromlist);
    PyObject *abs_name = NULL;
    PyObject *final_mod = NULL;
    PyObject *mod = NULL;
    PyObject *package = NULL;
    PyInterpreterState *interp = PyThreadState_GET()->interp;
    int has_from;

    if (name == NULL) {
        PyErr_SetString(PyExc_ValueError, "Empty module name");
        goto error;
    }

    /* The below code is importlib.__import__() & _gcd_import(), ported to C
       for added performance. */

    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError, "module name must be a string");
        goto error;
    }
    if (PyUnicode_READY(name) < 0) {
        goto error;
    }
    if (level < 0) {
        PyErr_SetString(PyExc_ValueError, "level must be >= 0");
        goto error;
    }

    if (level > 0) {
        abs_name = resolve_name(name, globals, level);
        if (abs_name == NULL)
            goto error;
    }
    else {  /* level == 0 */
        if (PyUnicode_GET_LENGTH(name) == 0) {
            PyErr_SetString(PyExc_ValueError, "Empty module name");
            goto error;
        }
        abs_name = name;
        Py_INCREF(abs_name);
    }

    mod = PyDict_GetItem(interp->modules, abs_name);
    if (mod == Py_None) {
        PyObject *msg = PyUnicode_FromFormat("import of %R halted; "
                                             "None in sys.modules", abs_name);
        if (msg != NULL) {
            PyErr_SetImportErrorSubclass(PyExc_ModuleNotFoundError, msg,
                    abs_name, NULL);
            Py_DECREF(msg);
        }
        mod = NULL;
        goto error;
    }
    else if (mod != NULL) {
        _Py_IDENTIFIER(__spec__);
        _Py_IDENTIFIER(_initializing);
        _Py_IDENTIFIER(_lock_unlock_module);
        PyObject *value = NULL;
        PyObject *spec;
        int initializing = 0;

        Py_INCREF(mod);
        /* Optimization: only call _bootstrap._lock_unlock_module() if
           __spec__._initializing is true.
           NOTE: because of this, initializing must be set *before*
           stuffing the new module in sys.modules.
         */
        spec = _PyObject_GetAttrId(mod, &PyId___spec__);
        if (spec != NULL) {
            value = _PyObject_GetAttrId(spec, &PyId__initializing);
            Py_DECREF(spec);
        }
        if (value == NULL)
            PyErr_Clear();
        else {
            initializing = PyObject_IsTrue(value);
            Py_DECREF(value);
            if (initializing == -1)
                PyErr_Clear();
            if (initializing > 0) {
#ifdef WITH_THREAD
                _PyImport_AcquireLock();
#endif
                /* _bootstrap._lock_unlock_module() releases the import lock */
                value = _PyObject_CallMethodIdObjArgs(interp->importlib,
                                                &PyId__lock_unlock_module, abs_name,
                                                NULL);
                if (value == NULL)
                    goto error;
                Py_DECREF(value);
            }
        }
    }
    else {
#ifdef WITH_THREAD
        _PyImport_AcquireLock();
#endif
        /* _bootstrap._find_and_load() releases the import lock */
        mod = _PyObject_CallMethodIdObjArgs(interp->importlib,
                                            &PyId__find_and_load, abs_name,
                                            interp->import_func, NULL);
        if (mod == NULL) {
            goto error;
        }
    }

    has_from = 0;
    if (fromlist != NULL && fromlist != Py_None) {
        has_from = PyObject_IsTrue(fromlist);
        if (has_from < 0)
            goto error;
    }
    if (!has_from) {
        Py_ssize_t len = PyUnicode_GET_LENGTH(name);
        if (level == 0 || len > 0) {
            Py_ssize_t dot;

            dot = PyUnicode_FindChar(name, '.', 0, len, 1);
            if (dot == -2) {
                goto error;
            }

            if (dot == -1) {
                /* No dot in module name, simple exit */
                final_mod = mod;
                Py_INCREF(mod);
                goto error;
            }

            if (level == 0) {
                PyObject *front = PyUnicode_Substring(name, 0, dot);
                if (front == NULL) {
                    goto error;
                }

                final_mod = PyImport_ImportModuleLevelObject(front, NULL, NULL, NULL, 0);
                Py_DECREF(front);
            }
            else {
                Py_ssize_t cut_off = len - dot;
                Py_ssize_t abs_name_len = PyUnicode_GET_LENGTH(abs_name);
                PyObject *to_return = PyUnicode_Substring(abs_name, 0,
                                                        abs_name_len - cut_off);
                if (to_return == NULL) {
                    goto error;
                }

                final_mod = PyDict_GetItem(interp->modules, to_return);
                Py_DECREF(to_return);
                if (final_mod == NULL) {
                    PyErr_Format(PyExc_KeyError,
                                 "%R not in sys.modules as expected",
                                 to_return);
                    goto error;
                }
                Py_INCREF(final_mod);
            }
        }
        else {
            final_mod = mod;
            Py_INCREF(mod);
        }
    }
    else {
        final_mod = _PyObject_CallMethodIdObjArgs(interp->importlib,
                                                  &PyId__handle_fromlist, mod,
                                                  fromlist, interp->import_func,
                                                  NULL);
    }

  error:
    Py_XDECREF(abs_name);
    Py_XDECREF(mod);
    Py_XDECREF(package);
    if (final_mod == NULL)
        remove_importlib_frames();
    return final_mod;
}

PyObject *
PyImport_ImportModuleLevel(const char *name, PyObject *globals, PyObject *locals,
                           PyObject *fromlist, int level)
{
    PyObject *nameobj, *mod;
    nameobj = PyUnicode_FromString(name);
    if (nameobj == NULL)
        return NULL;
    mod = PyImport_ImportModuleLevelObject(nameobj, globals, locals,
                                           fromlist, level);
    Py_DECREF(nameobj);
    return mod;
}


/* Re-import a module of any kind and return its module object, WITH
   INCREMENTED REFERENCE COUNT */

PyObject *
PyImport_ReloadModule(PyObject *m)
{
    _Py_IDENTIFIER(reload);
    PyObject *reloaded_module = NULL;
    PyObject *modules = PyImport_GetModuleDict();
    PyObject *imp = PyDict_GetItemString(modules, "imp");
    if (imp == NULL) {
        imp = PyImport_ImportModule("imp");
        if (imp == NULL) {
            return NULL;
        }
    }
    else {
        Py_INCREF(imp);
    }

    reloaded_module = _PyObject_CallMethodId(imp, &PyId_reload, "O", m);
    Py_DECREF(imp);
    return reloaded_module;
}


/* Higher-level import emulator which emulates the "import" statement
   more accurately -- it invokes the __import__() function from the
   builtins of the current globals.  This means that the import is
   done using whatever import hooks are installed in the current
   environment.
   A dummy list ["__doc__"] is passed as the 4th argument so that
   e.g. PyImport_Import(PyUnicode_FromString("win32com.client.gencache"))
   will return <module "gencache"> instead of <module "win32com">. */

PyObject *
PyImport_Import(PyObject *module_name)
{
    static PyObject *silly_list = NULL;
    static PyObject *builtins_str = NULL;
    static PyObject *import_str = NULL;
    PyObject *globals = NULL;
    PyObject *import = NULL;
    PyObject *builtins = NULL;
    PyObject *modules = NULL;
    PyObject *r = NULL;

    /* Initialize constant string objects */
    if (silly_list == NULL) {
        import_str = PyUnicode_InternFromString("__import__");
        if (import_str == NULL)
            return NULL;
        builtins_str = PyUnicode_InternFromString("__builtins__");
        if (builtins_str == NULL)
            return NULL;
        silly_list = PyList_New(0);
        if (silly_list == NULL)
            return NULL;
    }

    /* Get the builtins from current globals */
    globals = PyEval_GetGlobals();
    if (globals != NULL) {
        Py_INCREF(globals);
        builtins = PyObject_GetItem(globals, builtins_str);
        if (builtins == NULL)
            goto err;
    }
    else {
        /* No globals -- use standard builtins, and fake globals */
        builtins = PyImport_ImportModuleLevel("builtins",
                                              NULL, NULL, NULL, 0);
        if (builtins == NULL)
            return NULL;
        globals = Py_BuildValue("{OO}", builtins_str, builtins);
        if (globals == NULL)
            goto err;
    }

    /* Get the __import__ function from the builtins */
    if (PyDict_Check(builtins)) {
        import = PyObject_GetItem(builtins, import_str);
        if (import == NULL)
            PyErr_SetObject(PyExc_KeyError, import_str);
    }
    else
        import = PyObject_GetAttr(builtins, import_str);
    if (import == NULL)
        goto err;

    /* Call the __import__ function with the proper argument list
       Always use absolute import here.
       Calling for side-effect of import. */
    r = PyObject_CallFunction(import, "OOOOi", module_name, globals,
                              globals, silly_list, 0, NULL);
    if (r == NULL)
        goto err;
    Py_DECREF(r);

    modules = PyImport_GetModuleDict();
    r = PyDict_GetItemWithError(modules, module_name);
    if (r != NULL) {
        Py_INCREF(r);
    }
    else if (!PyErr_Occurred()) {
        PyErr_SetObject(PyExc_KeyError, module_name);
    }

  err:
    Py_XDECREF(globals);
    Py_XDECREF(builtins);
    Py_XDECREF(import);

    return r;
}

/*[clinic input]
_imp.extension_suffixes

Returns the list of file suffixes used to identify extension modules.
[clinic start generated code]*/

static PyObject *
_imp_extension_suffixes_impl(PyObject *module)
/*[clinic end generated code: output=0bf346e25a8f0cd3 input=ecdeeecfcb6f839e]*/
{
    PyObject *list;
    const char *suffix;
    unsigned int index = 0;

    list = PyList_New(0);
    if (list == NULL)
        return NULL;
#ifdef HAVE_DYNAMIC_LOADING
    while ((suffix = _PyImport_DynLoadFiletab[index])) {
        PyObject *item = PyUnicode_FromString(suffix);
        if (item == NULL) {
            Py_DECREF(list);
            return NULL;
        }
        if (PyList_Append(list, item) < 0) {
            Py_DECREF(list);
            Py_DECREF(item);
            return NULL;
        }
        Py_DECREF(item);
        index += 1;
    }
#endif
    return list;
}

/*[clinic input]
_imp.init_frozen

    name: unicode
    /

Initializes a frozen module.
[clinic start generated code]*/

static PyObject *
_imp_init_frozen_impl(PyObject *module, PyObject *name)
/*[clinic end generated code: output=fc0511ed869fd69c input=13019adfc04f3fb3]*/
{
    int ret;
    PyObject *m;

    ret = PyImport_ImportFrozenModuleObject(name);
    if (ret < 0)
        return NULL;
    if (ret == 0) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    m = PyImport_AddModuleObject(name);
    Py_XINCREF(m);
    return m;
}

/*[clinic input]
_imp.get_frozen_object

    name: unicode
    /

Create a code object for a frozen module.
[clinic start generated code]*/

static PyObject *
_imp_get_frozen_object_impl(PyObject *module, PyObject *name)
/*[clinic end generated code: output=2568cc5b7aa0da63 input=ed689bc05358fdbd]*/
{
    return get_frozen_object(name);
}

/*[clinic input]
_imp.is_frozen_package

    name: unicode
    /

Returns True if the module name is of a frozen package.
[clinic start generated code]*/

static PyObject *
_imp_is_frozen_package_impl(PyObject *module, PyObject *name)
/*[clinic end generated code: output=e70cbdb45784a1c9 input=81b6cdecd080fbb8]*/
{
    return is_frozen_package(name);
}

/*[clinic input]
_imp.is_builtin

    name: unicode
    /

Returns True if the module name corresponds to a built-in module.
[clinic start generated code]*/

static PyObject *
_imp_is_builtin_impl(PyObject *module, PyObject *name)
/*[clinic end generated code: output=3bfd1162e2d3be82 input=86befdac021dd1c7]*/
{
    return PyLong_FromLong(is_builtin(name));
}

/*[clinic input]
_imp.is_frozen

    name: unicode
    /

Returns True if the module name corresponds to a frozen module.
[clinic start generated code]*/

static PyObject *
_imp_is_frozen_impl(PyObject *module, PyObject *name)
/*[clinic end generated code: output=01f408f5ec0f2577 input=7301dbca1897d66b]*/
{
    const struct _frozen *p;

    p = find_frozen(name);
    return PyBool_FromLong((long) (p == NULL ? 0 : p->size));
}

/* Common implementation for _imp.exec_dynamic and _imp.exec_builtin */
static int
exec_builtin_or_dynamic(PyObject *mod) {
    PyModuleDef *def;
    void *state;

    if (!PyModule_Check(mod)) {
        return 0;
    }

    def = PyModule_GetDef(mod);
    if (def == NULL) {
        return 0;
    }

    state = PyModule_GetState(mod);
    if (state) {
        /* Already initialized; skip reload */
        return 0;
    }

    return PyModule_ExecDef(mod, def);
}

#ifdef HAVE_DYNAMIC_LOADING

/*[clinic input]
_imp.create_dynamic

    spec: object
    file: object = NULL
    /

Create an extension module.
[clinic start generated code]*/

static PyObject *
_imp_create_dynamic_impl(PyObject *module, PyObject *spec, PyObject *file)
/*[clinic end generated code: output=83249b827a4fde77 input=c31b954f4cf4e09d]*/
{
    PyObject *mod, *name, *path;
    FILE *fp;

    name = PyObject_GetAttrString(spec, "name");
    if (name == NULL) {
        return NULL;
    }

    path = PyObject_GetAttrString(spec, "origin");
    if (path == NULL) {
        Py_DECREF(name);
        return NULL;
    }

    mod = _PyImport_FindExtensionObject(name, path);
    if (mod != NULL) {
        Py_DECREF(name);
        Py_DECREF(path);
        Py_INCREF(mod);
        return mod;
    }

    if (file != NULL) {
        fp = _Py_fopen_obj(path, "r");
        if (fp == NULL) {
            Py_DECREF(name);
            Py_DECREF(path);
            return NULL;
        }
    }
    else
        fp = NULL;

    mod = _PyImport_LoadDynamicModuleWithSpec(spec, fp);

    Py_DECREF(name);
    Py_DECREF(path);
    if (fp)
        fclose(fp);
    return mod;
}

/*[clinic input]
_imp.exec_dynamic -> int

    mod: object
    /

Initialize an extension module.
[clinic start generated code]*/

static int
_imp_exec_dynamic_impl(PyObject *module, PyObject *mod)
/*[clinic end generated code: output=f5720ac7b465877d input=9fdbfcb250280d3a]*/
{
    return exec_builtin_or_dynamic(mod);
}


#endif /* HAVE_DYNAMIC_LOADING */

/*[clinic input]
_imp.exec_builtin -> int

    mod: object
    /

Initialize a built-in module.
[clinic start generated code]*/

static int
_imp_exec_builtin_impl(PyObject *module, PyObject *mod)
/*[clinic end generated code: output=0262447b240c038e input=7beed5a2f12a60ca]*/
{
    return exec_builtin_or_dynamic(mod);
}

/*[clinic input]
dump buffer
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=524ce2e021e4eba6]*/


PyDoc_STRVAR(doc_imp,
"(Extremely) low-level import machinery bits as used by importlib and imp.");

static PyMethodDef imp_methods[] = {
    _IMP_EXTENSION_SUFFIXES_METHODDEF
    _IMP_LOCK_HELD_METHODDEF
    _IMP_ACQUIRE_LOCK_METHODDEF
    _IMP_RELEASE_LOCK_METHODDEF
    _IMP_GET_FROZEN_OBJECT_METHODDEF
    _IMP_IS_FROZEN_PACKAGE_METHODDEF
    _IMP_CREATE_BUILTIN_METHODDEF
    _IMP_INIT_FROZEN_METHODDEF
    _IMP_IS_BUILTIN_METHODDEF
    _IMP_IS_FROZEN_METHODDEF
    _IMP_CREATE_DYNAMIC_METHODDEF
    _IMP_EXEC_DYNAMIC_METHODDEF
    _IMP_EXEC_BUILTIN_METHODDEF
    _IMP__FIX_CO_FILENAME_METHODDEF
    {NULL, NULL}  /* sentinel */
};


static struct PyModuleDef impmodule = {
    PyModuleDef_HEAD_INIT,
    "_imp",
    doc_imp,
    0,
    imp_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_imp(void)
{
    PyObject *m, *d;

    m = PyModule_Create(&impmodule);
    if (m == NULL)
        goto failure;
    d = PyModule_GetDict(m);
    if (d == NULL)
        goto failure;

    return m;
  failure:
    Py_XDECREF(m);
    return NULL;
}


/* API for embedding applications that want to add their own entries
   to the table of built-in modules.  This should normally be called
   *before* Py_Initialize().  When the table resize fails, -1 is
   returned and the existing table is unchanged.

   After a similar function by Just van Rossum. */

int
PyImport_ExtendInittab(struct _inittab *newtab)
{
    static struct _inittab *our_copy = NULL;
    struct _inittab *p;
    int i, n;

    /* Count the number of entries in both tables */
    for (n = 0; newtab[n].name != NULL; n++)
        ;
    if (n == 0)
        return 0; /* Nothing to do */
    for (i = 0; PyImport_Inittab[i].name != NULL; i++)
        ;

    /* Allocate new memory for the combined table */
    p = our_copy;
    PyMem_RESIZE(p, struct _inittab, i+n+1);
    if (p == NULL)
        return -1;

    /* Copy the tables into the new memory */
    if (our_copy != PyImport_Inittab)
        memcpy(p, PyImport_Inittab, (i+1) * sizeof(struct _inittab));
    PyImport_Inittab = our_copy = p;
    memcpy(p+i, newtab, (n+1) * sizeof(struct _inittab));

    return 0;
}

/* Shorthand to add a single entry given a name and a function */

int
PyImport_AppendInittab(const char *name, PyObject* (*initfunc)(void))
{
    struct _inittab newtab[2];

    memset(newtab, '\0', sizeof newtab);

    newtab[0].name = name;
    newtab[0].initfunc = initfunc;

    return PyImport_ExtendInittab(newtab);
}

#ifdef __cplusplus
}
#endif
