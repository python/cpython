/* Module definition and import implementation */

#include "Python.h"
#include "pycore_hashtable.h"     // _Py_hashtable_new_full()
#include "pycore_import.h"        // _PyImport_BootstrapImp()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // struct _import_runtime_state
#include "pycore_namespace.h"     // _PyNamespace_Type
#include "pycore_object.h"        // _Py_SetImmortal()
#include "pycore_pyerrors.h"      // _PyErr_SetString()
#include "pycore_pyhash.h"        // _Py_KeyedHash()
#include "pycore_pylifecycle.h"
#include "pycore_pymem.h"         // _PyMem_SetDefaultAllocator()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_sysmodule.h"     // _PySys_Audit()
#include "pycore_time.h"          // _PyTime_PerfCounterUnchecked()
#include "pycore_weakref.h"       // _PyWeakref_GET_REF()

#include "marshal.h"              // PyMarshal_ReadObjectFromString()
#include "pycore_importdl.h"      // _PyImport_DynLoadFiletab
#include "pydtrace.h"             // PyDTrace_IMPORT_FIND_LOAD_START_ENABLED()
#include <stdbool.h>              // bool

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif


/*[clinic input]
module _imp
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=9c332475d8686284]*/

#include "clinic/import.c.h"


/*******************************/
/* process-global import state */
/*******************************/

/* This table is defined in config.c: */
extern struct _inittab _PyImport_Inittab[];

// This is not used after Py_Initialize() is called.
// (See _PyRuntimeState.imports.inittab.)
struct _inittab *PyImport_Inittab = _PyImport_Inittab;
// When we dynamically allocate a larger table for PyImport_ExtendInittab(),
// we track the pointer here so we can deallocate it during finalization.
static struct _inittab *inittab_copy = NULL;


/*******************************/
/* runtime-global import state */
/*******************************/

#define INITTAB _PyRuntime.imports.inittab
#define LAST_MODULE_INDEX _PyRuntime.imports.last_module_index
#define EXTENSIONS _PyRuntime.imports.extensions

#define PKGCONTEXT (_PyRuntime.imports.pkgcontext)


/*******************************/
/* interpreter import state */
/*******************************/

#define MODULES(interp) \
    (interp)->imports.modules
#define MODULES_BY_INDEX(interp) \
    (interp)->imports.modules_by_index
#define IMPORTLIB(interp) \
    (interp)->imports.importlib
#define OVERRIDE_MULTI_INTERP_EXTENSIONS_CHECK(interp) \
    (interp)->imports.override_multi_interp_extensions_check
#define OVERRIDE_FROZEN_MODULES(interp) \
    (interp)->imports.override_frozen_modules
#ifdef HAVE_DLOPEN
#  define DLOPENFLAGS(interp) \
        (interp)->imports.dlopenflags
#endif
#define IMPORT_FUNC(interp) \
    (interp)->imports.import_func

#define IMPORT_LOCK(interp) \
    (interp)->imports.lock.mutex
#define IMPORT_LOCK_THREAD(interp) \
    (interp)->imports.lock.thread
#define IMPORT_LOCK_LEVEL(interp) \
    (interp)->imports.lock.level

#define FIND_AND_LOAD(interp) \
    (interp)->imports.find_and_load


/*******************/
/* the import lock */
/*******************/

/* Locking primitives to prevent parallel imports of the same module
   in different threads to return with a partially loaded module.
   These calls are serialized by the global interpreter lock. */

void
_PyImport_AcquireLock(PyInterpreterState *interp)
{
    unsigned long me = PyThread_get_thread_ident();
    if (me == PYTHREAD_INVALID_THREAD_ID)
        return; /* Too bad */
    if (IMPORT_LOCK(interp) == NULL) {
        IMPORT_LOCK(interp) = PyThread_allocate_lock();
        if (IMPORT_LOCK(interp) == NULL)
            return;  /* Nothing much we can do. */
    }
    if (IMPORT_LOCK_THREAD(interp) == me) {
        IMPORT_LOCK_LEVEL(interp)++;
        return;
    }
    if (IMPORT_LOCK_THREAD(interp) != PYTHREAD_INVALID_THREAD_ID ||
        !PyThread_acquire_lock(IMPORT_LOCK(interp), 0))
    {
        PyThreadState *tstate = PyEval_SaveThread();
        PyThread_acquire_lock(IMPORT_LOCK(interp), WAIT_LOCK);
        PyEval_RestoreThread(tstate);
    }
    assert(IMPORT_LOCK_LEVEL(interp) == 0);
    IMPORT_LOCK_THREAD(interp) = me;
    IMPORT_LOCK_LEVEL(interp) = 1;
}

int
_PyImport_ReleaseLock(PyInterpreterState *interp)
{
    unsigned long me = PyThread_get_thread_ident();
    if (me == PYTHREAD_INVALID_THREAD_ID || IMPORT_LOCK(interp) == NULL)
        return 0; /* Too bad */
    if (IMPORT_LOCK_THREAD(interp) != me)
        return -1;
    IMPORT_LOCK_LEVEL(interp)--;
    assert(IMPORT_LOCK_LEVEL(interp) >= 0);
    if (IMPORT_LOCK_LEVEL(interp) == 0) {
        IMPORT_LOCK_THREAD(interp) = PYTHREAD_INVALID_THREAD_ID;
        PyThread_release_lock(IMPORT_LOCK(interp));
    }
    return 1;
}

#ifdef HAVE_FORK
/* This function is called from PyOS_AfterFork_Child() to ensure that newly
   created child processes do not share locks with the parent.
   We now acquire the import lock around fork() calls but on some platforms
   (Solaris 9 and earlier? see isue7242) that still left us with problems. */
PyStatus
_PyImport_ReInitLock(PyInterpreterState *interp)
{
    if (IMPORT_LOCK(interp) != NULL) {
        if (_PyThread_at_fork_reinit(&IMPORT_LOCK(interp)) < 0) {
            return _PyStatus_ERR("failed to create a new lock");
        }
    }

    if (IMPORT_LOCK_LEVEL(interp) > 1) {
        /* Forked as a side effect of import */
        unsigned long me = PyThread_get_thread_ident();
        PyThread_acquire_lock(IMPORT_LOCK(interp), WAIT_LOCK);
        IMPORT_LOCK_THREAD(interp) = me;
        IMPORT_LOCK_LEVEL(interp)--;
    } else {
        IMPORT_LOCK_THREAD(interp) = PYTHREAD_INVALID_THREAD_ID;
        IMPORT_LOCK_LEVEL(interp) = 0;
    }
    return _PyStatus_OK();
}
#endif


/***************/
/* sys.modules */
/***************/

PyObject *
_PyImport_InitModules(PyInterpreterState *interp)
{
    assert(MODULES(interp) == NULL);
    MODULES(interp) = PyDict_New();
    if (MODULES(interp) == NULL) {
        return NULL;
    }
    return MODULES(interp);
}

PyObject *
_PyImport_GetModules(PyInterpreterState *interp)
{
    return MODULES(interp);
}

void
_PyImport_ClearModules(PyInterpreterState *interp)
{
    Py_SETREF(MODULES(interp), NULL);
}

static inline PyObject *
get_modules_dict(PyThreadState *tstate, bool fatal)
{
    /* Technically, it would make sense to incref the dict,
     * since sys.modules could be swapped out and decref'ed to 0
     * before the caller is done using it.  However, that is highly
     * unlikely, especially since we can rely on a global lock
     * (i.e. the GIL) for thread-safety. */
    PyObject *modules = MODULES(tstate->interp);
    if (modules == NULL) {
        if (fatal) {
            Py_FatalError("interpreter has no modules dictionary");
        }
        _PyErr_SetString(tstate, PyExc_RuntimeError,
                         "unable to get sys.modules");
        return NULL;
    }
    return modules;
}

PyObject *
PyImport_GetModuleDict(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return get_modules_dict(tstate, true);
}

int
_PyImport_SetModule(PyObject *name, PyObject *m)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *modules = get_modules_dict(tstate, true);
    return PyObject_SetItem(modules, name, m);
}

int
_PyImport_SetModuleString(const char *name, PyObject *m)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *modules = get_modules_dict(tstate, true);
    return PyMapping_SetItemString(modules, name, m);
}

static PyObject *
import_get_module(PyThreadState *tstate, PyObject *name)
{
    PyObject *modules = get_modules_dict(tstate, false);
    if (modules == NULL) {
        return NULL;
    }

    PyObject *m;
    Py_INCREF(modules);
    (void)PyMapping_GetOptionalItem(modules, name, &m);
    Py_DECREF(modules);
    return m;
}

static int
import_ensure_initialized(PyInterpreterState *interp, PyObject *mod, PyObject *name)
{
    PyObject *spec;

    /* Optimization: only call _bootstrap._lock_unlock_module() if
       __spec__._initializing is true.
       NOTE: because of this, initializing must be set *before*
       stuffing the new module in sys.modules.
    */
    int rc = PyObject_GetOptionalAttr(mod, &_Py_ID(__spec__), &spec);
    if (rc > 0) {
        rc = _PyModuleSpec_IsInitializing(spec);
        Py_DECREF(spec);
    }
    if (rc <= 0) {
        return rc;
    }
    /* Wait until module is done importing. */
    PyObject *value = PyObject_CallMethodOneArg(
        IMPORTLIB(interp), &_Py_ID(_lock_unlock_module), name);
    if (value == NULL) {
        return -1;
    }
    Py_DECREF(value);
    return 0;
}

static void remove_importlib_frames(PyThreadState *tstate);

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

/* Get the module object corresponding to a module name.
   First check the modules dictionary if there's one there,
   if not, create a new one and insert it in the modules dictionary. */

