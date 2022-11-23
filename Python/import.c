/* Module definition and import implementation */

#include "Python.h"

#include "pycore_import.h"        // _PyImport_BootstrapImp()
#include "pycore_initconfig.h"
#include "pycore_pyerrors.h"
#include "pycore_pyhash.h"
#include "pycore_pylifecycle.h"
#include "pycore_pymem.h"         // _PyMem_SetDefaultAllocator()
#include "pycore_interp.h"        // _PyInterpreterState_ClearModules()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_sysmodule.h"
#include "errcode.h"
#include "marshal.h"
#include "code.h"
#include "importdl.h"
#include "pydtrace.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define CACHEDIR "__pycache__"

/* Forward references */
static PyObject *import_add_module(PyThreadState *tstate, PyObject *name);

/* See _PyImport_FixupExtensionObject() below */
static PyObject *extensions = NULL;

/* This table is defined in config.c: */
extern struct _inittab _PyImport_Inittab[];

struct _inittab *PyImport_Inittab = _PyImport_Inittab;
static struct _inittab *inittab_copy = NULL;

_Py_IDENTIFIER(__path__);
_Py_IDENTIFIER(__spec__);

/*[clinic input]
module _imp
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=9c332475d8686284]*/

#include "clinic/import.c.h"

/* Initialize things */

PyStatus
_PyImportZip_Init(PyThreadState *tstate)
{
    PyObject *path_hooks, *zipimport;
    int err = 0;

    path_hooks = PySys_GetObject("path_hooks");
    if (path_hooks == NULL) {
        _PyErr_SetString(tstate, PyExc_RuntimeError,
                         "unable to get sys.path_hooks");
        goto error;
    }

    int verbose = _PyInterpreterState_GetConfig(tstate->interp)->verbose;
    if (verbose) {
        PySys_WriteStderr("# installing zipimport hook\n");
    }

    zipimport = PyImport_ImportModule("zipimport");
    if (zipimport == NULL) {
        _PyErr_Clear(tstate); /* No zip import module -- okay */
        if (verbose) {
            PySys_WriteStderr("# can't import zipimport\n");
        }
    }
    else {
        _Py_IDENTIFIER(zipimporter);
        PyObject *zipimporter = _PyObject_GetAttrId(zipimport,
                                                    &PyId_zipimporter);
        Py_DECREF(zipimport);
        if (zipimporter == NULL) {
            _PyErr_Clear(tstate); /* No zipimporter object -- okay */
            if (verbose) {
                PySys_WriteStderr("# can't import zipimport.zipimporter\n");
            }
        }
        else {
            /* sys.path_hooks.insert(0, zipimporter) */
            err = PyList_Insert(path_hooks, 0, zipimporter);
            Py_DECREF(zipimporter);
            if (err < 0) {
                goto error;
            }
            if (verbose) {
                PySys_WriteStderr("# installed zipimport hook\n");
            }
        }
    }

    return _PyStatus_OK();

  error:
    PyErr_Print();
    return _PyStatus_ERR("initializing zipimport failed");
}

/* Locking primitives to prevent parallel imports of the same module
   in different threads to return with a partially loaded module.
   These calls are serialized by the global interpreter lock. */

static PyThread_type_lock import_lock = NULL;
static unsigned long import_lock_thread = PYTHREAD_INVALID_THREAD_ID;
static int import_lock_level = 0;

void
_PyImport_AcquireLock(void)
{
    unsigned long me = PyThread_get_thread_ident();
    if (me == PYTHREAD_INVALID_THREAD_ID)
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
    if (import_lock_thread != PYTHREAD_INVALID_THREAD_ID ||
        !PyThread_acquire_lock(import_lock, 0))
    {
        PyThreadState *tstate = PyEval_SaveThread();
        PyThread_acquire_lock(import_lock, WAIT_LOCK);
        PyEval_RestoreThread(tstate);
    }
    assert(import_lock_level == 0);
    import_lock_thread = me;
    import_lock_level = 1;
}

int
_PyImport_ReleaseLock(void)
{
    unsigned long me = PyThread_get_thread_ident();
    if (me == PYTHREAD_INVALID_THREAD_ID || import_lock == NULL)
        return 0; /* Too bad */
    if (import_lock_thread != me)
        return -1;
    import_lock_level--;
    assert(import_lock_level >= 0);
    if (import_lock_level == 0) {
        import_lock_thread = PYTHREAD_INVALID_THREAD_ID;
        PyThread_release_lock(import_lock);
    }
    return 1;
}

#ifdef HAVE_FORK
/* This function is called from PyOS_AfterFork_Child() to ensure that newly
   created child processes do not share locks with the parent.
   We now acquire the import lock around fork() calls but on some platforms
   (Solaris 9 and earlier? see isue7242) that still left us with problems. */
PyStatus
_PyImport_ReInitLock(void)
{
    if (import_lock != NULL) {
        if (_PyThread_at_fork_reinit(&import_lock) < 0) {
            return _PyStatus_ERR("failed to create a new lock");
        }
    }

    if (import_lock_level > 1) {
        /* Forked as a side effect of import */
        unsigned long me = PyThread_get_thread_ident();
        PyThread_acquire_lock(import_lock, WAIT_LOCK);
        import_lock_thread = me;
        import_lock_level--;
    } else {
        import_lock_thread = PYTHREAD_INVALID_THREAD_ID;
        import_lock_level = 0;
    }
    return _PyStatus_OK();
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
    return PyBool_FromLong(import_lock_thread != PYTHREAD_INVALID_THREAD_ID);
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
    _PyImport_AcquireLock();
    Py_RETURN_NONE;
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
    if (_PyImport_ReleaseLock() < 0) {
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
        return NULL;
    }
    Py_RETURN_NONE;
}