static PyObject *
import_add_module(PyThreadState *tstate, PyObject *name)
{
    PyObject *modules = get_modules_dict(tstate, false);
    if (modules == NULL) {
        return NULL;
    }

    PyObject *m;
    if (PyMapping_GetOptionalItem(modules, name, &m) < 0) {
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
PyImport_AddModuleRef(const char *name)
{
    PyObject *name_obj = PyUnicode_FromString(name);
    if (name_obj == NULL) {
        return NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *module = import_add_module(tstate, name_obj);
    Py_DECREF(name_obj);
    return module;
}


PyObject *
PyImport_AddModuleObject(PyObject *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *mod = import_add_module(tstate, name);
    if (!mod) {
        return NULL;
    }

    // gh-86160: PyImport_AddModuleObject() returns a borrowed reference.
    // Create a weak reference to produce a borrowed reference, since it can
    // become NULL. sys.modules type can be different than dict and it is not
    // guaranteed that it keeps a strong reference to the module. It can be a
    // custom mapping with __getitem__() which returns a new object or removes
    // returned object, or __setitem__ which does nothing. There is so much
    // unknown.  With weakref we can be sure that we get either a reference to
    // live object or NULL.
    //
    // Use PyImport_AddModuleRef() to avoid these issues.
    PyObject *ref = PyWeakref_NewRef(mod, NULL);
    Py_DECREF(mod);
    if (ref == NULL) {
        return NULL;
    }
    mod = _PyWeakref_GET_REF(ref);
    Py_DECREF(ref);
    Py_XDECREF(mod);

    if (mod == NULL && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_RuntimeError,
                        "sys.modules does not hold a strong reference "
                        "to the module");
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
    PyObject *exc = _PyErr_GetRaisedException(tstate);

    PyObject *modules = get_modules_dict(tstate, true);
    if (PyDict_CheckExact(modules)) {
        // Error is reported to the caller
        (void)PyDict_Pop(modules, name, NULL);
    }
    else if (PyMapping_DelItem(modules, name) < 0) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
            _PyErr_Clear(tstate);
        }
    }

    _PyErr_ChainExceptions1(exc);
}


/************************************/
/* per-interpreter modules-by-index */
/************************************/

Py_ssize_t
_PyImport_GetNextModuleIndex(void)
{
    return _Py_atomic_add_ssize(&LAST_MODULE_INDEX, 1) + 1;
}

static const char *
_modules_by_index_check(PyInterpreterState *interp, Py_ssize_t index)
{
    if (index == 0) {
        return "invalid module index";
    }
    if (MODULES_BY_INDEX(interp) == NULL) {
        return "Interpreters module-list not accessible.";
    }
    if (index > PyList_GET_SIZE(MODULES_BY_INDEX(interp))) {
        return "Module index out of bounds.";
    }
    return NULL;
}

static PyObject *
_modules_by_index_get(PyInterpreterState *interp, PyModuleDef *def)
{
    Py_ssize_t index = def->m_base.m_index;
    if (_modules_by_index_check(interp, index) != NULL) {
        return NULL;
    }
    PyObject *res = PyList_GET_ITEM(MODULES_BY_INDEX(interp), index);
    return res==Py_None ? NULL : res;
}

static int
_modules_by_index_set(PyInterpreterState *interp,
                      PyModuleDef *def, PyObject *module)
{
    assert(def != NULL);
    assert(def->m_slots == NULL);
    assert(def->m_base.m_index > 0);

    if (MODULES_BY_INDEX(interp) == NULL) {
        MODULES_BY_INDEX(interp) = PyList_New(0);
        if (MODULES_BY_INDEX(interp) == NULL) {
            return -1;
        }
    }

    Py_ssize_t index = def->m_base.m_index;
    while (PyList_GET_SIZE(MODULES_BY_INDEX(interp)) <= index) {
        if (PyList_Append(MODULES_BY_INDEX(interp), Py_None) < 0) {
            return -1;
        }
    }

    return PyList_SetItem(MODULES_BY_INDEX(interp), index, Py_NewRef(module));
}

static int
_modules_by_index_clear_one(PyInterpreterState *interp, PyModuleDef *def)
{
    Py_ssize_t index = def->m_base.m_index;
    const char *err = _modules_by_index_check(interp, index);
    if (err != NULL) {
        Py_FatalError(err);
        return -1;
    }
    return PyList_SetItem(MODULES_BY_INDEX(interp), index, Py_NewRef(Py_None));
}


PyObject*
PyState_FindModule(PyModuleDef* module)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (module->m_slots) {
        return NULL;
    }
    return _modules_by_index_get(interp, module);
}

/* _PyState_AddModule() has been completely removed from the C-API
   (and was removed from the limited API in 3.6).  However, we're
   playing it safe and keeping it around for any stable ABI extensions
   built against 3.2-3.5. */
int
_PyState_AddModule(PyThreadState *tstate, PyObject* module, PyModuleDef* def)
{
    if (!def) {
        assert(_PyErr_Occurred(tstate));
        return -1;
    }
    if (def->m_slots) {
        _PyErr_SetString(tstate,
                         PyExc_SystemError,
                         "PyState_AddModule called on module with slots");
        return -1;
    }
    return _modules_by_index_set(tstate->interp, def, module);
}

int
PyState_AddModule(PyObject* module, PyModuleDef* def)
{
    if (!def) {
        Py_FatalError("module definition is NULL");
        return -1;
    }

    PyThreadState *tstate = _PyThreadState_GET();
    if (def->m_slots) {
        _PyErr_SetString(tstate,
                         PyExc_SystemError,
                         "PyState_AddModule called on module with slots");
        return -1;
    }

    PyInterpreterState *interp = tstate->interp;
    Py_ssize_t index = def->m_base.m_index;
    if (MODULES_BY_INDEX(interp) &&
        index < PyList_GET_SIZE(MODULES_BY_INDEX(interp)) &&
        module == PyList_GET_ITEM(MODULES_BY_INDEX(interp), index))
    {
        _Py_FatalErrorFormat(__func__, "module %p already added", module);
        return -1;
    }

    return _modules_by_index_set(interp, def, module);
}

int
PyState_RemoveModule(PyModuleDef* def)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (def->m_slots) {
        _PyErr_SetString(tstate,
                         PyExc_SystemError,
                         "PyState_RemoveModule called on module with slots");
        return -1;
    }
    return _modules_by_index_clear_one(tstate->interp, def);
}


// Used by finalize_modules()
void
_PyImport_ClearModulesByIndex(PyInterpreterState *interp)
{
    if (!MODULES_BY_INDEX(interp)) {
        return;
    }

    Py_ssize_t i;
    for (i = 0; i < PyList_GET_SIZE(MODULES_BY_INDEX(interp)); i++) {
        PyObject *m = PyList_GET_ITEM(MODULES_BY_INDEX(interp), i);
        if (PyModule_Check(m)) {
            /* cleanup the saved copy of module dicts */
            PyModuleDef *md = PyModule_GetDef(m);
            if (md) {
                Py_CLEAR(md->m_base.m_copy);
            }
        }
    }

    /* Setting modules_by_index to NULL could be dangerous, so we
       clear the list instead. */
    if (PyList_SetSlice(MODULES_BY_INDEX(interp),
                        0, PyList_GET_SIZE(MODULES_BY_INDEX(interp)),
                        NULL)) {
        PyErr_FormatUnraisable("Exception ignored on clearing interpreters module list");
    }
}


/*********************/
/* extension modules */
/*********************/

/*
    It may help to have a big picture view of what happens
    when an extension is loaded.  This includes when it is imported
    for the first time.

    Here's a summary, using importlib._bootstrap._load() as a starting point.

    1.  importlib._bootstrap._load()
    2.    _load():  acquire import lock
    3.    _load() -> importlib._bootstrap._load_unlocked()
    4.      _load_unlocked() -> importlib._bootstrap.module_from_spec()
    5.        module_from_spec() -> ExtensionFileLoader.create_module()
    6.          create_module() -> _imp.create_dynamic()
                    (see below)
    7.        module_from_spec() -> importlib._bootstrap._init_module_attrs()
    8.      _load_unlocked():  sys.modules[name] = module
    9.      _load_unlocked() -> ExtensionFileLoader.exec_module()
    10.       exec_module() -> _imp.exec_dynamic()
                  (see below)
    11.   _load():  release import lock


    ...for single-phase init modules, where m_size == -1:

    (6). first time  (not found in _PyRuntime.imports.extensions):
       A. _imp_create_dynamic_impl() -> import_find_extension()
       B. _imp_create_dynamic_impl() -> _PyImport_GetModInitFunc()
       C.   _PyImport_GetModInitFunc():  load <module init func>
       D. _imp_create_dynamic_impl() -> import_run_extension()
       E.   import_run_extension() -> _PyImport_RunModInitFunc()
       F.     _PyImport_RunModInitFunc():  call <module init func>
       G.       <module init func> -> PyModule_Create() -> PyModule_Create2()
                                          -> PyModule_CreateInitialized()
       H.         PyModule_CreateInitialized() -> PyModule_New()
       I.         PyModule_CreateInitialized():  allocate mod->md_state
       J.         PyModule_CreateInitialized() -> PyModule_AddFunctions()
       K.         PyModule_CreateInitialized() -> PyModule_SetDocString()
       L.       PyModule_CreateInitialized():  set mod->md_def
       M.       <module init func>:  initialize the module, etc.
       N.   import_run_extension()
                -> _PyImport_CheckSubinterpIncompatibleExtensionAllowed()
       O.   import_run_extension():  set __file__
       P.   import_run_extension() -> update_global_state_for_extension()
       Q.     update_global_state_for_extension():
                      copy __dict__ into def->m_base.m_copy
       R.     update_global_state_for_extension():
                      add it to _PyRuntime.imports.extensions
       S.   import_run_extension() -> finish_singlephase_extension()
       T.     finish_singlephase_extension():
                      add it to interp->imports.modules_by_index
       U.     finish_singlephase_extension():  add it to sys.modules

       Step (Q) is skipped for core modules (sys/builtins).

    (6). subsequent times  (found in _PyRuntime.imports.extensions):
       A. _imp_create_dynamic_impl() -> import_find_extension()
       B.   import_find_extension() -> reload_singlephase_extension()
       C.     reload_singlephase_extension()
                  -> _PyImport_CheckSubinterpIncompatibleExtensionAllowed()
       D.     reload_singlephase_extension() -> import_add_module()
       E.       if name in sys.modules:  use that module
       F.       else:
                  1. import_add_module() -> PyModule_NewObject()
                  2. import_add_module():  set it on sys.modules
       G.     reload_singlephase_extension():  copy the "m_copy" dict into __dict__
       H.     reload_singlephase_extension():  add to modules_by_index

    (10). (every time):
       A. noop


    ...for single-phase init modules, where m_size >= 0:

    (6). not main interpreter and never loaded there - every time  (not found in _PyRuntime.imports.extensions):
       A-P. (same as for m_size == -1)
       Q.     _PyImport_RunModInitFunc():  set def->m_base.m_init
       R. (skipped)
       S-U. (same as for m_size == -1)

    (6). main interpreter - first time  (not found in _PyRuntime.imports.extensions):
       A-P. (same as for m_size == -1)
       Q.     _PyImport_RunModInitFunc():  set def->m_base.m_init
       R-U. (same as for m_size == -1)

    (6). subsequent times  (found in _PyRuntime.imports.extensions):
       A. _imp_create_dynamic_impl() -> import_find_extension()
       B.   import_find_extension() -> reload_singlephase_extension()
       C.     reload_singlephase_extension()
                  -> _PyImport_CheckSubinterpIncompatibleExtensionAllowed()
       D.     reload_singlephase_extension():  call def->m_base.m_init  (see above)
       E.     reload_singlephase_extension():  add the module to sys.modules
       F.     reload_singlephase_extension():  add to modules_by_index

    (10). every time:
       A. noop


    ...for multi-phase init modules:

    (6). every time:
       A. _imp_create_dynamic_impl() -> import_find_extension()  (not found)
       B. _imp_create_dynamic_impl() -> _PyImport_GetModInitFunc()
       C.   _PyImport_GetModInitFunc():  load <module init func>
       D. _imp_create_dynamic_impl() -> import_run_extension()
       E.   import_run_extension() -> _PyImport_RunModInitFunc()
       F.     _PyImport_RunModInitFunc():  call <module init func>
       G.   import_run_extension() -> PyModule_FromDefAndSpec()
       H.      PyModule_FromDefAndSpec(): gather/check moduledef slots
       I.      if there's a Py_mod_create slot:
                 1. PyModule_FromDefAndSpec():  call its function
       J.      else:
                 1. PyModule_FromDefAndSpec() -> PyModule_NewObject()
       K:      PyModule_FromDefAndSpec():  set mod->md_def
       L.      PyModule_FromDefAndSpec() -> _add_methods_to_object()
       M.      PyModule_FromDefAndSpec() -> PyModule_SetDocString()

    (10). every time:
       A. _imp_exec_dynamic_impl() -> exec_builtin_or_dynamic()
       B.   if mod->md_state == NULL (including if m_size == 0):
            1. exec_builtin_or_dynamic() -> PyModule_ExecDef()
            2.   PyModule_ExecDef():  allocate mod->md_state
            3.   if there's a Py_mod_exec slot:
                 1. PyModule_ExecDef():  call its function
 */


/* Make sure name is fully qualified.

   This is a bit of a hack: when the shared library is loaded,
   the module name is "package.module", but the module calls
   PyModule_Create*() with just "module" for the name.  The shared
   library loader squirrels away the true name of the module in
   _PyRuntime.imports.pkgcontext, and PyModule_Create*() will
   substitute this (if the name actually matches).
*/

#ifdef HAVE_THREAD_LOCAL
_Py_thread_local const char *pkgcontext = NULL;
# undef PKGCONTEXT
# define PKGCONTEXT pkgcontext
#endif