void
_PyImport_Fini(void)
{
    Py_CLEAR(extensions);
    if (import_lock != NULL) {
        PyThread_free_lock(import_lock);
        import_lock = NULL;
    }
}

void
_PyImport_Fini2(void)
{
    /* Use the same memory allocator than PyImport_ExtendInittab(). */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    // Reset PyImport_Inittab
    PyImport_Inittab = _PyImport_Inittab;

    /* Free memory allocated by PyImport_ExtendInittab() */
    PyMem_RawFree(inittab_copy);
    inittab_copy = NULL;

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}

/* Helper for sys */

PyObject *
PyImport_GetModuleDict(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->modules == NULL) {
        Py_FatalError("interpreter has no modules dictionary");
    }
    return interp->modules;
}

/* In some corner cases it is important to be sure that the import
   machinery has been initialized (or not cleaned up yet).  For
   example, see issue #4236 and PyModule_Create2(). */

int
_PyImport_IsInitialized(PyInterpreterState *interp)
{
    if (interp->modules == NULL)
        return 0;
    return 1;
}

PyObject *
_PyImport_GetModuleId(struct _Py_Identifier *nameid)
{
    PyObject *name = _PyUnicode_FromId(nameid); /* borrowed */
    if (name == NULL) {
        return NULL;
    }
    return PyImport_GetModule(name);
}

int
_PyImport_SetModule(PyObject *name, PyObject *m)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *modules = interp->modules;
    return PyObject_SetItem(modules, name, m);
}

int
_PyImport_SetModuleString(const char *name, PyObject *m)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *modules = interp->modules;
    return PyMapping_SetItemString(modules, name, m);
}

static PyObject *
import_get_module(PyThreadState *tstate, PyObject *name)
{
    PyObject *modules = tstate->interp->modules;
    if (modules == NULL) {
        _PyErr_SetString(tstate, PyExc_RuntimeError,
                         "unable to get sys.modules");
        return NULL;
    }

    PyObject *m;
    Py_INCREF(modules);
    if (PyDict_CheckExact(modules)) {
        m = PyDict_GetItemWithError(modules, name);  /* borrowed */
        Py_XINCREF(m);
    }
    else {
        m = PyObject_GetItem(modules, name);
        if (m == NULL && _PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
            _PyErr_Clear(tstate);
        }
    }
    Py_DECREF(modules);
    return m;
}


static int
import_ensure_initialized(PyInterpreterState *interp, PyObject *mod, PyObject *name)
{
    PyObject *spec;

    _Py_IDENTIFIER(_lock_unlock_module);

    /* Optimization: only call _bootstrap._lock_unlock_module() if
       __spec__._initializing is true.
       NOTE: because of this, initializing must be set *before*
       stuffing the new module in sys.modules.
    */
    spec = _PyObject_GetAttrId(mod, &PyId___spec__);
    int busy = _PyModuleSpec_IsInitializing(spec);
    Py_XDECREF(spec);
    if (busy) {
        /* Wait until module is done importing. */
        PyObject *value = _PyObject_CallMethodIdOneArg(
            interp->importlib, &PyId__lock_unlock_module, name);
        if (value == NULL) {
            return -1;
        }
        Py_DECREF(value);
    }
    return 0;
}


/* Helper for pythonrun.c -- return magic number and tag. */