const char *
_PyImport_ResolveNameWithPackageContext(const char *name)
{
#ifndef HAVE_THREAD_LOCAL
    PyThread_acquire_lock(EXTENSIONS.mutex, WAIT_LOCK);
#endif
    if (PKGCONTEXT != NULL) {
        const char *p = strrchr(PKGCONTEXT, '.');
        if (p != NULL && strcmp(name, p+1) == 0) {
            name = PKGCONTEXT;
            PKGCONTEXT = NULL;
        }
    }
#ifndef HAVE_THREAD_LOCAL
    PyThread_release_lock(EXTENSIONS.mutex);
#endif
    return name;
}

const char *
_PyImport_SwapPackageContext(const char *newcontext)
{
#ifndef HAVE_THREAD_LOCAL
    PyThread_acquire_lock(EXTENSIONS.mutex, WAIT_LOCK);
#endif
    const char *oldcontext = PKGCONTEXT;
    PKGCONTEXT = newcontext;
#ifndef HAVE_THREAD_LOCAL
    PyThread_release_lock(EXTENSIONS.mutex);
#endif
    return oldcontext;
}

#ifdef HAVE_DLOPEN
int
_PyImport_GetDLOpenFlags(PyInterpreterState *interp)
{
    return DLOPENFLAGS(interp);
}

void
_PyImport_SetDLOpenFlags(PyInterpreterState *interp, int new_val)
{
    DLOPENFLAGS(interp) = new_val;
}
#endif  // HAVE_DLOPEN


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


static int clear_singlephase_extension(PyInterpreterState *interp,
                                       PyObject *name, PyObject *filename);

// Currently, this is only used for testing.
// (See _testinternalcapi.clear_extension().)
int
_PyImport_ClearExtension(PyObject *name, PyObject *filename)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();

    /* Clearing a module's C globals is up to the module. */
    if (clear_singlephase_extension(interp, name, filename) < 0) {
        return -1;
    }

    // In the future we'll probably also make sure the extension's
    // file handle (and DL handle) is closed (requires saving it).

    return 0;
}


/*****************************/
/* single-phase init modules */
/*****************************/

/*
We support a number of kinds of single-phase init builtin/extension modules:

* "basic"
    * no module state (PyModuleDef.m_size == -1)
    * does not support repeated init (we use PyModuleDef.m_base.m_copy)
    * may have process-global state
    * the module's def is cached in _PyRuntime.imports.extensions,
      by (name, filename)
* "reinit"
    * no module state (PyModuleDef.m_size == 0)
    * supports repeated init (m_copy is never used)
    * should not have any process-global state
    * its def is never cached in _PyRuntime.imports.extensions
      (except, currently, under the main interpreter, for some reason)
* "with state"  (almost the same as reinit)
    * has module state (PyModuleDef.m_size > 0)
    * supports repeated init (m_copy is never used)
    * should not have any process-global state
    * its def is never cached in _PyRuntime.imports.extensions
      (except, currently, under the main interpreter, for some reason)

There are also variants within those classes:

* two or more modules share a PyModuleDef
    * a module's init func uses another module's PyModuleDef
    * a module's init func calls another's module's init func
    * a module's init "func" is actually a variable statically initialized
      to another module's init func
* two or modules share "methods"
    * a module's init func copies another module's PyModuleDef
      (with a different name)
* (basic-only) two or modules share process-global state

In the first case, where modules share a PyModuleDef, the following
notable weirdness happens:

* the module's __name__ matches the def, not the requested name
* the last module (with the same def) to be imported for the first time wins
    * returned by PyState_Find_Module() (via interp->modules_by_index)
    * (non-basic-only) its init func is used when re-loading any of them
      (via the def's m_init)
    * (basic-only) the copy of its __dict__ is used when re-loading any of them
      (via the def's m_copy)

However, the following happens as expected:

* a new module object (with its own __dict__) is created for each request
* the module's __spec__ has the requested name
* the loaded module is cached in sys.modules under the requested name
* the m_index field of the shared def is not changed,
  so at least PyState_FindModule() will always look in the same place

For "basic" modules there are other quirks:

* (whether sharing a def or not) when loaded the first time,
  m_copy is set before _init_module_attrs() is called
  in importlib._bootstrap.module_from_spec(),
  so when the module is re-loaded, the previous value
  for __wpec__ (and others) is reset, possibly unexpectedly.

Generally, when multiple interpreters are involved, some of the above
gets even messier.
*/

static inline void
extensions_lock_acquire(void)
{
    PyMutex_Lock(&_PyRuntime.imports.extensions.mutex);
}

static inline void
extensions_lock_release(void)
{
    PyMutex_Unlock(&_PyRuntime.imports.extensions.mutex);
}

/* Magic for extension modules (built-in as well as dynamically
   loaded).  To prevent initializing an extension module more than
   once, we keep a static dictionary 'extensions' keyed by the tuple
   (module name, module name)  (for built-in modules) or by
   (filename, module name) (for dynamically loaded modules), containing these
   modules.  A copy of the module's dictionary is stored by calling
   fix_up_extension() immediately after the module initialization
   function succeeds.  A copy can be retrieved from there by calling
   import_find_extension().

   Modules which do support multiple initialization set their m_size
   field to a non-negative number (indicating the size of the
   module-specific state). They are still recorded in the extensions
   dictionary, to avoid loading shared libraries twice.
*/

static void *
hashtable_key_from_2_strings(PyObject *str1, PyObject *str2, const char sep)
{
    Py_ssize_t str1_len, str2_len;
    const char *str1_data = PyUnicode_AsUTF8AndSize(str1, &str1_len);
    const char *str2_data = PyUnicode_AsUTF8AndSize(str2, &str2_len);
    if (str1_data == NULL || str2_data == NULL) {
        return NULL;
    }
    /* Make sure sep and the NULL byte won't cause an overflow. */
    assert(SIZE_MAX - str1_len - str2_len > 2);
    size_t size = str1_len + 1 + str2_len + 1;

    char *key = PyMem_RawMalloc(size);
    if (key == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    strncpy(key, str1_data, str1_len);
    key[str1_len] = sep;
    strncpy(key + str1_len + 1, str2_data, str2_len + 1);
    assert(strlen(key) == size - 1);
    return key;
}

static Py_uhash_t
hashtable_hash_str(const void *key)
{
    return _Py_HashBytes(key, strlen((const char *)key));
}

static int
hashtable_compare_str(const void *key1, const void *key2)
{
    return strcmp((const char *)key1, (const char *)key2) == 0;
}

static void
hashtable_destroy_str(void *ptr)
{
    PyMem_RawFree(ptr);
}

#define HTSEP ':'

static int
_extensions_cache_init(void)
{
    _Py_hashtable_allocator_t alloc = {PyMem_RawMalloc, PyMem_RawFree};
    EXTENSIONS.hashtable = _Py_hashtable_new_full(
        hashtable_hash_str,
        hashtable_compare_str,
        hashtable_destroy_str,  // key
        /* There's no need to decref the def since it's immortal. */
        NULL,  // value
        &alloc
    );
    if (EXTENSIONS.hashtable == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

static _Py_hashtable_entry_t *
_extensions_cache_find_unlocked(PyObject *path, PyObject *name,
                                void **p_key)
{
    if (EXTENSIONS.hashtable == NULL) {
        return NULL;
    }
    void *key = hashtable_key_from_2_strings(path, name, HTSEP);
    if (key == NULL) {
        return NULL;
    }
    _Py_hashtable_entry_t *entry =
            _Py_hashtable_get_entry(EXTENSIONS.hashtable, key);
    if (p_key != NULL) {
        *p_key = key;
    }
    else {
        hashtable_destroy_str(key);
    }
    return entry;
}

static PyModuleDef *
_extensions_cache_get(PyObject *path, PyObject *name)
{
    PyModuleDef *def = NULL;
    extensions_lock_acquire();

    _Py_hashtable_entry_t *entry =
            _extensions_cache_find_unlocked(path, name, NULL);
    if (entry == NULL) {
        /* It was never added. */
        goto finally;
    }
    def = (PyModuleDef *)entry->value;

finally:
    extensions_lock_release();
    return def;
}

static int
_extensions_cache_set(PyObject *path, PyObject *name, PyModuleDef *def)
{
    int res = -1;
    assert(def != NULL);
    extensions_lock_acquire();

    if (EXTENSIONS.hashtable == NULL) {
        if (_extensions_cache_init() < 0) {
            goto finally;
        }
    }

    int already_set = 0;
    void *key = NULL;
    _Py_hashtable_entry_t *entry =
            _extensions_cache_find_unlocked(path, name, &key);
    if (entry == NULL) {
        /* It was never added. */
        if (_Py_hashtable_set(EXTENSIONS.hashtable, key, def) < 0) {
            PyErr_NoMemory();
            goto finally;
        }
        /* The hashtable owns the key now. */
        key = NULL;
    }
    else if (entry->value == NULL) {
        /* It was previously deleted. */
        entry->value = def;
    }
    else {
        /* We expect it to be static, so it must be the same pointer. */
        assert((PyModuleDef *)entry->value == def);
        /* It was already added. */
        already_set = 1;
    }

    if (!already_set) {
        /* We assume that all module defs are statically allocated
           and will never be freed.  Otherwise, we would incref here. */
        _Py_SetImmortal((PyObject *)def);
    }
    res = 0;

finally:
    extensions_lock_release();
    if (key != NULL) {
        hashtable_destroy_str(key);
    }
    return res;
}

static void
_extensions_cache_delete(PyObject *path, PyObject *name)
{
    extensions_lock_acquire();

    if (EXTENSIONS.hashtable == NULL) {
        /* It was never added. */
        goto finally;
    }

    _Py_hashtable_entry_t *entry =
            _extensions_cache_find_unlocked(path, name, NULL);
    if (entry == NULL) {
        /* It was never added. */
        goto finally;
    }
    if (entry->value == NULL) {
        /* It was already removed. */
        goto finally;
    }
    /* If we hadn't made the stored defs immortal, we would decref here.
       However, this decref would be problematic if the module def were
       dynamically allocated, it were the last ref, and this function
       were called with an interpreter other than the def's owner. */
    assert(_Py_IsImmortal(entry->value));
    entry->value = NULL;

finally:
    extensions_lock_release();
}

static void
_extensions_cache_clear_all(void)
{
    /* The runtime (i.e. main interpreter) must be finalizing,
       so we don't need to worry about the lock. */
    _Py_hashtable_destroy(EXTENSIONS.hashtable);
    EXTENSIONS.hashtable = NULL;
}

#undef HTSEP


static bool
check_multi_interp_extensions(PyInterpreterState *interp)
{
    int override = OVERRIDE_MULTI_INTERP_EXTENSIONS_CHECK(interp);
    if (override < 0) {
        return false;
    }
    else if (override > 0) {
        return true;
    }
    else if (_PyInterpreterState_HasFeature(
                interp, Py_RTFLAGS_MULTI_INTERP_EXTENSIONS)) {
        return true;
    }
    return false;
}

int
_PyImport_CheckSubinterpIncompatibleExtensionAllowed(const char *name)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (check_multi_interp_extensions(interp)) {
        assert(!_Py_IsMainInterpreter(interp));
        PyErr_Format(PyExc_ImportError,
                     "module %s does not support loading in subinterpreters",
                     name);
        return -1;
    }
    return 0;
}

static PyObject *
get_core_module_dict(PyInterpreterState *interp,
                     PyObject *name, PyObject *path)
{
    /* Only builtin modules are core. */
    if (path == name) {
        assert(!PyErr_Occurred());
        if (PyUnicode_CompareWithASCIIString(name, "sys") == 0) {
            return interp->sysdict_copy;
        }
        assert(!PyErr_Occurred());
        if (PyUnicode_CompareWithASCIIString(name, "builtins") == 0) {
            return interp->builtins_copy;
        }
        assert(!PyErr_Occurred());
    }
    return NULL;
}

static inline int
is_core_module(PyInterpreterState *interp, PyObject *name, PyObject *path)
{
    /* This might be called before the core dict copies are in place,
       so we can't rely on get_core_module_dict() here. */
    if (path == name) {
        if (PyUnicode_CompareWithASCIIString(name, "sys") == 0) {
            return 1;
        }
        if (PyUnicode_CompareWithASCIIString(name, "builtins") == 0) {
            return 1;
        }
    }
    return 0;
}

#ifndef NDEBUG
static _Py_ext_module_kind
_get_extension_kind(PyModuleDef *def, bool check_size)
{
    _Py_ext_module_kind kind;
    if (def == NULL) {
        /* It must be a module created by reload_singlephase_extension()
         * from m_copy.  Ideally we'd do away with this case. */
        kind = _Py_ext_module_kind_SINGLEPHASE;
    }
    else if (def->m_slots != NULL) {
        kind = _Py_ext_module_kind_MULTIPHASE;
    }
    else if (check_size && def->m_size == -1) {
        kind = _Py_ext_module_kind_SINGLEPHASE;
    }
    else if (def->m_base.m_init != NULL) {
        kind = _Py_ext_module_kind_SINGLEPHASE;
    }
    else {
        // This is probably single-phase init, but a multi-phase
        // module *can* have NULL m_slots.
        kind = _Py_ext_module_kind_UNKNOWN;
    }
    return kind;
}

/* The module might not be fully initialized yet
 * and PyModule_FromDefAndSpec() checks m_size
 * so we skip m_size. */
#define assert_multiphase_def(def)                                  \
    do {                                                            \
        _Py_ext_module_kind kind = _get_extension_kind(def, false); \
        assert(kind == _Py_ext_module_kind_MULTIPHASE               \
                /* m_slots can be NULL. */                          \
                || kind == _Py_ext_module_kind_UNKNOWN);            \
    } while (0)

#define assert_singlephase_def(def)                                 \
    do {                                                            \
        _Py_ext_module_kind kind = _get_extension_kind(def, true);  \
        assert(kind == _Py_ext_module_kind_SINGLEPHASE              \
                || kind == _Py_ext_module_kind_UNKNOWN);            \
    } while (0)

#define assert_singlephase(def) \
    assert_singlephase_def(def)

#else  /* defined(NDEBUG) */
#define assert_multiphase_def(def)
#define assert_singlephase_def(def)
#define assert_singlephase(def)
#endif


struct singlephase_global_update {
    PyObject *m_dict;
    PyModInitFunction m_init;
};

static int
update_global_state_for_extension(PyThreadState *tstate,
                                  PyObject *path, PyObject *name,
                                  PyModuleDef *def,
                                  struct singlephase_global_update *singlephase)
{
    /* Copy the module's __dict__, if applicable. */
    if (singlephase == NULL) {
        assert(def->m_base.m_copy == NULL);
    }
    else {
        if (singlephase->m_init != NULL) {
            assert(singlephase->m_dict == NULL);
            assert(def->m_base.m_copy == NULL);
            assert(def->m_size >= 0);
            /* Remember pointer to module init function. */
            // XXX If two modules share a def then def->m_base will
            // reflect the last one added (here) to the global cache.
            // We should prevent this somehow.  The simplest solution
            // is probably to store m_copy/m_init in the cache along
            // with the def, rather than within the def.
            def->m_base.m_init = singlephase->m_init;
        }
        else if (singlephase->m_dict == NULL) {
            /* It must be a core builtin module. */
            assert(is_core_module(tstate->interp, name, path));
            assert(def->m_size == -1);
            assert(def->m_base.m_copy == NULL);
            assert(def->m_base.m_init == NULL);
        }
        else {
            assert(PyDict_Check(singlephase->m_dict));
            // gh-88216: Extensions and def->m_base.m_copy can be updated
            // when the extension module doesn't support sub-interpreters.
            assert(def->m_size == -1);
            assert(!is_core_module(tstate->interp, name, path));
            assert(PyUnicode_CompareWithASCIIString(name, "sys") != 0);
            assert(PyUnicode_CompareWithASCIIString(name, "builtins") != 0);
            /* XXX gh-88216: The copied dict is owned by the current
             * interpreter.  That's a problem if the interpreter has
             * its own obmalloc state or if the module is successfully
             * imported into such an interpreter.  If the interpreter
             * has its own GIL then there may be data races and
             * PyImport_ClearModulesByIndex() can crash.  Normally,
             * a single-phase init module cannot be imported in an
             * isolated interpreter, but there are ways around that.
             * Hence, heere be dragons!  Ideally we would instead do
             * something like make a read-only, immortal copy of the
             * dict using PyMem_RawMalloc() and store *that* in m_copy.
             * Then we'd need to make sure to clear that when the
             * runtime is finalized, rather than in
             * PyImport_ClearModulesByIndex(). */
            if (def->m_base.m_copy) {
                /* Somebody already imported the module,
                   likely under a different name.
                   XXX this should really not happen. */
                Py_CLEAR(def->m_base.m_copy);
            }
            def->m_base.m_copy = PyDict_Copy(singlephase->m_dict);
            if (def->m_base.m_copy == NULL) {
                // XXX Ignore this error?  Doing so would effectively
                // mark the module as not loadable. */
                return -1;
            }
        }
    }

    /* Add the module's def to the global cache. */
    // XXX Why special-case the main interpreter?
    if (_Py_IsMainInterpreter(tstate->interp) || def->m_size == -1) {
#ifndef NDEBUG
        PyModuleDef *cached = _extensions_cache_get(path, name);
        assert(cached == NULL || cached == def);
#endif
        if (_extensions_cache_set(path, name, def) < 0) {
            return -1;
        }
    }

    return 0;
}

/* For multi-phase init modules, the module is finished
 * by PyModule_FromDefAndSpec(). */
static int
finish_singlephase_extension(PyThreadState *tstate,
                             PyObject *mod, PyModuleDef *def,
                             PyObject *name, PyObject *modules)
{
    assert(mod != NULL && PyModule_Check(mod));
    assert(def == _PyModule_GetDef(mod));

    if (_modules_by_index_set(tstate->interp, def, mod) < 0) {
        return -1;
    }

    if (modules != NULL) {
        if (PyObject_SetItem(modules, name, mod) < 0) {
            return -1;
        }
    }

    return 0;
}


static PyObject *
reload_singlephase_extension(PyThreadState *tstate, PyModuleDef *def,
                             struct _Py_ext_module_loader_info *info)
{
    assert_singlephase(def);
    PyObject *mod = NULL;

    /* It may have been successfully imported previously
       in an interpreter that allows legacy modules
       but is not allowed in the current interpreter. */
    const char *name_buf = PyUnicode_AsUTF8(info->name);
    assert(name_buf != NULL);
    if (_PyImport_CheckSubinterpIncompatibleExtensionAllowed(name_buf) < 0) {
        return NULL;
    }

    PyObject *modules = get_modules_dict(tstate, true);
    if (def->m_size == -1) {
        PyObject *m_copy = def->m_base.m_copy;
        /* Module does not support repeated initialization */
        if (m_copy == NULL) {
            /* It might be a core module (e.g. sys & builtins),
               for which we don't set m_copy. */
            m_copy = get_core_module_dict(
                    tstate->interp, info->name, info->path);
            if (m_copy == NULL) {
                assert(!PyErr_Occurred());
                return NULL;
            }
        }
        mod = import_add_module(tstate, info->name);
        if (mod == NULL) {
            return NULL;
        }
        PyObject *mdict = PyModule_GetDict(mod);
        if (mdict == NULL) {
            Py_DECREF(mod);
            return NULL;
        }
        if (PyDict_Update(mdict, m_copy)) {
            Py_DECREF(mod);
            return NULL;
        }
        /* We can't set mod->md_def if it's missing,
         * because _PyImport_ClearModulesByIndex() might break
         * due to violating interpreter isolation.  See the note
         * in fix_up_extension_for_interpreter().  Until that
         * is solved, we leave md_def set to NULL. */
        assert(_PyModule_GetDef(mod) == NULL
               || _PyModule_GetDef(mod) == def);
    }
    else {
        if (def->m_base.m_init == NULL) {
            assert(!PyErr_Occurred());
            return NULL;
        }
        struct _Py_ext_module_loader_result res;
        if (_PyImport_RunModInitFunc(def->m_base.m_init, info, &res) < 0) {
            _Py_ext_module_loader_result_apply_error(&res, name_buf);
            return NULL;
        }
        assert(!PyErr_Occurred());
        assert(res.err == NULL);
        assert(res.kind == _Py_ext_module_kind_SINGLEPHASE);
        mod = res.module;
        /* Tchnically, the init function could return a different module def.
         * Then we would probably need to update the global cache.
         * However, we don't expect anyone to change the def. */
        assert(res.def == def);
        _Py_ext_module_loader_result_clear(&res);

        /* Remember the filename as the __file__ attribute */
        if (info->filename != NULL) {
            if (PyModule_AddObjectRef(mod, "__file__", info->filename) < 0) {
                PyErr_Clear(); /* Not important enough to report */
            }
        }

        if (PyObject_SetItem(modules, info->name, mod) == -1) {
            Py_DECREF(mod);
            return NULL;
        }
    }

    if (_modules_by_index_set(tstate->interp, def, mod) < 0) {
        PyMapping_DelItem(modules, info->name);
        Py_DECREF(mod);
        return NULL;
    }

    return mod;
}

static PyObject *
import_find_extension(PyThreadState *tstate,
                      struct _Py_ext_module_loader_info *info)
{
    /* Only single-phase init modules will be in the cache. */
    PyModuleDef *def = _extensions_cache_get(info->path, info->name);
    if (def == NULL) {
        return NULL;
    }
    assert_singlephase(def);

    /* It may have been successfully imported previously
       in an interpreter that allows legacy modules
       but is not allowed in the current interpreter. */
    const char *name_buf = PyUnicode_AsUTF8(info->name);
    assert(name_buf != NULL);
    if (_PyImport_CheckSubinterpIncompatibleExtensionAllowed(name_buf) < 0) {
        return NULL;
    }

    PyObject *mod = reload_singlephase_extension(tstate, def, info);
    if (mod == NULL) {
        return NULL;
    }

    int verbose = _PyInterpreterState_GetConfig(tstate->interp)->verbose;
    if (verbose) {
        PySys_FormatStderr("import %U # previously loaded (%R)\n",
                           info->name, info->path);
    }
    return mod;
}

static PyObject *
import_run_extension(PyThreadState *tstate, PyModInitFunction p0,
                     struct _Py_ext_module_loader_info *info,
                     PyObject *spec, PyObject *modules)
{
    /* Core modules go through _PyImport_FixupBuiltin(). */
    assert(!is_core_module(tstate->interp, info->name, info->path));

    PyObject *mod = NULL;
    PyModuleDef *def = NULL;
    const char *name_buf = PyBytes_AS_STRING(info->name_encoded);

    struct _Py_ext_module_loader_result res;
    if (_PyImport_RunModInitFunc(p0, info, &res) < 0) {
        /* We discard res.def. */
        assert(res.module == NULL);
        _Py_ext_module_loader_result_apply_error(&res, name_buf);
        return NULL;
    }
    assert(!PyErr_Occurred());
    assert(res.err == NULL);

    mod = res.module;
    res.module = NULL;
    def = res.def;
    assert(def != NULL);

    if (res.kind == _Py_ext_module_kind_MULTIPHASE) {
        assert_multiphase_def(def);
        assert(mod == NULL);
        mod = PyModule_FromDefAndSpec(def, spec);
        if (mod == NULL) {
            goto error;
        }
    }
    else {
        assert(res.kind == _Py_ext_module_kind_SINGLEPHASE);
        assert_singlephase_def(def);
        assert(PyModule_Check(mod));

        if (_PyImport_CheckSubinterpIncompatibleExtensionAllowed(name_buf) < 0) {
            goto error;
        }

        /* Remember the filename as the __file__ attribute */
        if (info->filename != NULL) {
            if (PyModule_AddObjectRef(mod, "__file__", info->filename) < 0) {
                PyErr_Clear(); /* Not important enough to report */
            }
        }

        /* Update global import state. */
        struct singlephase_global_update singlephase = {0};
        // gh-88216: Extensions and def->m_base.m_copy can be updated
        // when the extension module doesn't support sub-interpreters.
        if (def->m_size == -1) {
            /* We will reload from m_copy. */
            assert(def->m_base.m_init == NULL);
            singlephase.m_dict = PyModule_GetDict(mod);
            assert(singlephase.m_dict != NULL);
        }
        else {
            /* We will reload via the init function. */
            assert(def->m_size >= 0);
            singlephase.m_init = p0;
        }
        if (update_global_state_for_extension(
                tstate, info->path, info->name, def, &singlephase) < 0)
        {
            goto error;
        }

        /* Update per-interpreter import state. */
        PyObject *modules = get_modules_dict(tstate, true);
        if (finish_singlephase_extension(
                tstate, mod, def, info->name, modules) < 0)
        {
            goto error;
        }
    }

    _Py_ext_module_loader_result_clear(&res);
    return mod;

error:
    Py_XDECREF(mod);
    _Py_ext_module_loader_result_clear(&res);
    return NULL;
}


static int
clear_singlephase_extension(PyInterpreterState *interp,
                            PyObject *name, PyObject *path)
{
    PyModuleDef *def = _extensions_cache_get(path, name);
    if (def == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        return 0;
    }

    /* Clear data set when the module was initially loaded. */
    def->m_base.m_init = NULL;
    Py_CLEAR(def->m_base.m_copy);
    // We leave m_index alone since there's no reason to reset it.

    /* Clear the PyState_*Module() cache entry. */
    if (_modules_by_index_check(interp, def->m_base.m_index) == NULL) {
        if (_modules_by_index_clear_one(interp, def) < 0) {
            return -1;
        }
    }

    /* Clear the cached module def. */
    _extensions_cache_delete(path, name);

    return 0;
}


/*******************/
/* builtin modules */
/*******************/

int
_PyImport_FixupBuiltin(PyThreadState *tstate, PyObject *mod, const char *name,
                       PyObject *modules)
{
    int res = -1;
    assert(mod != NULL && PyModule_Check(mod));

    PyObject *nameobj;
    nameobj = PyUnicode_InternFromString(name);
    if (nameobj == NULL) {
        return -1;
    }

    PyModuleDef *def = PyModule_GetDef(mod);
    if (def == NULL) {
        PyErr_BadInternalCall();
        goto finally;
    }

    /* We only use _PyImport_FixupBuiltin() for the core builtin modules
     * (sys and builtins).  These modules are single-phase init with no
     * module state, but we also don't populate def->m_base.m_copy
     * for them. */
    assert(is_core_module(tstate->interp, nameobj, nameobj));
    assert_singlephase_def(def);
    assert(def->m_size == -1);
    assert(def->m_base.m_copy == NULL);

    struct singlephase_global_update singlephase = {
        /* We don't want def->m_base.m_copy populated. */
        .m_dict=NULL,
    };
    if (update_global_state_for_extension(
            tstate, nameobj, nameobj, def, &singlephase) < 0)
    {
        goto finally;
    }

    if (finish_singlephase_extension(tstate, mod, def, nameobj, modules) < 0) {
        goto finally;
    }

    res = 0;

finally:
    Py_DECREF(nameobj);
    return res;
}

/* Helper to test for built-in module */

static int
is_builtin(PyObject *name)
{
    int i;
    struct _inittab *inittab = INITTAB;
    for (i = 0; inittab[i].name != NULL; i++) {
        if (_PyUnicode_EqualToASCIIString(name, inittab[i].name)) {
            if (inittab[i].initfunc == NULL)
                return -1;
            else
                return 1;
        }
    }
    return 0;
}

static PyObject*
create_builtin(PyThreadState *tstate, PyObject *name, PyObject *spec)
{
    struct _Py_ext_module_loader_info info;
    if (_Py_ext_module_loader_info_init_for_builtin(&info, name) < 0) {
        return NULL;
    }

    PyObject *mod = import_find_extension(tstate, &info);
    if (mod != NULL) {
        assert(!_PyErr_Occurred(tstate));
        assert_singlephase(_PyModule_GetDef(mod));
        goto finally;
    }
    else if (_PyErr_Occurred(tstate)) {
        goto finally;
    }

    struct _inittab *found = NULL;
    for (struct _inittab *p = INITTAB; p->name != NULL; p++) {
        if (_PyUnicode_EqualToASCIIString(info.name, p->name)) {
            found = p;
        }
    }
    if (found == NULL) {
        // not found
        mod = Py_NewRef(Py_None);
        goto finally;
    }

    PyModInitFunction p0 = (PyModInitFunction)found->initfunc;
    if (p0 == NULL) {
        /* Cannot re-init internal module ("sys" or "builtins") */
        assert(is_core_module(tstate->interp, info.name, info.path));
        mod = import_add_module(tstate, info.name);
        goto finally;
    }

    /* Now load it. */
    mod = import_run_extension(
                    tstate, p0, &info, spec, get_modules_dict(tstate, true));

finally:
    _Py_ext_module_loader_info_clear(&info);
    return mod;
}


/*****************************/
/* the builtin modules table */
/*****************************/

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

    if (INITTAB != NULL) {
        Py_FatalError("PyImport_ExtendInittab() may not be called after Py_Initialize()");
    }

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

    if (INITTAB != NULL) {
        Py_FatalError("PyImport_AppendInittab() may not be called after Py_Initialize()");
    }

    memset(newtab, '\0', sizeof newtab);

    newtab[0].name = name;
    newtab[0].initfunc = initfunc;

    return PyImport_ExtendInittab(newtab);
}


/* the internal table */

static int
init_builtin_modules_table(void)
{
    size_t size;
    for (size = 0; PyImport_Inittab[size].name != NULL; size++)
        ;
    size++;

    /* Make the copy. */
    struct _inittab *copied = PyMem_RawMalloc(size * sizeof(struct _inittab));
    if (copied == NULL) {
        return -1;
    }
    memcpy(copied, PyImport_Inittab, size * sizeof(struct _inittab));
    INITTAB = copied;
    return 0;
}

static void
fini_builtin_modules_table(void)
{
    struct _inittab *inittab = INITTAB;
    INITTAB = NULL;
    PyMem_RawFree(inittab);
}

PyObject *
_PyImport_GetBuiltinModuleNames(void)
{
    PyObject *list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }
    struct _inittab *inittab = INITTAB;
    for (Py_ssize_t i = 0; inittab[i].name != NULL; i++) {
        PyObject *name = PyUnicode_FromString(inittab[i].name);
        if (name == NULL) {
            Py_DECREF(list);
            return NULL;
        }
        if (PyList_Append(list, name) < 0) {
            Py_DECREF(name);
            Py_DECREF(list);
            return NULL;
        }
        Py_DECREF(name);
    }
    return list;
}