long
PyImport_GetMagicNumber(void)
{
    long res;
    PyInterpreterState *interp = _PyInterpreterState_GET();
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
   import_find_extension().

   Modules which do support multiple initialization set their m_size
   field to a non-negative number (indicating the size of the
   module-specific state). They are still recorded in the extensions
   dictionary, to avoid loading shared libraries twice.
*/

int
_PyImport_FixupExtensionObject(PyObject *mod, PyObject *name,
                               PyObject *filename, PyObject *modules)
{
    if (mod == NULL || !PyModule_Check(mod)) {
        PyErr_BadInternalCall();
        return -1;
    }

    struct PyModuleDef *def = PyModule_GetDef(mod);
    if (!def) {
        PyErr_BadInternalCall();
        return -1;
    }

    PyThreadState *tstate = _PyThreadState_GET();
    if (PyObject_SetItem(modules, name, mod) < 0) {
        return -1;
    }
    if (_PyState_AddModule(tstate, mod, def) < 0) {
        PyMapping_DelItem(modules, name);
        return -1;
    }

    // bpo-44050: Extensions and def->m_base.m_copy can be updated
    // when the extension module doesn't support sub-interpreters.
    if (_Py_IsMainInterpreter(tstate->interp) || def->m_size == -1) {
        if (def->m_size == -1) {
            if (def->m_base.m_copy) {
                /* Somebody already imported the module,
                   likely under a different name.
                   XXX this should really not happen. */
                Py_CLEAR(def->m_base.m_copy);
            }
            PyObject *dict = PyModule_GetDict(mod);
            if (dict == NULL) {
                return -1;
            }
            def->m_base.m_copy = PyDict_Copy(dict);
            if (def->m_base.m_copy == NULL) {
                return -1;
            }
        }

        if (extensions == NULL) {
            extensions = PyDict_New();
            if (extensions == NULL) {
                return -1;
            }
        }

        PyObject *key = PyTuple_Pack(2, filename, name);
        if (key == NULL) {
            return -1;
        }
        int res = PyDict_SetItem(extensions, key, (PyObject *)def);
        Py_DECREF(key);
        if (res < 0) {
            return -1;
        }
    }

    return 0;
}

int
_PyImport_FixupBuiltin(PyObject *mod, const char *name, PyObject *modules)
{
    int res;
    PyObject *nameobj;
    nameobj = PyUnicode_InternFromString(name);
    if (nameobj == NULL)
        return -1;
    res = _PyImport_FixupExtensionObject(mod, nameobj, nameobj, modules);
    Py_DECREF(nameobj);
    return res;
}

static PyObject *
import_find_extension(PyThreadState *tstate, PyObject *name,
                      PyObject *filename)
{
    if (extensions == NULL) {
        return NULL;
    }

    PyObject *key = PyTuple_Pack(2, filename, name);
    if (key == NULL) {
        return NULL;
    }
    PyModuleDef* def = (PyModuleDef *)PyDict_GetItemWithError(extensions, key);
    Py_DECREF(key);
    if (def == NULL) {
        return NULL;
    }

    PyObject *mod, *mdict;
    PyObject *modules = tstate->interp->modules;

    if (def->m_size == -1) {
        /* Module does not support repeated initialization */
        if (def->m_base.m_copy == NULL)
            return NULL;
        mod = import_add_module(tstate, name);
        if (mod == NULL)
            return NULL;
        mdict = PyModule_GetDict(mod);
        if (mdict == NULL) {
            Py_DECREF(mod);
            return NULL;
        }
        if (PyDict_Update(mdict, def->m_base.m_copy)) {
            Py_DECREF(mod);
            return NULL;
        }
    }
    else {
        if (def->m_base.m_init == NULL)
            return NULL;
        mod = def->m_base.m_init();
        if (mod == NULL)
            return NULL;
        if (PyObject_SetItem(modules, name, mod) == -1) {
            Py_DECREF(mod);
            return NULL;
        }
    }
    if (_PyState_AddModule(tstate, mod, def) < 0) {
        PyMapping_DelItem(modules, name);
        Py_DECREF(mod);
        return NULL;
    }

    int verbose = _PyInterpreterState_GetConfig(tstate->interp)->verbose;
    if (verbose) {
        PySys_FormatStderr("import %U # previously loaded (%R)\n",
                           name, filename);
    }
    return mod;
}

PyObject *
_PyImport_FindExtensionObject(PyObject *name, PyObject *filename)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *mod = import_find_extension(tstate, name, filename);
    if (mod) {
        PyObject *ref = PyWeakref_NewRef(mod, NULL);
        Py_DECREF(mod);
        if (ref == NULL) {
            return NULL;
        }
        mod = PyWeakref_GetObject(ref);
        Py_DECREF(ref);
    }
    return mod; /* borrowed reference */
}


/* Get the module object corresponding to a module name.
   First check the modules dictionary if there's one there,
   if not, create a new one and insert it in the modules dictionary. */