/********************/
/* the magic number */
/********************/

/* Helper for pythonrun.c -- return magic number and tag. */

long
PyImport_GetMagicNumber(void)
{
    long res;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *external, *pyc_magic;

    external = PyObject_GetAttrString(IMPORTLIB(interp), "_bootstrap_external");
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


/*********************************/
/* a Python module's code object */
/*********************************/

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

        if (interp == NULL) {
            Py_FatalError("no current interpreter");
        }

        external= PyObject_GetAttrString(IMPORTLIB(interp),
                                         "_bootstrap_external");
        if (external != NULL) {
            pathobj = PyObject_CallMethodOneArg(
                external, &_Py_ID(_get_sourcefile), cpathobj);
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
    PyObject *m, *d;

    m = import_add_module(tstate, name);
    if (m == NULL)
        return NULL;
    /* If the module is being reloaded, we get the old module back
       and re-use its dict to exec the new code. */
    d = PyModule_GetDict(m);
    int r = PyDict_Contains(d, &_Py_ID(__builtins__));
    if (r == 0) {
        r = PyDict_SetItem(d, &_Py_ID(__builtins__), PyEval_GetBuiltins());
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

    d = module_dict_for_exec(tstate, name);
    if (d == NULL) {
        return NULL;
    }

    if (pathname == NULL) {
        pathname = ((PyCodeObject *)co)->co_filename;
    }
    external = PyObject_GetAttrString(IMPORTLIB(tstate->interp),
                                      "_bootstrap_external");
    if (external == NULL) {
        Py_DECREF(d);
        return NULL;
    }
    res = PyObject_CallMethodObjArgs(external, &_Py_ID(_fix_up_module),
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

    Py_XSETREF(co->co_filename, Py_NewRef(newname));

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


/******************/
/* frozen modules */
/******************/

/* Return true if the name is an alias.  In that case, "alias" is set
   to the original module name.  If it is an alias but the original
   module isn't known then "alias" is set to NULL while true is returned. */
static bool
resolve_module_alias(const char *name, const struct _module_alias *aliases,
                     const char **alias)
{
    const struct _module_alias *entry;
    for (entry = aliases; ; entry++) {
        if (entry->name == NULL) {
            /* It isn't an alias. */
            return false;
        }
        if (strcmp(name, entry->name) == 0) {
            if (alias != NULL) {
                *alias = entry->orig;
            }
            return true;
        }
    }
}

static bool
use_frozen(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    int override = OVERRIDE_FROZEN_MODULES(interp);
    if (override > 0) {
        return true;
    }
    else if (override < 0) {
        return false;
    }
    else {
        return interp->config.use_frozen_modules;
    }
}

static PyObject *
list_frozen_module_names(void)
{
    PyObject *names = PyList_New(0);
    if (names == NULL) {
        return NULL;
    }
    bool enabled = use_frozen();
    const struct _frozen *p;
#define ADD_MODULE(name) \
    do { \
        PyObject *nameobj = PyUnicode_FromString(name); \
        if (nameobj == NULL) { \
            goto error; \
        } \
        int res = PyList_Append(names, nameobj); \
        Py_DECREF(nameobj); \
        if (res != 0) { \
            goto error; \
        } \
    } while(0)
    // We always use the bootstrap modules.
    for (p = _PyImport_FrozenBootstrap; ; p++) {
        if (p->name == NULL) {
            break;
        }
        ADD_MODULE(p->name);
    }
    // Frozen stdlib modules may be disabled.
    for (p = _PyImport_FrozenStdlib; ; p++) {
        if (p->name == NULL) {
            break;
        }
        if (enabled) {
            ADD_MODULE(p->name);
        }
    }
    for (p = _PyImport_FrozenTest; ; p++) {
        if (p->name == NULL) {
            break;
        }
        if (enabled) {
            ADD_MODULE(p->name);
        }
    }
#undef ADD_MODULE
    // Add any custom modules.
    if (PyImport_FrozenModules != NULL) {
        for (p = PyImport_FrozenModules; ; p++) {
            if (p->name == NULL) {
                break;
            }
            PyObject *nameobj = PyUnicode_FromString(p->name);
            if (nameobj == NULL) {
                goto error;
            }
            int found = PySequence_Contains(names, nameobj);
            if (found < 0) {
                Py_DECREF(nameobj);
                goto error;
            }
            else if (found) {
                Py_DECREF(nameobj);
            }
            else {
                int res = PyList_Append(names, nameobj);
                Py_DECREF(nameobj);
                if (res != 0) {
                    goto error;
                }
            }
        }
    }
    return names;

error:
    Py_DECREF(names);
    return NULL;
}

typedef enum {
    FROZEN_OKAY,
    FROZEN_BAD_NAME,    // The given module name wasn't valid.
    FROZEN_NOT_FOUND,   // It wasn't in PyImport_FrozenModules.
    FROZEN_DISABLED,    // -X frozen_modules=off (and not essential)
    FROZEN_EXCLUDED,    /* The PyImport_FrozenModules entry has NULL "code"
                           (module is present but marked as unimportable, stops search). */
    FROZEN_INVALID,     /* The PyImport_FrozenModules entry is bogus
                           (eg. does not contain executable code). */
} frozen_status;

static inline void
set_frozen_error(frozen_status status, PyObject *modname)
{
    const char *err = NULL;
    switch (status) {
        case FROZEN_BAD_NAME:
        case FROZEN_NOT_FOUND:
            err = "No such frozen object named %R";
            break;
        case FROZEN_DISABLED:
            err = "Frozen modules are disabled and the frozen object named %R is not essential";
            break;
        case FROZEN_EXCLUDED:
            err = "Excluded frozen object named %R";
            break;
        case FROZEN_INVALID:
            err = "Frozen object named %R is invalid";
            break;
        case FROZEN_OKAY:
            // There was no error.
            break;
        default:
            Py_UNREACHABLE();
    }
    if (err != NULL) {
        PyObject *msg = PyUnicode_FromFormat(err, modname);
        if (msg == NULL) {
            PyErr_Clear();
        }
        PyErr_SetImportError(msg, modname, NULL);
        Py_XDECREF(msg);
    }
}

static const struct _frozen *
look_up_frozen(const char *name)
{
    const struct _frozen *p;
    // We always use the bootstrap modules.
    for (p = _PyImport_FrozenBootstrap; ; p++) {
        if (p->name == NULL) {
            // We hit the end-of-list sentinel value.
            break;
        }
        if (strcmp(name, p->name) == 0) {
            return p;
        }
    }
    // Prefer custom modules, if any.  Frozen stdlib modules can be
    // disabled here by setting "code" to NULL in the array entry.
    if (PyImport_FrozenModules != NULL) {
        for (p = PyImport_FrozenModules; ; p++) {
            if (p->name == NULL) {
                break;
            }
            if (strcmp(name, p->name) == 0) {
                return p;
            }
        }
    }
    // Frozen stdlib modules may be disabled.
    if (use_frozen()) {
        for (p = _PyImport_FrozenStdlib; ; p++) {
            if (p->name == NULL) {
                break;
            }
            if (strcmp(name, p->name) == 0) {
                return p;
            }
        }
        for (p = _PyImport_FrozenTest; ; p++) {
            if (p->name == NULL) {
                break;
            }
            if (strcmp(name, p->name) == 0) {
                return p;
            }
        }
    }
    return NULL;
}

struct frozen_info {
    PyObject *nameobj;
    const char *data;
    Py_ssize_t size;
    bool is_package;
    bool is_alias;
    const char *origname;
};

static frozen_status
find_frozen(PyObject *nameobj, struct frozen_info *info)
{
    if (info != NULL) {
        memset(info, 0, sizeof(*info));
    }

    if (nameobj == NULL || nameobj == Py_None) {
        return FROZEN_BAD_NAME;
    }
    const char *name = PyUnicode_AsUTF8(nameobj);
    if (name == NULL) {
        // Note that this function previously used
        // _PyUnicode_EqualToASCIIString().  We clear the error here
        // (instead of propagating it) to match the earlier behavior
        // more closely.
        PyErr_Clear();
        return FROZEN_BAD_NAME;
    }

    const struct _frozen *p = look_up_frozen(name);
    if (p == NULL) {
        return FROZEN_NOT_FOUND;
    }
    if (info != NULL) {
        info->nameobj = nameobj;  // borrowed
        info->data = (const char *)p->code;
        info->size = p->size;
        info->is_package = p->is_package;
        if (p->size < 0) {
            // backward compatibility with negative size values
            info->size = -(p->size);
            info->is_package = true;
        }
        info->origname = name;
        info->is_alias = resolve_module_alias(name, _PyImport_FrozenAliases,
                                              &info->origname);
    }
    if (p->code == NULL) {
        /* It is frozen but marked as un-importable. */
        return FROZEN_EXCLUDED;
    }
    if (p->code[0] == '\0' || p->size == 0) {
        /* Does not contain executable code. */
        return FROZEN_INVALID;
    }
    return FROZEN_OKAY;
}

static PyObject *
unmarshal_frozen_code(PyInterpreterState *interp, struct frozen_info *info)
{
    PyObject *co = PyMarshal_ReadObjectFromString(info->data, info->size);
    if (co == NULL) {
        /* Does not contain executable code. */
        PyErr_Clear();
        set_frozen_error(FROZEN_INVALID, info->nameobj);
        return NULL;
    }
    if (!PyCode_Check(co)) {
        // We stick with TypeError for backward compatibility.
        PyErr_Format(PyExc_TypeError,
                     "frozen object %R is not a code object",
                     info->nameobj);
        Py_DECREF(co);
        return NULL;
    }
    return co;
}


/* Initialize a frozen module.
   Return 1 for success, 0 if the module is not found, and -1 with
   an exception set if the initialization failed.
   This function is also used from frozenmain.c */

int
PyImport_ImportFrozenModuleObject(PyObject *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *co, *m, *d = NULL;
    int err;

    struct frozen_info info;
    frozen_status status = find_frozen(name, &info);
    if (status == FROZEN_NOT_FOUND || status == FROZEN_DISABLED) {
        return 0;
    }
    else if (status == FROZEN_BAD_NAME) {
        return 0;
    }
    else if (status != FROZEN_OKAY) {
        set_frozen_error(status, name);
        return -1;
    }
    co = unmarshal_frozen_code(tstate->interp, &info);
    if (co == NULL) {
        return -1;
    }
    if (info.is_package) {
        /* Set __path__ to the empty list */
        PyObject *l;
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
    if (m == NULL) {
        goto err_return;
    }
    Py_DECREF(m);
    /* Set __origname__ (consumed in FrozenImporter._setup_module()). */
    PyObject *origname;
    if (info.origname) {
        origname = PyUnicode_FromString(info.origname);
        if (origname == NULL) {
            goto err_return;
        }
    }
    else {
        origname = Py_NewRef(Py_None);
    }
    err = PyDict_SetItemString(d, "__origname__", origname);
    Py_DECREF(origname);
    if (err != 0) {
        goto err_return;
    }
    Py_DECREF(d);
    Py_DECREF(co);
    return 1;

err_return:
    Py_XDECREF(d);
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


/*************/
/* importlib */
/*************/

/* Import the _imp extension by calling manually _imp.create_builtin() and
   _imp.exec_builtin() since importlib is not initialized yet. Initializing
   importlib requires the _imp module: this function fix the bootstrap issue.
 */
static PyObject*
bootstrap_imp(PyThreadState *tstate)
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

/* Global initializations.  Can be undone by Py_FinalizeEx().  Don't
   call this twice without an intervening Py_FinalizeEx() call.  When
   initializations fail, a fatal error is issued and the function does
   not return.  On return, the first thread and interpreter state have
   been created.

   Locking: you must hold the interpreter lock while calling this.
   (If the lock has not yet been initialized, that's equivalent to
   having the lock, but you cannot use multiple threads.)

*/
static int
init_importlib(PyThreadState *tstate, PyObject *sysmod)
{
    assert(!_PyErr_Occurred(tstate));

    PyInterpreterState *interp = tstate->interp;
    int verbose = _PyInterpreterState_GetConfig(interp)->verbose;

    // Import _importlib through its frozen version, _frozen_importlib.
    if (verbose) {
        PySys_FormatStderr("import _frozen_importlib # frozen\n");
    }
    if (PyImport_ImportFrozenModule("_frozen_importlib") <= 0) {
        return -1;
    }

    PyObject *importlib = PyImport_AddModuleRef("_frozen_importlib");
    if (importlib == NULL) {
        return -1;
    }
    IMPORTLIB(interp) = importlib;

    // Import the _imp module
    if (verbose) {
        PySys_FormatStderr("import _imp # builtin\n");
    }
    PyObject *imp_mod = bootstrap_imp(tstate);
    if (imp_mod == NULL) {
        return -1;
    }
    if (_PyImport_SetModuleString("_imp", imp_mod) < 0) {
        Py_DECREF(imp_mod);
        return -1;
    }

    // Install importlib as the implementation of import
    PyObject *value = PyObject_CallMethod(importlib, "_install",
                                          "OO", sysmod, imp_mod);
    Py_DECREF(imp_mod);
    if (value == NULL) {
        return -1;
    }
    Py_DECREF(value);

    assert(!_PyErr_Occurred(tstate));
    return 0;
}


static int
init_importlib_external(PyInterpreterState *interp)
{
    PyObject *value;
    value = PyObject_CallMethod(IMPORTLIB(interp),
                                "_install_external_importers", "");
    if (value == NULL) {
        return -1;
    }
    Py_DECREF(value);
    return 0;
}

PyObject *
_PyImport_GetImportlibLoader(PyInterpreterState *interp,
                             const char *loader_name)
{
    return PyObject_GetAttrString(IMPORTLIB(interp), loader_name);
}

PyObject *
_PyImport_GetImportlibExternalLoader(PyInterpreterState *interp,
                                     const char *loader_name)
{
    PyObject *bootstrap = PyObject_GetAttrString(IMPORTLIB(interp),
                                                 "_bootstrap_external");
    if (bootstrap == NULL) {
        return NULL;
    }

    PyObject *loader_type = PyObject_GetAttrString(bootstrap, loader_name);
    Py_DECREF(bootstrap);
    return loader_type;
}

PyObject *
_PyImport_BlessMyLoader(PyInterpreterState *interp, PyObject *module_globals)
{
    PyObject *external = PyObject_GetAttrString(IMPORTLIB(interp),
                                                "_bootstrap_external");
    if (external == NULL) {
        return NULL;
    }

    PyObject *loader = PyObject_CallMethod(external, "_bless_my_loader",
                                           "O", module_globals, NULL);
    Py_DECREF(external);
    return loader;
}

PyObject *
_PyImport_ImportlibModuleRepr(PyInterpreterState *interp, PyObject *m)
{
    return PyObject_CallMethod(IMPORTLIB(interp), "_module_repr", "O", m);
}


/*******************/

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

    if (!PyList_Check(path_hooks)) {
        PyErr_SetString(PyExc_RuntimeError, "sys.path_hooks is not a list");
        return NULL;
    }
    if (!PyDict_Check(path_importer_cache)) {
        PyErr_SetString(PyExc_RuntimeError, "sys.path_importer_cache is not a dict");
        return NULL;
    }

    nhooks = PyList_Size(path_hooks);
    if (nhooks < 0)
        return NULL; /* Shouldn't happen */

    if (PyDict_GetItemRef(path_importer_cache, p, &importer) != 0) {
        // found or error
        return importer;
    }
    // not found
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
    if (path_importer_cache == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "lost sys.path_importer_cache");
        return NULL;
    }
    Py_INCREF(path_importer_cache);
    PyObject *path_hooks = PySys_GetObject("path_hooks");
    if (path_hooks == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "lost sys.path_hooks");
        Py_DECREF(path_importer_cache);
        return NULL;
    }
    Py_INCREF(path_hooks);
    PyObject *importer = get_path_importer(tstate, path_importer_cache, path_hooks, path);
    Py_DECREF(path_hooks);
    Py_DECREF(path_importer_cache);
    return importer;
}


/*********************/
/* importing modules */
/*********************/

int
_PyImport_InitDefaultImportFunc(PyInterpreterState *interp)
{
    // Get the __import__ function
    PyObject *import_func;
    if (PyDict_GetItemStringRef(interp->builtins, "__import__", &import_func) <= 0) {
        return -1;
    }
    IMPORT_FUNC(interp) = import_func;
    return 0;
}

int
_PyImport_IsDefaultImportFunc(PyInterpreterState *interp, PyObject *func)
{
    return func == IMPORT_FUNC(interp);
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
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
        "PyImport_ImportModuleNoBlock() is deprecated and scheduled for "
        "removal in Python 3.15. Use PyImport_ImportModule() instead.", 1))
    {
        return NULL;
    }
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
    PyObject **prev_link, **outer_link = NULL;
    PyObject *base_tb = NULL;

    /* Synopsis: if it's an ImportError, we trim all importlib chunks
       from the traceback. We always trim chunks
       which end with a call to "_call_with_frames_removed". */

    PyObject *exc = _PyErr_GetRaisedException(tstate);
    if (exc == NULL || _PyInterpreterState_GetConfig(tstate->interp)->verbose) {
        goto done;
    }

    if (PyType_IsSubtype(Py_TYPE(exc), (PyTypeObject *) PyExc_ImportError)) {
        always_trim = 1;
    }

    assert(PyExceptionInstance_Check(exc));
    base_tb = PyException_GetTraceback(exc);
    prev_link = &base_tb;
    PyObject *tb = base_tb;
    while (tb != NULL) {
        assert(PyTraceBack_Check(tb));
        PyTracebackObject *traceback = (PyTracebackObject *)tb;
        PyObject *next = (PyObject *) traceback->tb_next;
        PyFrameObject *frame = traceback->tb_frame;
        PyCodeObject *code = PyFrame_GetCode(frame);
        int now_in_importlib;

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
            Py_XSETREF(*outer_link, Py_XNewRef(next));
            prev_link = outer_link;
        }
        else {
            prev_link = (PyObject **) &traceback->tb_next;
        }
        Py_DECREF(code);
        tb = next;
    }
    if (base_tb == NULL) {
        base_tb = Py_None;
        Py_INCREF(Py_None);
    }
    PyException_SetTraceback(exc, base_tb);
done:
    Py_XDECREF(base_tb);
    _PyErr_SetRaisedException(tstate, exc);
}