static PyObject *
import_add_module(PyThreadState *tstate, PyObject *name)
{
    PyObject *modules = tstate->interp->modules;
    if (modules == NULL) {
        _PyErr_SetString(tstate, PyExc_RuntimeError,
                         "no import module dictionary");
        return NULL;
    }

    PyObject *m;
    if (PyDict_CheckExact(modules)) {
        m = PyDict_GetItemWithError(modules, name);
        Py_XINCREF(m);
    }
    else {
        m = PyObject_GetItem(modules, name);
        // For backward-compatibility we copy the behavior
        // of PyDict_GetItemWithError().
        if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
            _PyErr_Clear(tstate);
        }
    }
    if (_PyErr_Occurred(tstate)) {
        return NULL;
    }
    if (m != NULL && PyModule_Check(m)) {
        return m;
    }
    Py_XDECREF(m);
    m = PyModule_NewObject(name);
    if (m == NULL)
        return NULL;
    if (PyObject_SetItem(modules, name, m) != 0) {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

PyObject *
PyImport_AddModuleObject(PyObject *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *mod = import_add_module(tstate, name);
    if (mod) {
        PyObject *ref = PyWeakref_NewRef(mod, NULL);
        Py_DECREF(mod);
        if (ref == NULL) {
            return NULL;
        }
        mod = PyWeakref_GetObject(ref);
        Py_DECREF(ref);
    }
    return mod; /* borrowed reference */
}


PyObject *
PyImport_AddModule(const char *name)
{
    PyObject *nameobj = PyUnicode_FromString(name);
    if (nameobj == NULL) {
        return NULL;
    }
    PyObject *module = PyImport_AddModuleObject(nameobj);
    Py_DECREF(nameobj);
    return module;
}


/* Remove name from sys.modules, if it's there.
 * Can be called with an exception raised.
 * If fail to remove name a new exception will be chained with the old
 * exception, otherwise the old exception is preserved.
 */
static void
remove_module(PyThreadState *tstate, PyObject *name)
{
    PyObject *type, *value, *traceback;
    _PyErr_Fetch(tstate, &type, &value, &traceback);

    PyObject *modules = tstate->interp->modules;
    if (PyDict_CheckExact(modules)) {
        PyObject *mod = _PyDict_Pop(modules, name, Py_None);
        Py_XDECREF(mod);
    }
    else if (PyMapping_DelItem(modules, name) < 0) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
            _PyErr_Clear(tstate);
        }
    }

    _PyErr_ChainExceptions(type, value, traceback);
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
        PyInterpreterState *interp = _PyInterpreterState_GET();
        _Py_IDENTIFIER(_get_sourcefile);

        if (interp == NULL) {
            Py_FatalError("no current interpreter");
        }

        external= PyObject_GetAttrString(interp->importlib,
                                         "_bootstrap_external");
        if (external != NULL) {
            pathobj = _PyObject_CallMethodIdOneArg(
                external, &PyId__get_sourcefile, cpathobj);
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
module_dict_for_exec(PyThreadState *tstate, PyObject *name)
{
    _Py_IDENTIFIER(__builtins__);
    PyObject *m, *d;

    m = import_add_module(tstate, name);
    if (m == NULL)
        return NULL;
    /* If the module is being reloaded, we get the old module back
       and re-use its dict to exec the new code. */
    d = PyModule_GetDict(m);
    int r = _PyDict_ContainsId(d, &PyId___builtins__);
    if (r == 0) {
        r = _PyDict_SetItemId(d, &PyId___builtins__,
                              PyEval_GetBuiltins());
    }
    if (r < 0) {
        remove_module(tstate, name);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(d);
    Py_DECREF(m);
    return d;
}

static PyObject *
exec_code_in_module(PyThreadState *tstate, PyObject *name,
                    PyObject *module_dict, PyObject *code_object)
{
    PyObject *v, *m;

    v = PyEval_EvalCode(code_object, module_dict, module_dict);
    if (v == NULL) {
        remove_module(tstate, name);
        return NULL;
    }
    Py_DECREF(v);

    m = import_get_module(tstate, name);
    if (m == NULL && !_PyErr_Occurred(tstate)) {
        _PyErr_Format(tstate, PyExc_ImportError,
                      "Loaded module %R not found in sys.modules",
                      name);
    }

    return m;
}

PyObject*
PyImport_ExecCodeModuleObject(PyObject *name, PyObject *co, PyObject *pathname,
                              PyObject *cpathname)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *d, *external, *res;
    _Py_IDENTIFIER(_fix_up_module);

    d = module_dict_for_exec(tstate, name);
    if (d == NULL) {
        return NULL;
    }

    if (pathname == NULL) {
        pathname = ((PyCodeObject *)co)->co_filename;
    }
    external = PyObject_GetAttrString(tstate->interp->importlib,
                                      "_bootstrap_external");
    if (external == NULL) {
        Py_DECREF(d);
        return NULL;
    }
    res = _PyObject_CallMethodIdObjArgs(external,
                                        &PyId__fix_up_module,
                                        d, name, pathname, cpathname, NULL);
    Py_DECREF(external);
    if (res != NULL) {
        Py_DECREF(res);
        res = exec_code_in_module(tstate, name, d, co);
    }
    Py_DECREF(d);
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
   path_importer_cache. */

static PyObject *
get_path_importer(PyThreadState *tstate, PyObject *path_importer_cache,
                  PyObject *path_hooks, PyObject *p)
{
    PyObject *importer;
    Py_ssize_t j, nhooks;

    /* These conditions are the caller's responsibility: */
    assert(PyList_Check(path_hooks));
    assert(PyDict_Check(path_importer_cache));

    nhooks = PyList_Size(path_hooks);
    if (nhooks < 0)
        return NULL; /* Shouldn't happen */

    importer = PyDict_GetItemWithError(path_importer_cache, p);
    if (importer != NULL || _PyErr_Occurred(tstate)) {
        Py_XINCREF(importer);
        return importer;
    }

    /* set path_importer_cache[p] to None to avoid recursion */
    if (PyDict_SetItem(path_importer_cache, p, Py_None) != 0)
        return NULL;

    for (j = 0; j < nhooks; j++) {
        PyObject *hook = PyList_GetItem(path_hooks, j);
        if (hook == NULL)
            return NULL;
        importer = PyObject_CallOneArg(hook, p);
        if (importer != NULL)
            break;

        if (!_PyErr_ExceptionMatches(tstate, PyExc_ImportError)) {
            return NULL;
        }
        _PyErr_Clear(tstate);
    }
    if (importer == NULL) {
        Py_RETURN_NONE;
    }
    if (PyDict_SetItem(path_importer_cache, p, importer) < 0) {
        Py_DECREF(importer);
        return NULL;
    }
    return importer;
}

PyObject *
PyImport_GetImporter(PyObject *path)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *path_importer_cache = PySys_GetObject("path_importer_cache");
    PyObject *path_hooks = PySys_GetObject("path_hooks");
    if (path_importer_cache == NULL || path_hooks == NULL) {
        return NULL;
    }
    return get_path_importer(tstate, path_importer_cache, path_hooks, path);
}

static PyObject*
create_builtin(PyThreadState *tstate, PyObject *name, PyObject *spec)
{
    PyObject *mod = import_find_extension(tstate, name, name);
    if (mod || _PyErr_Occurred(tstate)) {
        return mod;
    }

    PyObject *modules = tstate->interp->modules;
    for (struct _inittab *p = PyImport_Inittab; p->name != NULL; p++) {
        if (_PyUnicode_EqualToASCIIString(name, p->name)) {
            if (p->initfunc == NULL) {
                /* Cannot re-init internal module ("sys" or "builtins") */
                mod = PyImport_AddModuleObject(name);
                return Py_XNewRef(mod);
            }

            mod = (*p->initfunc)();
            if (mod == NULL) {
                return NULL;
            }

            if (PyObject_TypeCheck(mod, &PyModuleDef_Type)) {
                return PyModule_FromDefAndSpec((PyModuleDef*)mod, spec);
            }
            else {
                /* Remember pointer to module init function. */
                PyModuleDef *def = PyModule_GetDef(mod);
                if (def == NULL) {
                    return NULL;
                }

                def->m_base.m_init = p->initfunc;
                if (_PyImport_FixupExtensionObject(mod, name, name,
                                                   modules) < 0) {
                    return NULL;
                }
                return mod;
            }
        }
    }

    // not found
    Py_RETURN_NONE;
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
    PyThreadState *tstate = _PyThreadState_GET();

    PyObject *name = PyObject_GetAttrString(spec, "name");
    if (name == NULL) {
        return NULL;
    }

    PyObject *mod = create_builtin(tstate, name, spec);
    Py_DECREF(name);
    return mod;
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
    PyThreadState *tstate = _PyThreadState_GET();
    const struct _frozen *p;
    PyObject *co, *m, *d;
    int ispackage;
    int size;

    p = find_frozen(name);

    if (p == NULL)
        return 0;
    if (p->code == NULL) {
        _PyErr_Format(tstate, PyExc_ImportError,
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
        _PyErr_Format(tstate, PyExc_TypeError,
                      "frozen object %R is not a code object",
                      name);
        goto err_return;
    }
    if (ispackage) {
        /* Set __path__ to the empty list */
        PyObject *l;
        int err;
        m = import_add_module(tstate, name);
        if (m == NULL)
            goto err_return;
        d = PyModule_GetDict(m);
        l = PyList_New(0);
        if (l == NULL) {
            Py_DECREF(m);
            goto err_return;
        }
        err = PyDict_SetItemString(d, "__path__", l);
        Py_DECREF(l);
        Py_DECREF(m);
        if (err != 0)
            goto err_return;
    }
    d = module_dict_for_exec(tstate, name);
    if (d == NULL) {
        goto err_return;
    }
    m = exec_code_in_module(tstate, name, d, co);
    Py_DECREF(d);
    if (m == NULL) {
        goto err_return;
    }
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
remove_importlib_frames(PyThreadState *tstate)
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

    _PyErr_Fetch(tstate, &exception, &value, &base_tb);
    if (!exception || _PyInterpreterState_GetConfig(tstate->interp)->verbose) {
        goto done;
    }

    if (PyType_IsSubtype((PyTypeObject *) exception,
                         (PyTypeObject *) PyExc_ImportError))
        always_trim = 1;

    prev_link = &base_tb;
    tb = base_tb;
    while (tb != NULL) {
        PyTracebackObject *traceback = (PyTracebackObject *)tb;
        PyObject *next = (PyObject *) traceback->tb_next;
        PyFrameObject *frame = traceback->tb_frame;
        PyCodeObject *code = PyFrame_GetCode(frame);
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
        Py_DECREF(code);
        tb = next;
    }
done:
    _PyErr_Restore(tstate, exception, value, base_tb);
}


static PyObject *
resolve_name(PyThreadState *tstate, PyObject *name, PyObject *globals, int level)
{
    _Py_IDENTIFIER(__package__);
    _Py_IDENTIFIER(__name__);
    _Py_IDENTIFIER(parent);
    PyObject *abs_name;
    PyObject *package = NULL;
    PyObject *spec;
    Py_ssize_t last_dot;
    PyObject *base;
    int level_up;

    if (globals == NULL) {
        _PyErr_SetString(tstate, PyExc_KeyError, "'__name__' not in globals");
        goto error;
    }
    if (!PyDict_Check(globals)) {
        _PyErr_SetString(tstate, PyExc_TypeError, "globals must be a dict");
        goto error;
    }
    package = _PyDict_GetItemIdWithError(globals, &PyId___package__);
    if (package == Py_None) {
        package = NULL;
    }
    else if (package == NULL && _PyErr_Occurred(tstate)) {
        goto error;
    }
    spec = _PyDict_GetItemIdWithError(globals, &PyId___spec__);
    if (spec == NULL && _PyErr_Occurred(tstate)) {
        goto error;
    }

    if (package != NULL) {
        Py_INCREF(package);
        if (!PyUnicode_Check(package)) {
            _PyErr_SetString(tstate, PyExc_TypeError,
                             "package must be a string");
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
            _PyErr_SetString(tstate, PyExc_TypeError,
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

        package = _PyDict_GetItemIdWithError(globals, &PyId___name__);
        if (package == NULL) {
            if (!_PyErr_Occurred(tstate)) {
                _PyErr_SetString(tstate, PyExc_KeyError,
                                 "'__name__' not in globals");
            }
            goto error;
        }

        Py_INCREF(package);
        if (!PyUnicode_Check(package)) {
            _PyErr_SetString(tstate, PyExc_TypeError,
                             "__name__ must be a string");
            goto error;
        }

        int haspath = _PyDict_ContainsId(globals, &PyId___path__);
        if (haspath < 0) {
            goto error;
        }
        if (!haspath) {
            Py_ssize_t dot;

            if (PyUnicode_READY(package) < 0) {
                goto error;
            }

            dot = PyUnicode_FindChar(package, '.',
                                        0, PyUnicode_GET_LENGTH(package), -1);
            if (dot == -2) {
                goto error;
            }
            else if (dot == -1) {
                goto no_parent_error;
            }
            PyObject *substr = PyUnicode_Substring(package, 0, dot);
            if (substr == NULL) {
                goto error;
            }
            Py_SETREF(package, substr);
        }
    }

    last_dot = PyUnicode_GET_LENGTH(package);
    if (last_dot == 0) {
        goto no_parent_error;
    }

    for (level_up = 1; level_up < level; level_up += 1) {
        last_dot = PyUnicode_FindChar(package, '.', 0, last_dot, -1);
        if (last_dot == -2) {
            goto error;
        }
        else if (last_dot == -1) {
            _PyErr_SetString(tstate, PyExc_ImportError,
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

  no_parent_error:
    _PyErr_SetString(tstate, PyExc_ImportError,
                     "attempted relative import "
                     "with no known parent package");

  error:
    Py_XDECREF(package);
    return NULL;
}

static PyObject *
import_find_and_load(PyThreadState *tstate, PyObject *abs_name)
{
    _Py_IDENTIFIER(_find_and_load);
    PyObject *mod = NULL;
    PyInterpreterState *interp = tstate->interp;
    int import_time = _PyInterpreterState_GetConfig(interp)->import_time;
    static int import_level;
    static _PyTime_t accumulated;

    _PyTime_t t1 = 0, accumulated_copy = accumulated;

    PyObject *sys_path = PySys_GetObject("path");
    PyObject *sys_meta_path = PySys_GetObject("meta_path");
    PyObject *sys_path_hooks = PySys_GetObject("path_hooks");
    if (_PySys_Audit(tstate, "import", "OOOOO",
                     abs_name, Py_None, sys_path ? sys_path : Py_None,
                     sys_meta_path ? sys_meta_path : Py_None,
                     sys_path_hooks ? sys_path_hooks : Py_None) < 0) {
        return NULL;
    }


    /* XOptions is initialized after first some imports.
     * So we can't have negative cache before completed initialization.
     * Anyway, importlib._find_and_load is much slower than
     * _PyDict_GetItemIdWithError().
     */
    if (import_time) {
        static int header = 1;
        if (header) {
            fputs("import time: self [us] | cumulative | imported package\n",
                  stderr);
            header = 0;
        }

        import_level++;
        t1 = _PyTime_GetPerfCounter();
        accumulated = 0;
    }

    if (PyDTrace_IMPORT_FIND_LOAD_START_ENABLED())
        PyDTrace_IMPORT_FIND_LOAD_START(PyUnicode_AsUTF8(abs_name));

    mod = _PyObject_CallMethodIdObjArgs(interp->importlib,
                                        &PyId__find_and_load, abs_name,
                                        interp->import_func, NULL);

    if (PyDTrace_IMPORT_FIND_LOAD_DONE_ENABLED())
        PyDTrace_IMPORT_FIND_LOAD_DONE(PyUnicode_AsUTF8(abs_name),
                                       mod != NULL);

    if (import_time) {
        _PyTime_t cum = _PyTime_GetPerfCounter() - t1;

        import_level--;
        fprintf(stderr, "import time: %9ld | %10ld | %*s%s\n",
                (long)_PyTime_AsMicroseconds(cum - accumulated, _PyTime_ROUND_CEILING),
                (long)_PyTime_AsMicroseconds(cum, _PyTime_ROUND_CEILING),
                import_level*2, "", PyUnicode_AsUTF8(abs_name));

        accumulated = accumulated_copy + cum;
    }

    return mod;
}

PyObject *
PyImport_GetModule(PyObject *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *mod;

    mod = import_get_module(tstate, name);
    if (mod != NULL && mod != Py_None) {
        if (import_ensure_initialized(tstate->interp, mod, name) < 0) {
            Py_DECREF(mod);
            remove_importlib_frames(tstate);
            return NULL;
        }
    }
    return mod;
}

PyObject *
PyImport_ImportModuleLevelObject(PyObject *name, PyObject *globals,
                                 PyObject *locals, PyObject *fromlist,
                                 int level)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_IDENTIFIER(_handle_fromlist);
    PyObject *abs_name = NULL;
    PyObject *final_mod = NULL;
    PyObject *mod = NULL;
    PyObject *package = NULL;
    PyInterpreterState *interp = tstate->interp;
    int has_from;

    if (name == NULL) {
        _PyErr_SetString(tstate, PyExc_ValueError, "Empty module name");
        goto error;
    }

    /* The below code is importlib.__import__() & _gcd_import(), ported to C
       for added performance. */

    if (!PyUnicode_Check(name)) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "module name must be a string");
        goto error;
    }
    if (PyUnicode_READY(name) < 0) {
        goto error;
    }
    if (level < 0) {
        _PyErr_SetString(tstate, PyExc_ValueError, "level must be >= 0");
        goto error;
    }

    if (level > 0) {
        abs_name = resolve_name(tstate, name, globals, level);
        if (abs_name == NULL)
            goto error;
    }
    else {  /* level == 0 */
        if (PyUnicode_GET_LENGTH(name) == 0) {
            _PyErr_SetString(tstate, PyExc_ValueError, "Empty module name");
            goto error;
        }
        abs_name = name;
        Py_INCREF(abs_name);
    }

    mod = import_get_module(tstate, abs_name);
    if (mod == NULL && _PyErr_Occurred(tstate)) {
        goto error;
    }

    if (mod != NULL && mod != Py_None) {
        if (import_ensure_initialized(tstate->interp, mod, abs_name) < 0) {
            goto error;
        }
    }
    else {
        Py_XDECREF(mod);
        mod = import_find_and_load(tstate, abs_name);
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

                final_mod = import_get_module(tstate, to_return);
                Py_DECREF(to_return);
                if (final_mod == NULL) {
                    if (!_PyErr_Occurred(tstate)) {
                        _PyErr_Format(tstate, PyExc_KeyError,
                                      "%R not in sys.modules as expected",
                                      to_return);
                    }
                    goto error;
                }
            }
        }
        else {
            final_mod = mod;
            Py_INCREF(mod);
        }
    }
    else {
        PyObject *path;
        if (_PyObject_LookupAttrId(mod, &PyId___path__, &path) < 0) {
            goto error;
        }
        if (path) {
            Py_DECREF(path);
            final_mod = _PyObject_CallMethodIdObjArgs(
                        interp->importlib, &PyId__handle_fromlist,
                        mod, fromlist, interp->import_func, NULL);
        }
        else {
            final_mod = mod;
            Py_INCREF(mod);
        }
    }

  error:
    Py_XDECREF(abs_name);
    Py_XDECREF(mod);
    Py_XDECREF(package);
    if (final_mod == NULL) {
        remove_importlib_frames(tstate);
    }
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
    _Py_IDENTIFIER(importlib);
    _Py_IDENTIFIER(reload);
    PyObject *reloaded_module = NULL;
    PyObject *importlib = _PyImport_GetModuleId(&PyId_importlib);
    if (importlib == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }

        importlib = PyImport_ImportModule("importlib");
        if (importlib == NULL) {
            return NULL;
        }
    }

    reloaded_module = _PyObject_CallMethodIdOneArg(importlib, &PyId_reload, m);
    Py_DECREF(importlib);
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
    _Py_IDENTIFIER(__import__);
    _Py_IDENTIFIER(__builtins__);

    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *globals = NULL;
    PyObject *import = NULL;
    PyObject *builtins = NULL;
    PyObject *r = NULL;

    /* Initialize constant string objects */
    PyObject *import_str = _PyUnicode_FromId(&PyId___import__); // borrowed ref
    if (import_str == NULL) {
        return NULL;
    }

    PyObject *builtins_str = _PyUnicode_FromId(&PyId___builtins__); // borrowed ref
    if (builtins_str == NULL) {
        return NULL;
    }

    PyObject *from_list = PyList_New(0);
    if (from_list == NULL) {
        goto err;
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
        if (builtins == NULL) {
            goto err;
        }
        globals = Py_BuildValue("{OO}", builtins_str, builtins);
        if (globals == NULL)
            goto err;
    }

    /* Get the __import__ function from the builtins */
    if (PyDict_Check(builtins)) {
        import = PyObject_GetItem(builtins, import_str);
        if (import == NULL) {
            _PyErr_SetObject(tstate, PyExc_KeyError, import_str);
        }
    }
    else
        import = PyObject_GetAttr(builtins, import_str);
    if (import == NULL)
        goto err;

    /* Call the __import__ function with the proper argument list
       Always use absolute import here.
       Calling for side-effect of import. */
    r = PyObject_CallFunction(import, "OOOOi", module_name, globals,
                              globals, from_list, 0, NULL);
    if (r == NULL)
        goto err;
    Py_DECREF(r);

    r = import_get_module(tstate, module_name);
    if (r == NULL && !_PyErr_Occurred(tstate)) {
        _PyErr_SetObject(tstate, PyExc_KeyError, module_name);
    }

  err:
    Py_XDECREF(globals);
    Py_XDECREF(builtins);
    Py_XDECREF(import);
    Py_XDECREF(from_list);

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

    list = PyList_New(0);
    if (list == NULL)
        return NULL;
#ifdef HAVE_DYNAMIC_LOADING
    const char *suffix;
    unsigned int index = 0;

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
    PyThreadState *tstate = _PyThreadState_GET();
    int ret;

    ret = PyImport_ImportFrozenModuleObject(name);
    if (ret < 0)
        return NULL;
    if (ret == 0) {
        Py_RETURN_NONE;
    }
    return import_add_module(tstate, name);
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

    PyThreadState *tstate = _PyThreadState_GET();
    mod = import_find_extension(tstate, name, path);
    if (mod != NULL || PyErr_Occurred()) {
        Py_DECREF(name);
        Py_DECREF(path);
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
_imp.source_hash

    key: long
    source: Py_buffer
[clinic start generated code]*/

static PyObject *
_imp_source_hash_impl(PyObject *module, long key, Py_buffer *source)
/*[clinic end generated code: output=edb292448cf399ea input=9aaad1e590089789]*/
{
    union {
        uint64_t x;
        char data[sizeof(uint64_t)];
    } hash;
    hash.x = _Py_KeyedHash((uint64_t)key, source->buf, source->len);
#if !PY_LITTLE_ENDIAN
    // Force to little-endian. There really ought to be a succinct standard way
    // to do this.
    for (size_t i = 0; i < sizeof(hash.data)/2; i++) {
        char tmp = hash.data[i];
        hash.data[i] = hash.data[sizeof(hash.data) - i - 1];
        hash.data[sizeof(hash.data) - i - 1] = tmp;
    }
#endif
    return PyBytes_FromStringAndSize(hash.data, sizeof(hash.data));
}


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
    _IMP_SOURCE_HASH_METHODDEF
    {NULL, NULL}  /* sentinel */
};


static int
imp_module_exec(PyObject *module)
{
    const wchar_t *mode = _Py_GetConfig()->check_hash_pycs_mode;
    PyObject *pyc_mode = PyUnicode_FromWideChar(mode, -1);
    if (pyc_mode == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "check_hash_based_pycs", pyc_mode) < 0) {
        Py_DECREF(pyc_mode);
        return -1;
    }
    Py_DECREF(pyc_mode);

    return 0;
}


static PyModuleDef_Slot imp_slots[] = {
    {Py_mod_exec, imp_module_exec},
    {0, NULL}
};

static struct PyModuleDef imp_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_imp",
    .m_doc = doc_imp,
    .m_size = 0,
    .m_methods = imp_methods,
    .m_slots = imp_slots,
};