static PyObject *
resolve_name(PyThreadState *tstate, PyObject *name, PyObject *globals, int level)
{
    PyObject *abs_name;
    PyObject *package = NULL;
    PyObject *spec = NULL;
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
    if (PyDict_GetItemRef(globals, &_Py_ID(__package__), &package) < 0) {
        goto error;
    }
    if (package == Py_None) {
        Py_DECREF(package);
        package = NULL;
    }
    if (PyDict_GetItemRef(globals, &_Py_ID(__spec__), &spec) < 0) {
        goto error;
    }

    if (package != NULL) {
        if (!PyUnicode_Check(package)) {
            _PyErr_SetString(tstate, PyExc_TypeError,
                             "package must be a string");
            goto error;
        }
        else if (spec != NULL && spec != Py_None) {
            int equal;
            PyObject *parent = PyObject_GetAttr(spec, &_Py_ID(parent));
            if (parent == NULL) {
                goto error;
            }

            equal = PyObject_RichCompareBool(package, parent, Py_EQ);
            Py_DECREF(parent);
            if (equal < 0) {
                goto error;
            }
            else if (equal == 0) {
                if (PyErr_WarnEx(PyExc_DeprecationWarning,
                        "__package__ != __spec__.parent", 1) < 0) {
                    goto error;
                }
            }
        }
    }
    else if (spec != NULL && spec != Py_None) {
        package = PyObject_GetAttr(spec, &_Py_ID(parent));
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

        if (PyDict_GetItemRef(globals, &_Py_ID(__name__), &package) < 0) {
            goto error;
        }
        if (package == NULL) {
            _PyErr_SetString(tstate, PyExc_KeyError,
                             "'__name__' not in globals");
            goto error;
        }

        if (!PyUnicode_Check(package)) {
            _PyErr_SetString(tstate, PyExc_TypeError,
                             "__name__ must be a string");
            goto error;
        }

        int haspath = PyDict_Contains(globals, &_Py_ID(__path__));
        if (haspath < 0) {
            goto error;
        }
        if (!haspath) {
            Py_ssize_t dot;

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

    Py_XDECREF(spec);
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
    Py_XDECREF(spec);
    Py_XDECREF(package);
    return NULL;
}

static PyObject *
import_find_and_load(PyThreadState *tstate, PyObject *abs_name)
{
    PyObject *mod = NULL;
    PyInterpreterState *interp = tstate->interp;
    int import_time = _PyInterpreterState_GetConfig(interp)->import_time;
#define import_level FIND_AND_LOAD(interp).import_level
#define accumulated FIND_AND_LOAD(interp).accumulated

    PyTime_t t1 = 0, accumulated_copy = accumulated;

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
#define header FIND_AND_LOAD(interp).header
        if (header) {
            fputs("import time: self [us] | cumulative | imported package\n",
                  stderr);
            header = 0;
        }
#undef header

        import_level++;
        t1 = _PyTime_PerfCounterUnchecked();
        accumulated = 0;
    }

    if (PyDTrace_IMPORT_FIND_LOAD_START_ENABLED())
        PyDTrace_IMPORT_FIND_LOAD_START(PyUnicode_AsUTF8(abs_name));

    mod = PyObject_CallMethodObjArgs(IMPORTLIB(interp), &_Py_ID(_find_and_load),
                                     abs_name, IMPORT_FUNC(interp), NULL);

    if (PyDTrace_IMPORT_FIND_LOAD_DONE_ENABLED())
        PyDTrace_IMPORT_FIND_LOAD_DONE(PyUnicode_AsUTF8(abs_name),
                                       mod != NULL);

    if (import_time) {
        PyTime_t cum = _PyTime_PerfCounterUnchecked() - t1;

        import_level--;
        fprintf(stderr, "import time: %9ld | %10ld | %*s%s\n",
                (long)_PyTime_AsMicroseconds(cum - accumulated, _PyTime_ROUND_CEILING),
                (long)_PyTime_AsMicroseconds(cum, _PyTime_ROUND_CEILING),
                import_level*2, "", PyUnicode_AsUTF8(abs_name));

        accumulated = accumulated_copy + cum;
    }

    return mod;
#undef import_level
#undef accumulated
}

PyObject *
PyImport_ImportModuleLevelObject(PyObject *name, PyObject *globals,
                                 PyObject *locals, PyObject *fromlist,
                                 int level)
{
    PyThreadState *tstate = _PyThreadState_GET();
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
        abs_name = Py_NewRef(name);
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
                final_mod = Py_NewRef(mod);
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
            final_mod = Py_NewRef(mod);
        }
    }
    else {
        int has_path = PyObject_HasAttrWithError(mod, &_Py_ID(__path__));
        if (has_path < 0) {
            goto error;
        }
        if (has_path) {
            final_mod = PyObject_CallMethodObjArgs(
                        IMPORTLIB(interp), &_Py_ID(_handle_fromlist),
                        mod, fromlist, IMPORT_FUNC(interp), NULL);
        }
        else {
            final_mod = Py_NewRef(mod);
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
    PyObject *reloaded_module = NULL;
    PyObject *importlib = PyImport_GetModule(&_Py_ID(importlib));
    if (importlib == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }

        importlib = PyImport_ImportModule("importlib");
        if (importlib == NULL) {
            return NULL;
        }
    }

    reloaded_module = PyObject_CallMethodOneArg(importlib, &_Py_ID(reload), m);
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
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *globals = NULL;
    PyObject *import = NULL;
    PyObject *builtins = NULL;
    PyObject *r = NULL;

    PyObject *from_list = PyList_New(0);
    if (from_list == NULL) {
        goto err;
    }

    /* Get the builtins from current globals */
    globals = PyEval_GetGlobals();
    if (globals != NULL) {
        Py_INCREF(globals);
        builtins = PyObject_GetItem(globals, &_Py_ID(__builtins__));
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
        globals = Py_BuildValue("{OO}", &_Py_ID(__builtins__), builtins);
        if (globals == NULL)
            goto err;
    }

    /* Get the __import__ function from the builtins */
    if (PyDict_Check(builtins)) {
        import = PyObject_GetItem(builtins, &_Py_ID(__import__));
        if (import == NULL) {
            _PyErr_SetObject(tstate, PyExc_KeyError, &_Py_ID(__import__));
        }
    }
    else
        import = PyObject_GetAttr(builtins, &_Py_ID(__import__));
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


/*********************/
/* runtime lifecycle */
/*********************/

PyStatus
_PyImport_Init(void)
{
    if (INITTAB != NULL) {
        return _PyStatus_ERR("global import state already initialized");
    }

    PyStatus status = _PyStatus_OK();

    /* Force default raw memory allocator to get a known allocator to be able
       to release the memory in _PyImport_Fini() */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (init_builtin_modules_table() != 0) {
        status = PyStatus_NoMemory();
        goto done;
    }

done:
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return status;
}

void
_PyImport_Fini(void)
{
    /* Destroy the database used by _PyImport_{Fixup,Find}Extension */
    // XXX Should we actually leave them (mostly) intact, since we don't
    // ever dlclose() the module files?
    _extensions_cache_clear_all();

    /* Use the same memory allocator as _PyImport_Init(). */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    /* Free memory allocated by _PyImport_Init() */
    fini_builtin_modules_table();

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
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


/*************************/
/* interpreter lifecycle */
/*************************/

PyStatus
_PyImport_InitCore(PyThreadState *tstate, PyObject *sysmod, int importlib)
{
    // XXX Initialize here: interp->modules and interp->import_func.
    // XXX Initialize here: sys.modules and sys.meta_path.

    if (importlib) {
        /* This call sets up builtin and frozen import support */
        if (init_importlib(tstate, sysmod) < 0) {
            return _PyStatus_ERR("failed to initialize importlib");
        }
    }

    return _PyStatus_OK();
}

/* In some corner cases it is important to be sure that the import
   machinery has been initialized (or not cleaned up yet).  For
   example, see issue #4236 and PyModule_Create2(). */

int
_PyImport_IsInitialized(PyInterpreterState *interp)
{
    if (MODULES(interp) == NULL)
        return 0;
    return 1;
}

/* Clear the direct per-interpreter import state, if not cleared already. */
void
_PyImport_ClearCore(PyInterpreterState *interp)
{
    /* interp->modules should have been cleaned up and cleared already
       by _PyImport_FiniCore(). */
    Py_CLEAR(MODULES(interp));
    Py_CLEAR(MODULES_BY_INDEX(interp));
    Py_CLEAR(IMPORTLIB(interp));
    Py_CLEAR(IMPORT_FUNC(interp));
}

void
_PyImport_FiniCore(PyInterpreterState *interp)
{
    int verbose = _PyInterpreterState_GetConfig(interp)->verbose;

    if (_PySys_ClearAttrString(interp, "meta_path", verbose) < 0) {
        PyErr_FormatUnraisable("Exception ignored on clearing sys.meta_path");
    }

    // XXX Pull in most of finalize_modules() in pylifecycle.c.

    if (_PySys_ClearAttrString(interp, "modules", verbose) < 0) {
        PyErr_FormatUnraisable("Exception ignored on clearing sys.modules");
    }

    if (IMPORT_LOCK(interp) != NULL) {
        PyThread_free_lock(IMPORT_LOCK(interp));
        IMPORT_LOCK(interp) = NULL;
    }

    _PyImport_ClearCore(interp);
}

// XXX Add something like _PyImport_Disable() for use early in interp fini?


/* "external" imports */

static int
init_zipimport(PyThreadState *tstate, int verbose)
{
    PyObject *path_hooks = PySys_GetObject("path_hooks");
    if (path_hooks == NULL) {
        _PyErr_SetString(tstate, PyExc_RuntimeError,
                         "unable to get sys.path_hooks");
        return -1;
    }

    if (verbose) {
        PySys_WriteStderr("# installing zipimport hook\n");
    }

    PyObject *zipimporter = _PyImport_GetModuleAttrString("zipimport", "zipimporter");
    if (zipimporter == NULL) {
        _PyErr_Clear(tstate); /* No zipimporter object -- okay */
        if (verbose) {
            PySys_WriteStderr("# can't import zipimport.zipimporter\n");
        }
    }
    else {
        /* sys.path_hooks.insert(0, zipimporter) */
        int err = PyList_Insert(path_hooks, 0, zipimporter);
        Py_DECREF(zipimporter);
        if (err < 0) {
            return -1;
        }
        if (verbose) {
            PySys_WriteStderr("# installed zipimport hook\n");
        }
    }

    return 0;
}

PyStatus
_PyImport_InitExternal(PyThreadState *tstate)
{
    int verbose = _PyInterpreterState_GetConfig(tstate->interp)->verbose;

    // XXX Initialize here: sys.path_hooks and sys.path_importer_cache.

    if (init_importlib_external(tstate->interp) != 0) {
        _PyErr_Print(tstate);
        return _PyStatus_ERR("external importer setup failed");
    }

    if (init_zipimport(tstate, verbose) != 0) {
        PyErr_Print();
        return _PyStatus_ERR("initializing zipimport failed");
    }

    return _PyStatus_OK();
}

void
_PyImport_FiniExternal(PyInterpreterState *interp)
{
    int verbose = _PyInterpreterState_GetConfig(interp)->verbose;

    // XXX Uninstall importlib metapath importers here?

    if (_PySys_ClearAttrString(interp, "path_importer_cache", verbose) < 0) {
        PyErr_FormatUnraisable("Exception ignored on clearing sys.path_importer_cache");
    }
    if (_PySys_ClearAttrString(interp, "path_hooks", verbose) < 0) {
        PyErr_FormatUnraisable("Exception ignored on clearing sys.path_hooks");
    }
}


/******************/
/* module helpers */
/******************/

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


/**************/
/* the module */
/**************/

/*[clinic input]
_imp.lock_held

Return True if the import lock is currently held, else False.

On platforms without threads, return False.
[clinic start generated code]*/

static PyObject *
_imp_lock_held_impl(PyObject *module)
/*[clinic end generated code: output=8b89384b5e1963fc input=9b088f9b217d9bdf]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return PyBool_FromLong(
            IMPORT_LOCK_THREAD(interp) != PYTHREAD_INVALID_THREAD_ID);
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
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyImport_AcquireLock(interp);
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
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (_PyImport_ReleaseLock(interp) < 0) {
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
        return NULL;
    }
    Py_RETURN_NONE;
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

    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "name must be string, not %.200s",
                     Py_TYPE(name)->tp_name);
        Py_DECREF(name);
        return NULL;
    }

    PyObject *mod = create_builtin(tstate, name, spec);
    Py_DECREF(name);
    return mod;
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
_imp.find_frozen

    name: unicode
    /
    *
    withdata: bool = False

Return info about the corresponding frozen module (if there is one) or None.

The returned info (a 2-tuple):

 * data         the raw marshalled bytes
 * is_package   whether or not it is a package
 * origname     the originally frozen module's name, or None if not
                a stdlib module (this will usually be the same as
                the module's current name)
[clinic start generated code]*/

static PyObject *
_imp_find_frozen_impl(PyObject *module, PyObject *name, int withdata)
/*[clinic end generated code: output=8c1c3c7f925397a5 input=22a8847c201542fd]*/
{
    struct frozen_info info;
    frozen_status status = find_frozen(name, &info);
    if (status == FROZEN_NOT_FOUND || status == FROZEN_DISABLED) {
        Py_RETURN_NONE;
    }
    else if (status == FROZEN_BAD_NAME) {
        Py_RETURN_NONE;
    }
    else if (status != FROZEN_OKAY) {
        set_frozen_error(status, name);
        return NULL;
    }

    PyObject *data = NULL;
    if (withdata) {
        data = PyMemoryView_FromMemory((char *)info.data, info.size, PyBUF_READ);
        if (data == NULL) {
            return NULL;
        }
    }

    PyObject *origname = NULL;
    if (info.origname != NULL && info.origname[0] != '\0') {
        origname = PyUnicode_FromString(info.origname);
        if (origname == NULL) {
            Py_DECREF(data);
            return NULL;
        }
    }

    PyObject *result = PyTuple_Pack(3, data ? data : Py_None,
                                    info.is_package ? Py_True : Py_False,
                                    origname ? origname : Py_None);
    Py_XDECREF(origname);
    Py_XDECREF(data);
    return result;
}

/*[clinic input]
_imp.get_frozen_object

    name: unicode
    data as dataobj: object = None
    /

Create a code object for a frozen module.
[clinic start generated code]*/

static PyObject *
_imp_get_frozen_object_impl(PyObject *module, PyObject *name,
                            PyObject *dataobj)
/*[clinic end generated code: output=54368a673a35e745 input=034bdb88f6460b7b]*/
{
    struct frozen_info info = {0};
    Py_buffer buf = {0};
    if (PyObject_CheckBuffer(dataobj)) {
        if (PyObject_GetBuffer(dataobj, &buf, PyBUF_SIMPLE) != 0) {
            return NULL;
        }
        info.data = (const char *)buf.buf;
        info.size = buf.len;
    }
    else if (dataobj != Py_None) {
        _PyArg_BadArgument("get_frozen_object", "argument 2", "bytes", dataobj);
        return NULL;
    }
    else {
        frozen_status status = find_frozen(name, &info);
        if (status != FROZEN_OKAY) {
            set_frozen_error(status, name);
            return NULL;
        }
    }

    if (info.nameobj == NULL) {
        info.nameobj = name;
    }
    if (info.size == 0) {
        /* Does not contain executable code. */
        set_frozen_error(FROZEN_INVALID, name);
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *codeobj = unmarshal_frozen_code(interp, &info);
    if (dataobj != Py_None) {
        PyBuffer_Release(&buf);
    }
    return codeobj;
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
    struct frozen_info info;
    frozen_status status = find_frozen(name, &info);
    if (status != FROZEN_OKAY && status != FROZEN_EXCLUDED) {
        set_frozen_error(status, name);
        return NULL;
    }
    return PyBool_FromLong(info.is_package);
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
    struct frozen_info info;
    frozen_status status = find_frozen(name, &info);
    if (status != FROZEN_OKAY) {
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

/*[clinic input]
_imp._frozen_module_names

Returns the list of available frozen modules.
[clinic start generated code]*/

static PyObject *
_imp__frozen_module_names_impl(PyObject *module)
/*[clinic end generated code: output=80609ef6256310a8 input=76237fbfa94460d2]*/
{
    return list_frozen_module_names();
}

/*[clinic input]
_imp._override_frozen_modules_for_tests

    override: int
    /

(internal-only) Override PyConfig.use_frozen_modules.

(-1: "off", 1: "on", 0: no override)
See frozen_modules() in Lib/test/support/import_helper.py.
[clinic start generated code]*/

static PyObject *
_imp__override_frozen_modules_for_tests_impl(PyObject *module, int override)
/*[clinic end generated code: output=36d5cb1594160811 input=8f1f95a3ef21aec3]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    OVERRIDE_FROZEN_MODULES(interp) = override;
    Py_RETURN_NONE;
}

/*[clinic input]
_imp._override_multi_interp_extensions_check

    override: int
    /

(internal-only) Override PyInterpreterConfig.check_multi_interp_extensions.

(-1: "never", 1: "always", 0: no override)
[clinic start generated code]*/

static PyObject *
_imp__override_multi_interp_extensions_check_impl(PyObject *module,
                                                  int override)
/*[clinic end generated code: output=3ff043af52bbf280 input=e086a2ea181f92ae]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (_Py_IsMainInterpreter(interp)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "_imp._override_multi_interp_extensions_check() "
                        "cannot be used in the main interpreter");
        return NULL;
    }
#ifdef Py_GIL_DISABLED
    PyErr_SetString(PyExc_RuntimeError,
                    "_imp._override_multi_interp_extensions_check() "
                    "cannot be used in the free-threaded build");
    return NULL;
#else
    int oldvalue = OVERRIDE_MULTI_INTERP_EXTENSIONS_CHECK(interp);
    OVERRIDE_MULTI_INTERP_EXTENSIONS_CHECK(interp) = override;
    return PyLong_FromLong(oldvalue);
#endif
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
    PyObject *mod = NULL;
    PyThreadState *tstate = _PyThreadState_GET();

    struct _Py_ext_module_loader_info info;
    if (_Py_ext_module_loader_info_init_from_spec(&info, spec) < 0) {
        return NULL;
    }

    mod = import_find_extension(tstate, &info);
    if (mod != NULL) {
        assert(!_PyErr_Occurred(tstate));
        assert_singlephase(_PyModule_GetDef(mod));
        goto finally;
    }
    else if (_PyErr_Occurred(tstate)) {
        goto finally;
    }
    /* Otherwise it must be multi-phase init or the first time it's loaded. */

    if (PySys_Audit("import", "OOOOO", info.name, info.filename,
                    Py_None, Py_None, Py_None) < 0)
    {
        goto finally;
    }

    /* We would move this (and the fclose() below) into
     * _PyImport_GetModInitFunc(), but it isn't clear if the intervening
     * code relies on fp still being open. */
    FILE *fp;
    if (file != NULL) {
        fp = _Py_fopen_obj(info.filename, "r");
        if (fp == NULL) {
            goto finally;
        }
    }
    else {
        fp = NULL;
    }

    PyModInitFunction p0 = _PyImport_GetModInitFunc(&info, fp);
    if (p0 == NULL) {
        goto finally;
    }

    mod = import_run_extension(
                    tstate, p0, &info, spec, get_modules_dict(tstate, true));
    if (mod == NULL) {
    }

    // XXX Shouldn't this happen in the error cases too (i.e. in "finally")?
    if (fp) {
        fclose(fp);
    }

finally:
    _Py_ext_module_loader_info_clear(&info);
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
"(Extremely) low-level import machinery bits as used by importlib.");

static PyMethodDef imp_methods[] = {
    _IMP_EXTENSION_SUFFIXES_METHODDEF
    _IMP_LOCK_HELD_METHODDEF
    _IMP_ACQUIRE_LOCK_METHODDEF
    _IMP_RELEASE_LOCK_METHODDEF
    _IMP_FIND_FROZEN_METHODDEF
    _IMP_GET_FROZEN_OBJECT_METHODDEF
    _IMP_IS_FROZEN_PACKAGE_METHODDEF
    _IMP_CREATE_BUILTIN_METHODDEF
    _IMP_INIT_FROZEN_METHODDEF
    _IMP_IS_BUILTIN_METHODDEF
    _IMP_IS_FROZEN_METHODDEF
    _IMP__FROZEN_MODULE_NAMES_METHODDEF
    _IMP__OVERRIDE_FROZEN_MODULES_FOR_TESTS_METHODDEF
    _IMP__OVERRIDE_MULTI_INTERP_EXTENSIONS_CHECK_METHODDEF
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
    if (PyModule_Add(module, "check_hash_based_pycs", pyc_mode) < 0) {
        return -1;
    }

    return 0;
}


static PyModuleDef_Slot imp_slots[] = {
    {Py_mod_exec, imp_module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
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