PyMODINIT_FUNC
PyInit__imp(void)
{
    return PyModuleDef_Init(&imp_module);
}


// Import the _imp extension by calling manually _imp.create_builtin() and
// _imp.exec_builtin() since importlib is not initialized yet. Initializing
// importlib requires the _imp module: this function fix the bootstrap issue.
PyObject*
_PyImport_BootstrapImp(PyThreadState *tstate)
{
    PyObject *name = PyUnicode_FromString("_imp");
    if (name == NULL) {
        return NULL;
    }

    // Mock a ModuleSpec object just good enough for PyModule_FromDefAndSpec():
    // an object with just a name attribute.
    //
    // _imp.__spec__ is overridden by importlib._bootstrap._instal() anyway.
    PyObject *attrs = Py_BuildValue("{sO}", "name", name);
    if (attrs == NULL) {
        goto error;
    }
    PyObject *spec = _PyNamespace_New(attrs);
    Py_DECREF(attrs);
    if (spec == NULL) {
        goto error;
    }

    // Create the _imp module from its definition.
    PyObject *mod = create_builtin(tstate, name, spec);
    Py_CLEAR(name);
    Py_DECREF(spec);
    if (mod == NULL) {
        goto error;
    }
    assert(mod != Py_None);  // not found

    // Execute the _imp module: call imp_module_exec().
    if (exec_builtin_or_dynamic(mod) < 0) {
        Py_DECREF(mod);
        goto error;
    }
    return mod;

error:
    Py_XDECREF(name);
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
    struct _inittab *p;
    size_t i, n;
    int res = 0;

    /* Count the number of entries in both tables */
    for (n = 0; newtab[n].name != NULL; n++)
        ;
    if (n == 0)
        return 0; /* Nothing to do */
    for (i = 0; PyImport_Inittab[i].name != NULL; i++)
        ;

    /* Force default raw memory allocator to get a known allocator to be able
       to release the memory in _PyImport_Fini2() */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    /* Allocate new memory for the combined table */
    p = NULL;
    if (i + n <= SIZE_MAX / sizeof(struct _inittab) - 1) {
        size_t size = sizeof(struct _inittab) * (i + n + 1);
        p = PyMem_RawRealloc(inittab_copy, size);
    }
    if (p == NULL) {
        res = -1;
        goto done;
    }

    /* Copy the tables into the new memory at the first call
       to PyImport_ExtendInittab(). */
    if (inittab_copy != PyImport_Inittab) {
        memcpy(p, PyImport_Inittab, (i+1) * sizeof(struct _inittab));
    }
    memcpy(p + i, newtab, (n + 1) * sizeof(struct _inittab));
    PyImport_Inittab = inittab_copy = p;

done:
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return res;
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


PyObject *
_PyImport_GetModuleAttr(PyObject *modname, PyObject *attrname)
{
    PyObject *mod = PyImport_Import(modname);
    if (mod == NULL) {
        return NULL;
    }
    PyObject *result = PyObject_GetAttr(mod, attrname);
    Py_DECREF(mod);
    return result;
}

PyObject *
_PyImport_GetModuleAttrString(const char *modname, const char *attrname)
{
    PyObject *pmodname = PyUnicode_FromString(modname);
    if (pmodname == NULL) {
        return NULL;
    }
    PyObject *pattrname = PyUnicode_FromString(attrname);
    if (pattrname == NULL) {
        Py_DECREF(pmodname);
        return NULL;
    }
    PyObject *result = _PyImport_GetModuleAttr(pmodname, pattrname);
    Py_DECREF(pattrname);
    Py_DECREF(pmodname);
    return result;
}

#ifdef __cplusplus
}
#endif
