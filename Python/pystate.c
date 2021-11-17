
/* Thread and interpreter state structures and their interfaces */

#include "Python.h"
#include "pycore_ceval.h"
#include "pycore_initconfig.h"
#include "pycore_object.h"        // _PyType_InitCache()
#include "pycore_pyerrors.h"
#include "pycore_pylifecycle.h"
#include "pycore_pymem.h"         // _PyMem_SetDefaultAllocator()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_sysmodule.h"

/* --------------------------------------------------------------------------
CAUTION

Always use PyMem_RawMalloc() and PyMem_RawFree() directly in this file.  A
number of these functions are advertised as safe to call when the GIL isn't
held, and in a debug build Python redirects (e.g.) PyMem_NEW (etc) to Python's
debugging obmalloc functions.  Those aren't thread-safe (they rely on the GIL
to avoid the expense of doing their own locking).
-------------------------------------------------------------------------- */

#ifdef HAVE_DLOPEN
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#if !HAVE_DECL_RTLD_LAZY
#define RTLD_LAZY 1
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define _PyRuntimeGILState_GetThreadState(gilstate) \
    ((PyThreadState*)_Py_atomic_load_relaxed(&(gilstate)->tstate_current))
#define _PyRuntimeGILState_SetThreadState(gilstate, value) \
    _Py_atomic_store_relaxed(&(gilstate)->tstate_current, \
                             (uintptr_t)(value))

/* Forward declarations */
static PyThreadState *_PyGILState_GetThisThreadState(struct _gilstate_runtime_state *gilstate);
static void _PyThreadState_Delete(PyThreadState *tstate, int check_current);


static PyStatus
_PyRuntimeState_Init_impl(_PyRuntimeState *runtime)
{
    /* We preserve the hook across init, because there is
       currently no public API to set it between runtime
       initialization and interpreter initialization. */
    void *open_code_hook = runtime->open_code_hook;
    void *open_code_userdata = runtime->open_code_userdata;
    _Py_AuditHookEntry *audit_hook_head = runtime->audit_hook_head;
    // bpo-42882: Preserve next_index value if Py_Initialize()/Py_Finalize()
    // is called multiple times.
    Py_ssize_t unicode_next_index = runtime->unicode_ids.next_index;

    memset(runtime, 0, sizeof(*runtime));

    runtime->open_code_hook = open_code_hook;
    runtime->open_code_userdata = open_code_userdata;
    runtime->audit_hook_head = audit_hook_head;

    _PyEval_InitRuntimeState(&runtime->ceval);

    PyPreConfig_InitPythonConfig(&runtime->preconfig);

    runtime->gilstate.check_enabled = 1;

    /* A TSS key must be initialized with Py_tss_NEEDS_INIT
       in accordance with the specification. */
    Py_tss_t initial = Py_tss_NEEDS_INIT;
    runtime->gilstate.autoTSSkey = initial;

    runtime->interpreters.mutex = PyThread_allocate_lock();
    if (runtime->interpreters.mutex == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    runtime->interpreters.next_id = -1;

    runtime->xidregistry.mutex = PyThread_allocate_lock();
    if (runtime->xidregistry.mutex == NULL) {
        return _PyStatus_NO_MEMORY();
    }

    // Set it to the ID of the main thread of the main interpreter.
    runtime->main_thread = PyThread_get_thread_ident();

    runtime->unicode_ids.lock = PyThread_allocate_lock();
    if (runtime->unicode_ids.lock == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    runtime->unicode_ids.next_index = unicode_next_index;

    return _PyStatus_OK();
}

PyStatus
_PyRuntimeState_Init(_PyRuntimeState *runtime)
{
    /* Force default allocator, since _PyRuntimeState_Fini() must
       use the same allocator than this function. */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    PyStatus status = _PyRuntimeState_Init_impl(runtime);

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return status;
}

void
_PyRuntimeState_Fini(_PyRuntimeState *runtime)
{
    /* Force the allocator used by _PyRuntimeState_Init(). */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
#define FREE_LOCK(LOCK) \
    if (LOCK != NULL) { \
        PyThread_free_lock(LOCK); \
        LOCK = NULL; \
    }

    FREE_LOCK(runtime->interpreters.mutex);
    FREE_LOCK(runtime->xidregistry.mutex);
    FREE_LOCK(runtime->unicode_ids.lock);

#undef FREE_LOCK
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}

#ifdef HAVE_FORK
/* This function is called from PyOS_AfterFork_Child to ensure that
   newly created child processes do not share locks with the parent. */
PyStatus
_PyRuntimeState_ReInitThreads(_PyRuntimeState *runtime)
{
    // This was initially set in _PyRuntimeState_Init().
    runtime->main_thread = PyThread_get_thread_ident();

    /* Force default allocator, since _PyRuntimeState_Fini() must
       use the same allocator than this function. */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    int reinit_interp = _PyThread_at_fork_reinit(&runtime->interpreters.mutex);
    int reinit_xidregistry = _PyThread_at_fork_reinit(&runtime->xidregistry.mutex);
    int reinit_unicode_ids = _PyThread_at_fork_reinit(&runtime->unicode_ids.lock);

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    /* bpo-42540: id_mutex is freed by _PyInterpreterState_Delete, which does
     * not force the default allocator. */
    int reinit_main_id = _PyThread_at_fork_reinit(&runtime->interpreters.main->id_mutex);

    if (reinit_interp < 0
        || reinit_main_id < 0
        || reinit_xidregistry < 0
        || reinit_unicode_ids < 0)
    {
        return _PyStatus_ERR("Failed to reinitialize runtime locks");

    }
    return _PyStatus_OK();
}
#endif

#define HEAD_LOCK(runtime) \
    PyThread_acquire_lock((runtime)->interpreters.mutex, WAIT_LOCK)
#define HEAD_UNLOCK(runtime) \
    PyThread_release_lock((runtime)->interpreters.mutex)

/* Forward declaration */
static void _PyGILState_NoteThreadState(
    struct _gilstate_runtime_state *gilstate, PyThreadState* tstate);

PyStatus
_PyInterpreterState_Enable(_PyRuntimeState *runtime)
{
    struct pyinterpreters *interpreters = &runtime->interpreters;
    interpreters->next_id = 0;

    /* Py_Finalize() calls _PyRuntimeState_Fini() which clears the mutex.
       Create a new mutex if needed. */
    if (interpreters->mutex == NULL) {
        /* Force default allocator, since _PyRuntimeState_Fini() must
           use the same allocator than this function. */
        PyMemAllocatorEx old_alloc;
        _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

        interpreters->mutex = PyThread_allocate_lock();

        PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

        if (interpreters->mutex == NULL) {
            return _PyStatus_ERR("Can't initialize threads for interpreter");
        }
    }

    return _PyStatus_OK();
}

PyInterpreterState *
PyInterpreterState_New(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    /* tstate is NULL when Py_InitializeFromConfig() calls
       PyInterpreterState_New() to create the main interpreter. */
    if (_PySys_Audit(tstate, "cpython.PyInterpreterState_New", NULL) < 0) {
        return NULL;
    }

    PyInterpreterState *interp = PyMem_RawCalloc(1, sizeof(PyInterpreterState));
    if (interp == NULL) {
        return NULL;
    }

    interp->id_refcount = -1;

    /* Don't get runtime from tstate since tstate can be NULL */
    _PyRuntimeState *runtime = &_PyRuntime;
    interp->runtime = runtime;

    if (_PyEval_InitState(&interp->ceval) < 0) {
        goto out_of_memory;
    }

    _PyGC_InitState(&interp->gc);
    PyConfig_InitPythonConfig(&interp->config);
    _PyType_InitCache(interp);

    interp->eval_frame = _PyEval_EvalFrameDefault;
#ifdef HAVE_DLOPEN
#if HAVE_DECL_RTLD_NOW
    interp->dlopenflags = RTLD_NOW;
#else
    interp->dlopenflags = RTLD_LAZY;
#endif
#endif

    struct pyinterpreters *interpreters = &runtime->interpreters;

    HEAD_LOCK(runtime);
    if (interpreters->next_id < 0) {
        /* overflow or Py_Initialize() not called! */
        if (tstate != NULL) {
            _PyErr_SetString(tstate, PyExc_RuntimeError,
                             "failed to get an interpreter ID");
        }
        PyMem_RawFree(interp);
        interp = NULL;
    }
    else {
        interp->id = interpreters->next_id;
        interpreters->next_id += 1;
        interp->next = interpreters->head;
        if (interpreters->main == NULL) {
            interpreters->main = interp;
        }
        interpreters->head = interp;
    }
    HEAD_UNLOCK(runtime);

    if (interp == NULL) {
        return NULL;
    }

    interp->tstate_next_unique_id = 0;

    interp->audit_hooks = NULL;

    return interp;

out_of_memory:
    if (tstate != NULL) {
        _PyErr_NoMemory(tstate);
    }

    PyMem_RawFree(interp);
    return NULL;
}


static void
interpreter_clear(PyInterpreterState *interp, PyThreadState *tstate)
{
    _PyRuntimeState *runtime = interp->runtime;

    if (_PySys_Audit(tstate, "cpython.PyInterpreterState_Clear", NULL) < 0) {
        _PyErr_Clear(tstate);
    }

    HEAD_LOCK(runtime);
    for (PyThreadState *p = interp->tstate_head; p != NULL; p = p->next) {
        PyThreadState_Clear(p);
    }
    HEAD_UNLOCK(runtime);

    Py_CLEAR(interp->audit_hooks);

    PyConfig_Clear(&interp->config);
    Py_CLEAR(interp->codec_search_path);
    Py_CLEAR(interp->codec_search_cache);
    Py_CLEAR(interp->codec_error_registry);
    Py_CLEAR(interp->modules);
    Py_CLEAR(interp->modules_by_index);
    Py_CLEAR(interp->builtins_copy);
    Py_CLEAR(interp->importlib);
    Py_CLEAR(interp->import_func);
    Py_CLEAR(interp->dict);
#ifdef HAVE_FORK
    Py_CLEAR(interp->before_forkers);
    Py_CLEAR(interp->after_forkers_parent);
    Py_CLEAR(interp->after_forkers_child);
#endif

    _PyAST_Fini(interp);
    _PyWarnings_Fini(interp);
    _PyAtExit_Fini(interp);

    // All Python types must be destroyed before the last GC collection. Python
    // types create a reference cycle to themselves in their in their
    // PyTypeObject.tp_mro member (the tuple contains the type).

    /* Last garbage collection on this interpreter */
    _PyGC_CollectNoFail(tstate);
    _PyGC_Fini(interp);

    /* We don't clear sysdict and builtins until the end of this function.
       Because clearing other attributes can execute arbitrary Python code
       which requires sysdict and builtins. */
    PyDict_Clear(interp->sysdict);
    PyDict_Clear(interp->builtins);
    Py_CLEAR(interp->sysdict);
    Py_CLEAR(interp->builtins);

    // XXX Once we have one allocator per interpreter (i.e.
    // per-interpreter GC) we must ensure that all of the interpreter's
    // objects have been cleaned up at the point.
}


void
PyInterpreterState_Clear(PyInterpreterState *interp)
{
    // Use the current Python thread state to call audit hooks and to collect
    // garbage. It can be different than the current Python thread state
    // of 'interp'.
    PyThreadState *current_tstate = _PyThreadState_GET();

    interpreter_clear(interp, current_tstate);
}


void
_PyInterpreterState_Clear(PyThreadState *tstate)
{
    interpreter_clear(tstate->interp, tstate);
}


static void
zapthreads(PyInterpreterState *interp, int check_current)
{
    PyThreadState *tstate;
    /* No need to lock the mutex here because this should only happen
       when the threads are all really dead (XXX famous last words). */
    while ((tstate = interp->tstate_head) != NULL) {
        _PyThreadState_Delete(tstate, check_current);
    }
}


void
PyInterpreterState_Delete(PyInterpreterState *interp)
{
    _PyRuntimeState *runtime = interp->runtime;
    struct pyinterpreters *interpreters = &runtime->interpreters;
    zapthreads(interp, 0);

    _PyEval_FiniState(&interp->ceval);

    /* Delete current thread. After this, many C API calls become crashy. */
    _PyThreadState_Swap(&runtime->gilstate, NULL);

    HEAD_LOCK(runtime);
    PyInterpreterState **p;
    for (p = &interpreters->head; ; p = &(*p)->next) {
        if (*p == NULL) {
            Py_FatalError("NULL interpreter");
        }
        if (*p == interp) {
            break;
        }
    }
    if (interp->tstate_head != NULL) {
        Py_FatalError("remaining threads");
    }
    *p = interp->next;

    if (interpreters->main == interp) {
        interpreters->main = NULL;
        if (interpreters->head != NULL) {
            Py_FatalError("remaining subinterpreters");
        }
    }
    HEAD_UNLOCK(runtime);

    if (interp->id_mutex != NULL) {
        PyThread_free_lock(interp->id_mutex);
    }
    PyMem_RawFree(interp);
}


#ifdef HAVE_FORK
/*
 * Delete all interpreter states except the main interpreter.  If there
 * is a current interpreter state, it *must* be the main interpreter.
 */
PyStatus
_PyInterpreterState_DeleteExceptMain(_PyRuntimeState *runtime)
{
    struct _gilstate_runtime_state *gilstate = &runtime->gilstate;
    struct pyinterpreters *interpreters = &runtime->interpreters;

    PyThreadState *tstate = _PyThreadState_Swap(gilstate, NULL);
    if (tstate != NULL && tstate->interp != interpreters->main) {
        return _PyStatus_ERR("not main interpreter");
    }

    HEAD_LOCK(runtime);
    PyInterpreterState *interp = interpreters->head;
    interpreters->head = NULL;
    while (interp != NULL) {
        if (interp == interpreters->main) {
            interpreters->main->next = NULL;
            interpreters->head = interp;
            interp = interp->next;
            continue;
        }

        PyInterpreterState_Clear(interp);  // XXX must activate?
        zapthreads(interp, 1);
        if (interp->id_mutex != NULL) {
            PyThread_free_lock(interp->id_mutex);
        }
        PyInterpreterState *prev_interp = interp;
        interp = interp->next;
        PyMem_RawFree(prev_interp);
    }
    HEAD_UNLOCK(runtime);

    if (interpreters->head == NULL) {
        return _PyStatus_ERR("missing main interpreter");
    }
    _PyThreadState_Swap(gilstate, tstate);
    return _PyStatus_OK();
}
#endif


PyInterpreterState *
PyInterpreterState_Get(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_EnsureTstateNotNULL(tstate);
    PyInterpreterState *interp = tstate->interp;
    if (interp == NULL) {
        Py_FatalError("no current interpreter");
    }
    return interp;
}


int64_t
PyInterpreterState_GetID(PyInterpreterState *interp)
{
    if (interp == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "no interpreter provided");
        return -1;
    }
    return interp->id;
}


static PyInterpreterState *
interp_look_up_id(_PyRuntimeState *runtime, int64_t requested_id)
{
    PyInterpreterState *interp = runtime->interpreters.head;
    while (interp != NULL) {
        int64_t id = PyInterpreterState_GetID(interp);
        if (id < 0) {
            return NULL;
        }
        if (requested_id == id) {
            return interp;
        }
        interp = PyInterpreterState_Next(interp);
    }
    return NULL;
}

PyInterpreterState *
_PyInterpreterState_LookUpID(int64_t requested_id)
{
    PyInterpreterState *interp = NULL;
    if (requested_id >= 0) {
        _PyRuntimeState *runtime = &_PyRuntime;
        HEAD_LOCK(runtime);
        interp = interp_look_up_id(runtime, requested_id);
        HEAD_UNLOCK(runtime);
    }
    if (interp == NULL && !PyErr_Occurred()) {
        PyErr_Format(PyExc_RuntimeError,
                     "unrecognized interpreter ID %lld", requested_id);
    }
    return interp;
}


int
_PyInterpreterState_IDInitref(PyInterpreterState *interp)
{
    if (interp->id_mutex != NULL) {
        return 0;
    }
    interp->id_mutex = PyThread_allocate_lock();
    if (interp->id_mutex == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "failed to create init interpreter ID mutex");
        return -1;
    }
    interp->id_refcount = 0;
    return 0;
}


int
_PyInterpreterState_IDIncref(PyInterpreterState *interp)
{
    if (_PyInterpreterState_IDInitref(interp) < 0) {
        return -1;
    }

    PyThread_acquire_lock(interp->id_mutex, WAIT_LOCK);
    interp->id_refcount += 1;
    PyThread_release_lock(interp->id_mutex);
    return 0;
}


void
_PyInterpreterState_IDDecref(PyInterpreterState *interp)
{
    assert(interp->id_mutex != NULL);

    struct _gilstate_runtime_state *gilstate = &_PyRuntime.gilstate;
    PyThread_acquire_lock(interp->id_mutex, WAIT_LOCK);
    assert(interp->id_refcount != 0);
    interp->id_refcount -= 1;
    int64_t refcount = interp->id_refcount;
    PyThread_release_lock(interp->id_mutex);

    if (refcount == 0 && interp->requires_idref) {
        // XXX Using the "head" thread isn't strictly correct.
        PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
        // XXX Possible GILState issues?
        PyThreadState *save_tstate = _PyThreadState_Swap(gilstate, tstate);
        Py_EndInterpreter(tstate);
        _PyThreadState_Swap(gilstate, save_tstate);
    }
}

int
_PyInterpreterState_RequiresIDRef(PyInterpreterState *interp)
{
    return interp->requires_idref;
}

void
_PyInterpreterState_RequireIDRef(PyInterpreterState *interp, int required)
{
    interp->requires_idref = required ? 1 : 0;
}

PyObject *
_PyInterpreterState_GetMainModule(PyInterpreterState *interp)
{
    if (interp->modules == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "interpreter not initialized");
        return NULL;
    }
    return PyMapping_GetItemString(interp->modules, "__main__");
}

PyObject *
PyInterpreterState_GetDict(PyInterpreterState *interp)
{
    if (interp->dict == NULL) {
        interp->dict = PyDict_New();
        if (interp->dict == NULL) {
            PyErr_Clear();
        }
    }
    /* Returning NULL means no per-interpreter dict is available. */
    return interp->dict;
}

static PyThreadState *
new_threadstate(PyInterpreterState *interp, int init)
{
    _PyRuntimeState *runtime = interp->runtime;
    PyThreadState *tstate = (PyThreadState *)PyMem_RawMalloc(sizeof(PyThreadState));
    if (tstate == NULL) {
        return NULL;
    }

    tstate->interp = interp;

    tstate->frame = NULL;
    tstate->recursion_depth = 0;
    tstate->recursion_headroom = 0;
    tstate->stackcheck_counter = 0;
    tstate->tracing = 0;
    tstate->root_cframe.use_tracing = 0;
    tstate->cframe = &tstate->root_cframe;
    tstate->gilstate_counter = 0;
    tstate->async_exc = NULL;
    tstate->thread_id = PyThread_get_thread_ident();

    tstate->dict = NULL;

    tstate->curexc_type = NULL;
    tstate->curexc_value = NULL;
    tstate->curexc_traceback = NULL;

    tstate->exc_state.exc_type = NULL;
    tstate->exc_state.exc_value = NULL;
    tstate->exc_state.exc_traceback = NULL;
    tstate->exc_state.previous_item = NULL;
    tstate->exc_info = &tstate->exc_state;

    tstate->c_profilefunc = NULL;
    tstate->c_tracefunc = NULL;
    tstate->c_profileobj = NULL;
    tstate->c_traceobj = NULL;

    tstate->trash_delete_nesting = 0;
    tstate->trash_delete_later = NULL;
    tstate->on_delete = NULL;
    tstate->on_delete_data = NULL;

    tstate->coroutine_origin_tracking_depth = 0;

    tstate->async_gen_firstiter = NULL;
    tstate->async_gen_finalizer = NULL;

    tstate->context = NULL;
    tstate->context_ver = 1;

    if (init) {
        _PyThreadState_Init(tstate);
    }

    HEAD_LOCK(runtime);
    tstate->id = ++interp->tstate_next_unique_id;
    tstate->prev = NULL;
    tstate->next = interp->tstate_head;
    if (tstate->next)
        tstate->next->prev = tstate;
    interp->tstate_head = tstate;
    HEAD_UNLOCK(runtime);

    return tstate;
}

PyThreadState *
PyThreadState_New(PyInterpreterState *interp)
{
    return new_threadstate(interp, 1);
}

PyThreadState *
_PyThreadState_Prealloc(PyInterpreterState *interp)
{
    return new_threadstate(interp, 0);
}

void
_PyThreadState_Init(PyThreadState *tstate)
{
    _PyGILState_NoteThreadState(&tstate->interp->runtime->gilstate, tstate);
}

PyObject*
PyState_FindModule(struct PyModuleDef* module)
{
    Py_ssize_t index = module->m_base.m_index;
    PyInterpreterState *state = _PyInterpreterState_GET();
    PyObject *res;
    if (module->m_slots) {
        return NULL;
    }
    if (index == 0)
        return NULL;
    if (state->modules_by_index == NULL)
        return NULL;
    if (index >= PyList_GET_SIZE(state->modules_by_index))
        return NULL;
    res = PyList_GET_ITEM(state->modules_by_index, index);
    return res==Py_None ? NULL : res;
}

int
_PyState_AddModule(PyThreadState *tstate, PyObject* module, struct PyModuleDef* def)
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

    PyInterpreterState *interp = tstate->interp;
    if (!interp->modules_by_index) {
        interp->modules_by_index = PyList_New(0);
        if (!interp->modules_by_index) {
            return -1;
        }
    }

    while (PyList_GET_SIZE(interp->modules_by_index) <= def->m_base.m_index) {
        if (PyList_Append(interp->modules_by_index, Py_None) < 0) {
            return -1;
        }
    }

    Py_INCREF(module);
    return PyList_SetItem(interp->modules_by_index,
                          def->m_base.m_index, module);
}

int
PyState_AddModule(PyObject* module, struct PyModuleDef* def)
{
    if (!def) {
        Py_FatalError("module definition is NULL");
        return -1;
    }

    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;
    Py_ssize_t index = def->m_base.m_index;
    if (interp->modules_by_index &&
        index < PyList_GET_SIZE(interp->modules_by_index) &&
        module == PyList_GET_ITEM(interp->modules_by_index, index))
    {
        _Py_FatalErrorFormat(__func__, "module %p already added", module);
        return -1;
    }
    return _PyState_AddModule(tstate, module, def);
}

int
PyState_RemoveModule(struct PyModuleDef* def)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;

    if (def->m_slots) {
        _PyErr_SetString(tstate,
                         PyExc_SystemError,
                         "PyState_RemoveModule called on module with slots");
        return -1;
    }

    Py_ssize_t index = def->m_base.m_index;
    if (index == 0) {
        Py_FatalError("invalid module index");
    }
    if (interp->modules_by_index == NULL) {
        Py_FatalError("Interpreters module-list not accessible.");
    }
    if (index > PyList_GET_SIZE(interp->modules_by_index)) {
        Py_FatalError("Module index out of bounds.");
    }

    Py_INCREF(Py_None);
    return PyList_SetItem(interp->modules_by_index, index, Py_None);
}

// Used by finalize_modules()
void
_PyInterpreterState_ClearModules(PyInterpreterState *interp)
{
    if (!interp->modules_by_index) {
        return;
    }

    Py_ssize_t i;
    for (i = 0; i < PyList_GET_SIZE(interp->modules_by_index); i++) {
        PyObject *m = PyList_GET_ITEM(interp->modules_by_index, i);
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
    if (PyList_SetSlice(interp->modules_by_index,
                        0, PyList_GET_SIZE(interp->modules_by_index),
                        NULL)) {
        PyErr_WriteUnraisable(interp->modules_by_index);
    }
}

void
PyThreadState_Clear(PyThreadState *tstate)
{
    int verbose = _PyInterpreterState_GetConfig(tstate->interp)->verbose;

    if (verbose && tstate->frame != NULL) {
        /* bpo-20526: After the main thread calls
           _PyRuntimeState_SetFinalizing() in Py_FinalizeEx(), threads must
           exit when trying to take the GIL. If a thread exit in the middle of
           _PyEval_EvalFrameDefault(), tstate->frame is not reset to its
           previous value. It is more likely with daemon threads, but it can
           happen with regular threads if threading._shutdown() fails
           (ex: interrupted by CTRL+C). */
        fprintf(stderr,
          "PyThreadState_Clear: warning: thread still has a frame\n");
    }

    /* Don't clear tstate->frame: it is a borrowed reference */

    Py_CLEAR(tstate->dict);
    Py_CLEAR(tstate->async_exc);

    Py_CLEAR(tstate->curexc_type);
    Py_CLEAR(tstate->curexc_value);
    Py_CLEAR(tstate->curexc_traceback);

    Py_CLEAR(tstate->exc_state.exc_type);
    Py_CLEAR(tstate->exc_state.exc_value);
    Py_CLEAR(tstate->exc_state.exc_traceback);

    /* The stack of exception states should contain just this thread. */
    if (verbose && tstate->exc_info != &tstate->exc_state) {
        fprintf(stderr,
          "PyThreadState_Clear: warning: thread still has a generator\n");
    }

    tstate->c_profilefunc = NULL;
    tstate->c_tracefunc = NULL;
    Py_CLEAR(tstate->c_profileobj);
    Py_CLEAR(tstate->c_traceobj);

    Py_CLEAR(tstate->async_gen_firstiter);
    Py_CLEAR(tstate->async_gen_finalizer);

    Py_CLEAR(tstate->context);

    if (tstate->on_delete != NULL) {
        tstate->on_delete(tstate->on_delete_data);
    }
}


/* Common code for PyThreadState_Delete() and PyThreadState_DeleteCurrent() */
static void
tstate_delete_common(PyThreadState *tstate,
                     struct _gilstate_runtime_state *gilstate)
{
    _Py_EnsureTstateNotNULL(tstate);
    PyInterpreterState *interp = tstate->interp;
    if (interp == NULL) {
        Py_FatalError("NULL interpreter");
    }
    _PyRuntimeState *runtime = interp->runtime;

    HEAD_LOCK(runtime);
    if (tstate->prev) {
        tstate->prev->next = tstate->next;
    }
    else {
        interp->tstate_head = tstate->next;
    }
    if (tstate->next) {
        tstate->next->prev = tstate->prev;
    }
    HEAD_UNLOCK(runtime);

    if (gilstate->autoInterpreterState &&
        PyThread_tss_get(&gilstate->autoTSSkey) == tstate)
    {
        PyThread_tss_set(&gilstate->autoTSSkey, NULL);
    }
}


static void
_PyThreadState_Delete(PyThreadState *tstate, int check_current)
{
    struct _gilstate_runtime_state *gilstate = &tstate->interp->runtime->gilstate;
    if (check_current) {
        if (tstate == _PyRuntimeGILState_GetThreadState(gilstate)) {
            _Py_FatalErrorFormat(__func__, "tstate %p is still current", tstate);
        }
    }
    tstate_delete_common(tstate, gilstate);
    PyMem_RawFree(tstate);
}


void
PyThreadState_Delete(PyThreadState *tstate)
{
    _PyThreadState_Delete(tstate, 1);
}


void
_PyThreadState_DeleteCurrent(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);
    struct _gilstate_runtime_state *gilstate = &tstate->interp->runtime->gilstate;
    tstate_delete_common(tstate, gilstate);
    _PyRuntimeGILState_SetThreadState(gilstate, NULL);
    _PyEval_ReleaseLock(tstate);
    PyMem_RawFree(tstate);
}

void
PyThreadState_DeleteCurrent(void)
{
    struct _gilstate_runtime_state *gilstate = &_PyRuntime.gilstate;
    PyThreadState *tstate = _PyRuntimeGILState_GetThreadState(gilstate);
    _PyThreadState_DeleteCurrent(tstate);
}


/*
 * Delete all thread states except the one passed as argument.
 * Note that, if there is a current thread state, it *must* be the one
 * passed as argument.  Also, this won't touch any other interpreters
 * than the current one, since we don't know which thread state should
 * be kept in those other interpreters.
 */
void
_PyThreadState_DeleteExcept(_PyRuntimeState *runtime, PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;

    HEAD_LOCK(runtime);
    /* Remove all thread states, except tstate, from the linked list of
       thread states.  This will allow calling PyThreadState_Clear()
       without holding the lock. */
    PyThreadState *list = interp->tstate_head;
    if (list == tstate) {
        list = tstate->next;
    }
    if (tstate->prev) {
        tstate->prev->next = tstate->next;
    }
    if (tstate->next) {
        tstate->next->prev = tstate->prev;
    }
    tstate->prev = tstate->next = NULL;
    interp->tstate_head = tstate;
    HEAD_UNLOCK(runtime);

    /* Clear and deallocate all stale thread states.  Even if this
       executes Python code, we should be safe since it executes
       in the current thread, not one of the stale threads. */
    PyThreadState *p, *next;
    for (p = list; p; p = next) {
        next = p->next;
        PyThreadState_Clear(p);
        PyMem_RawFree(p);
    }
}


#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
PyThreadState*
_PyThreadState_GetTSS(void) {
    return PyThread_tss_get(&_PyRuntime.gilstate.autoTSSkey);
}
#endif


PyThreadState *
_PyThreadState_UncheckedGet(void)
{
    return _PyThreadState_GET();
}


PyThreadState *
PyThreadState_Get(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_EnsureTstateNotNULL(tstate);
    return tstate;
}


PyThreadState *
_PyThreadState_Swap(struct _gilstate_runtime_state *gilstate, PyThreadState *newts)
{
#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    PyThreadState *oldts = _PyThreadState_GetTSS();
#else
    PyThreadState *oldts = _PyRuntimeGILState_GetThreadState(gilstate);
#endif

    _PyRuntimeGILState_SetThreadState(gilstate, newts);
    /* It should not be possible for more than one thread state
       to be used for a thread.  Check this the best we can in debug
       builds.
    */
#if defined(Py_DEBUG)
    if (newts) {
        /* This can be called from PyEval_RestoreThread(). Similar
           to it, we need to ensure errno doesn't change.
        */
        int err = errno;
        PyThreadState *check = _PyGILState_GetThisThreadState(gilstate);
        if (check && check->interp == newts->interp && check != newts)
            Py_FatalError("Invalid thread state for this thread");
        errno = err;
    }
#endif
#ifdef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    PyThread_tss_set(&gilstate->autoTSSkey, newts);
#endif
    return oldts;
}

PyThreadState *
PyThreadState_Swap(PyThreadState *newts)
{
    return _PyThreadState_Swap(&_PyRuntime.gilstate, newts);
}

/* An extension mechanism to store arbitrary additional per-thread state.
   PyThreadState_GetDict() returns a dictionary that can be used to hold such
   state; the caller should pick a unique key and store its state there.  If
   PyThreadState_GetDict() returns NULL, an exception has *not* been raised
   and the caller should assume no per-thread state is available. */

PyObject *
_PyThreadState_GetDict(PyThreadState *tstate)
{
    assert(tstate != NULL);
    if (tstate->dict == NULL) {
        tstate->dict = PyDict_New();
        if (tstate->dict == NULL) {
            _PyErr_Clear(tstate);
        }
    }
    return tstate->dict;
}


PyObject *
PyThreadState_GetDict(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        return NULL;
    }
    return _PyThreadState_GetDict(tstate);
}


PyInterpreterState *
PyThreadState_GetInterpreter(PyThreadState *tstate)
{
    assert(tstate != NULL);
    return tstate->interp;
}


PyFrameObject*
PyThreadState_GetFrame(PyThreadState *tstate)
{
    assert(tstate != NULL);
    PyFrameObject *frame = tstate->frame;
    Py_XINCREF(frame);
    return frame;
}


uint64_t
PyThreadState_GetID(PyThreadState *tstate)
{
    assert(tstate != NULL);
    return tstate->id;
}


/* Asynchronously raise an exception in a thread.
   Requested by Just van Rossum and Alex Martelli.
   To prevent naive misuse, you must write your own extension
   to call this, or use ctypes.  Must be called with the GIL held.
   Returns the number of tstates modified (normally 1, but 0 if `id` didn't
   match any known thread id).  Can be called with exc=NULL to clear an
   existing async exception.  This raises no exceptions. */

int
PyThreadState_SetAsyncExc(unsigned long id, PyObject *exc)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    PyInterpreterState *interp = _PyRuntimeState_GetThreadState(runtime)->interp;

    /* Although the GIL is held, a few C API functions can be called
     * without the GIL held, and in particular some that create and
     * destroy thread and interpreter states.  Those can mutate the
     * list of thread states we're traversing, so to prevent that we lock
     * head_mutex for the duration.
     */
    HEAD_LOCK(runtime);
    for (PyThreadState *tstate = interp->tstate_head; tstate != NULL; tstate = tstate->next) {
        if (tstate->thread_id != id) {
            continue;
        }

        /* Tricky:  we need to decref the current value
         * (if any) in tstate->async_exc, but that can in turn
         * allow arbitrary Python code to run, including
         * perhaps calls to this function.  To prevent
         * deadlock, we need to release head_mutex before
         * the decref.
         */
        PyObject *old_exc = tstate->async_exc;
        Py_XINCREF(exc);
        tstate->async_exc = exc;
        HEAD_UNLOCK(runtime);

        Py_XDECREF(old_exc);
        _PyEval_SignalAsyncExc(tstate->interp);
        return 1;
    }
    HEAD_UNLOCK(runtime);
    return 0;
}


/* Routines for advanced debuggers, requested by David Beazley.
   Don't use unless you know what you are doing! */

PyInterpreterState *
PyInterpreterState_Head(void)
{
    return _PyRuntime.interpreters.head;
}

PyInterpreterState *
PyInterpreterState_Main(void)
{
    return _PyRuntime.interpreters.main;
}

PyInterpreterState *
PyInterpreterState_Next(PyInterpreterState *interp) {
    return interp->next;
}

PyThreadState *
PyInterpreterState_ThreadHead(PyInterpreterState *interp) {
    return interp->tstate_head;
}

PyThreadState *
PyThreadState_Next(PyThreadState *tstate) {
    return tstate->next;
}

/* The implementation of sys._current_frames().  This is intended to be
   called with the GIL held, as it will be when called via
   sys._current_frames().  It's possible it would work fine even without
   the GIL held, but haven't thought enough about that.
*/
PyObject *
_PyThread_CurrentFrames(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PySys_Audit(tstate, "sys._current_frames", NULL) < 0) {
        return NULL;
    }

    PyObject *result = PyDict_New();
    if (result == NULL) {
        return NULL;
    }

    /* for i in all interpreters:
     *     for t in all of i's thread states:
     *          if t's frame isn't NULL, map t's id to its frame
     * Because these lists can mutate even when the GIL is held, we
     * need to grab head_mutex for the duration.
     */
    _PyRuntimeState *runtime = tstate->interp->runtime;
    HEAD_LOCK(runtime);
    PyInterpreterState *i;
    for (i = runtime->interpreters.head; i != NULL; i = i->next) {
        PyThreadState *t;
        for (t = i->tstate_head; t != NULL; t = t->next) {
            PyFrameObject *frame = t->frame;
            if (frame == NULL) {
                continue;
            }
            PyObject *id = PyLong_FromUnsignedLong(t->thread_id);
            if (id == NULL) {
                goto fail;
            }
            int stat = PyDict_SetItem(result, id, (PyObject *)frame);
            Py_DECREF(id);
            if (stat < 0) {
                goto fail;
            }
        }
    }
    goto done;

fail:
    Py_CLEAR(result);

done:
    HEAD_UNLOCK(runtime);
    return result;
}

PyObject *
_PyThread_CurrentExceptions(void)
{
    PyThreadState *tstate = _PyThreadState_GET();

    _Py_EnsureTstateNotNULL(tstate);

    if (_PySys_Audit(tstate, "sys._current_exceptions", NULL) < 0) {
        return NULL;
    }

    PyObject *result = PyDict_New();
    if (result == NULL) {
        return NULL;
    }

    /* for i in all interpreters:
     *     for t in all of i's thread states:
     *          if t's frame isn't NULL, map t's id to its frame
     * Because these lists can mutate even when the GIL is held, we
     * need to grab head_mutex for the duration.
     */
    _PyRuntimeState *runtime = tstate->interp->runtime;
    HEAD_LOCK(runtime);
    PyInterpreterState *i;
    for (i = runtime->interpreters.head; i != NULL; i = i->next) {
        PyThreadState *t;
        for (t = i->tstate_head; t != NULL; t = t->next) {
            _PyErr_StackItem *err_info = _PyErr_GetTopmostException(t);
            if (err_info == NULL) {
                continue;
            }
            PyObject *id = PyLong_FromUnsignedLong(t->thread_id);
            if (id == NULL) {
                goto fail;
            }
            PyObject *exc_info = PyTuple_Pack(
                3,
                err_info->exc_type != NULL ? err_info->exc_type : Py_None,
                err_info->exc_value != NULL ? err_info->exc_value : Py_None,
                err_info->exc_traceback != NULL ? err_info->exc_traceback : Py_None);
            if (exc_info == NULL) {
                Py_DECREF(id);
                goto fail;
            }
            int stat = PyDict_SetItem(result, id, exc_info);
            Py_DECREF(id);
            Py_DECREF(exc_info);
            if (stat < 0) {
                goto fail;
            }
        }
    }
    goto done;

fail:
    Py_CLEAR(result);

done:
    HEAD_UNLOCK(runtime);
    return result;
}

/* Python "auto thread state" API. */

/* Keep this as a static, as it is not reliable!  It can only
   ever be compared to the state for the *current* thread.
   * If not equal, then it doesn't matter that the actual
     value may change immediately after comparison, as it can't
     possibly change to the current thread's state.
   * If equal, then the current thread holds the lock, so the value can't
     change until we yield the lock.
*/
static int
PyThreadState_IsCurrent(PyThreadState *tstate)
{
    /* Must be the tstate for this thread */
    struct _gilstate_runtime_state *gilstate = &_PyRuntime.gilstate;
    assert(_PyGILState_GetThisThreadState(gilstate) == tstate);
    return tstate == _PyRuntimeGILState_GetThreadState(gilstate);
}

/* Internal initialization/finalization functions called by
   Py_Initialize/Py_FinalizeEx
*/
PyStatus
_PyGILState_Init(_PyRuntimeState *runtime)
{
    struct _gilstate_runtime_state *gilstate = &runtime->gilstate;
    if (PyThread_tss_create(&gilstate->autoTSSkey) != 0) {
        return _PyStatus_NO_MEMORY();
    }
    // PyThreadState_New() calls _PyGILState_NoteThreadState() which does
    // nothing before autoInterpreterState is set.
    assert(gilstate->autoInterpreterState == NULL);
    return _PyStatus_OK();
}


PyStatus
_PyGILState_SetTstate(PyThreadState *tstate)
{
    if (!_Py_IsMainInterpreter(tstate->interp)) {
        /* Currently, PyGILState is shared by all interpreters. The main
         * interpreter is responsible to initialize it. */
        return _PyStatus_OK();
    }

    /* must init with valid states */
    assert(tstate != NULL);
    assert(tstate->interp != NULL);

    struct _gilstate_runtime_state *gilstate = &tstate->interp->runtime->gilstate;

    gilstate->autoInterpreterState = tstate->interp;
    assert(PyThread_tss_get(&gilstate->autoTSSkey) == NULL);
    assert(tstate->gilstate_counter == 0);

    _PyGILState_NoteThreadState(gilstate, tstate);
    return _PyStatus_OK();
}

PyInterpreterState *
_PyGILState_GetInterpreterStateUnsafe(void)
{
    return _PyRuntime.gilstate.autoInterpreterState;
}

void
_PyGILState_Fini(PyInterpreterState *interp)
{
    struct _gilstate_runtime_state *gilstate = &interp->runtime->gilstate;
    PyThread_tss_delete(&gilstate->autoTSSkey);
    gilstate->autoInterpreterState = NULL;
}

#ifdef HAVE_FORK
/* Reset the TSS key - called by PyOS_AfterFork_Child().
 * This should not be necessary, but some - buggy - pthread implementations
 * don't reset TSS upon fork(), see issue #10517.
 */
PyStatus
_PyGILState_Reinit(_PyRuntimeState *runtime)
{
    struct _gilstate_runtime_state *gilstate = &runtime->gilstate;
    PyThreadState *tstate = _PyGILState_GetThisThreadState(gilstate);

    PyThread_tss_delete(&gilstate->autoTSSkey);
    if (PyThread_tss_create(&gilstate->autoTSSkey) != 0) {
        return _PyStatus_NO_MEMORY();
    }

    /* If the thread had an associated auto thread state, reassociate it with
     * the new key. */
    if (tstate &&
        PyThread_tss_set(&gilstate->autoTSSkey, (void *)tstate) != 0)
    {
        return _PyStatus_ERR("failed to set autoTSSkey");
    }
    return _PyStatus_OK();
}
#endif

/* When a thread state is created for a thread by some mechanism other than
   PyGILState_Ensure, it's important that the GILState machinery knows about
   it so it doesn't try to create another thread state for the thread (this is
   a better fix for SF bug #1010677 than the first one attempted).
*/
static void
_PyGILState_NoteThreadState(struct _gilstate_runtime_state *gilstate, PyThreadState* tstate)
{
    /* If autoTSSkey isn't initialized, this must be the very first
       threadstate created in Py_Initialize().  Don't do anything for now
       (we'll be back here when _PyGILState_Init is called). */
    if (!gilstate->autoInterpreterState) {
        return;
    }

    /* Stick the thread state for this thread in thread specific storage.

       The only situation where you can legitimately have more than one
       thread state for an OS level thread is when there are multiple
       interpreters.

       You shouldn't really be using the PyGILState_ APIs anyway (see issues
       #10915 and #15751).

       The first thread state created for that given OS level thread will
       "win", which seems reasonable behaviour.
    */
    if (PyThread_tss_get(&gilstate->autoTSSkey) == NULL) {
        if ((PyThread_tss_set(&gilstate->autoTSSkey, (void *)tstate)) != 0) {
            Py_FatalError("Couldn't create autoTSSkey mapping");
        }
    }

    /* PyGILState_Release must not try to delete this thread state. */
    tstate->gilstate_counter = 1;
}

/* The public functions */
static PyThreadState *
_PyGILState_GetThisThreadState(struct _gilstate_runtime_state *gilstate)
{
    if (gilstate->autoInterpreterState == NULL)
        return NULL;
    return (PyThreadState *)PyThread_tss_get(&gilstate->autoTSSkey);
}

PyThreadState *
PyGILState_GetThisThreadState(void)
{
    return _PyGILState_GetThisThreadState(&_PyRuntime.gilstate);
}

int
PyGILState_Check(void)
{
    struct _gilstate_runtime_state *gilstate = &_PyRuntime.gilstate;
    if (!gilstate->check_enabled) {
        return 1;
    }

    if (!PyThread_tss_is_created(&gilstate->autoTSSkey)) {
        return 1;
    }

    PyThreadState *tstate = _PyRuntimeGILState_GetThreadState(gilstate);
    if (tstate == NULL) {
        return 0;
    }

    return (tstate == _PyGILState_GetThisThreadState(gilstate));
}

PyGILState_STATE
PyGILState_Ensure(void)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    struct _gilstate_runtime_state *gilstate = &runtime->gilstate;

    /* Note that we do not auto-init Python here - apart from
       potential races with 2 threads auto-initializing, pep-311
       spells out other issues.  Embedders are expected to have
       called Py_Initialize(). */

    /* Ensure that _PyEval_InitThreads() and _PyGILState_Init() have been
       called by Py_Initialize() */
#ifndef EXPERIMENTAL_ISOLATED_SUBINTERPRETERS
    assert(_PyEval_ThreadsInitialized(runtime));
#endif
    assert(gilstate->autoInterpreterState);

    PyThreadState *tcur = (PyThreadState *)PyThread_tss_get(&gilstate->autoTSSkey);
    int current;
    if (tcur == NULL) {
        /* Create a new Python thread state for this thread */
        tcur = PyThreadState_New(gilstate->autoInterpreterState);
        if (tcur == NULL) {
            Py_FatalError("Couldn't create thread-state for new thread");
        }

        /* This is our thread state!  We'll need to delete it in the
           matching call to PyGILState_Release(). */
        tcur->gilstate_counter = 0;
        current = 0; /* new thread state is never current */
    }
    else {
        current = PyThreadState_IsCurrent(tcur);
    }

    if (current == 0) {
        PyEval_RestoreThread(tcur);
    }

    /* Update our counter in the thread-state - no need for locks:
       - tcur will remain valid as we hold the GIL.
       - the counter is safe as we are the only thread "allowed"
         to modify this value
    */
    ++tcur->gilstate_counter;

    return current ? PyGILState_LOCKED : PyGILState_UNLOCKED;
}

void
PyGILState_Release(PyGILState_STATE oldstate)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    PyThreadState *tstate = PyThread_tss_get(&runtime->gilstate.autoTSSkey);
    if (tstate == NULL) {
        Py_FatalError("auto-releasing thread-state, "
                      "but no thread-state for this thread");
    }

    /* We must hold the GIL and have our thread state current */
    /* XXX - remove the check - the assert should be fine,
       but while this is very new (April 2003), the extra check
       by release-only users can't hurt.
    */
    if (!PyThreadState_IsCurrent(tstate)) {
        _Py_FatalErrorFormat(__func__,
                             "thread state %p must be current when releasing",
                             tstate);
    }
    assert(PyThreadState_IsCurrent(tstate));
    --tstate->gilstate_counter;
    assert(tstate->gilstate_counter >= 0); /* illegal counter value */

    /* If we're going to destroy this thread-state, we must
     * clear it while the GIL is held, as destructors may run.
     */
    if (tstate->gilstate_counter == 0) {
        /* can't have been locked when we created it */
        assert(oldstate == PyGILState_UNLOCKED);
        PyThreadState_Clear(tstate);
        /* Delete the thread-state.  Note this releases the GIL too!
         * It's vital that the GIL be held here, to avoid shutdown
         * races; see bugs 225673 and 1061968 (that nasty bug has a
         * habit of coming back).
         */
        assert(_PyRuntimeGILState_GetThreadState(&runtime->gilstate) == tstate);
        _PyThreadState_DeleteCurrent(tstate);
    }
    /* Release the lock if necessary */
    else if (oldstate == PyGILState_UNLOCKED)
        PyEval_SaveThread();
}


/**************************/
/* cross-interpreter data */
/**************************/

/* cross-interpreter data */

crossinterpdatafunc _PyCrossInterpreterData_Lookup(PyObject *);

/* This is a separate func from _PyCrossInterpreterData_Lookup in order
   to keep the registry code separate. */
static crossinterpdatafunc
_lookup_getdata(PyObject *obj)
{
    crossinterpdatafunc getdata = _PyCrossInterpreterData_Lookup(obj);
    if (getdata == NULL && PyErr_Occurred() == 0)
        PyErr_Format(PyExc_ValueError,
                     "%S does not support cross-interpreter data", obj);
    return getdata;
}

int
_PyObject_CheckCrossInterpreterData(PyObject *obj)
{
    crossinterpdatafunc getdata = _lookup_getdata(obj);
    if (getdata == NULL) {
        return -1;
    }
    return 0;
}

static int
_check_xidata(PyThreadState *tstate, _PyCrossInterpreterData *data)
{
    // data->data can be anything, including NULL, so we don't check it.

    // data->obj may be NULL, so we don't check it.

    if (data->interp < 0) {
        _PyErr_SetString(tstate, PyExc_SystemError, "missing interp");
        return -1;
    }

    if (data->new_object == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "missing new_object func");
        return -1;
    }

    // data->free may be NULL, so we don't check it.

    return 0;
}

int
_PyObject_GetCrossInterpreterData(PyObject *obj, _PyCrossInterpreterData *data)
{
    // PyThreadState_Get() aborts if tstate is NULL.
    PyThreadState *tstate = PyThreadState_Get();
    PyInterpreterState *interp = tstate->interp;

    // Reset data before re-populating.
    *data = (_PyCrossInterpreterData){0};
    data->free = PyMem_RawFree;  // Set a default that may be overridden.

    // Call the "getdata" func for the object.
    Py_INCREF(obj);
    crossinterpdatafunc getdata = _lookup_getdata(obj);
    if (getdata == NULL) {
        Py_DECREF(obj);
        return -1;
    }
    int res = getdata(obj, data);
    Py_DECREF(obj);
    if (res != 0) {
        return -1;
    }

    // Fill in the blanks and validate the result.
    data->interp = interp->id;
    if (_check_xidata(tstate, data) != 0) {
        _PyCrossInterpreterData_Release(data);
        return -1;
    }

    return 0;
}

static void
_release_xidata(void *arg)
{
    _PyCrossInterpreterData *data = (_PyCrossInterpreterData *)arg;
    if (data->free != NULL) {
        data->free(data->data);
    }
    Py_XDECREF(data->obj);
}

static void
_call_in_interpreter(struct _gilstate_runtime_state *gilstate,
                     PyInterpreterState *interp,
                     void (*func)(void *), void *arg)
{
    /* We would use Py_AddPendingCall() if it weren't specific to the
     * main interpreter (see bpo-33608).  In the meantime we take a
     * naive approach.
     */
    PyThreadState *save_tstate = NULL;
    if (interp != _PyRuntimeGILState_GetThreadState(gilstate)->interp) {
        // XXX Using the "head" thread isn't strictly correct.
        PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
        // XXX Possible GILState issues?
        save_tstate = _PyThreadState_Swap(gilstate, tstate);
    }

    func(arg);

    // Switch back.
    if (save_tstate != NULL) {
        _PyThreadState_Swap(gilstate, save_tstate);
    }
}

void
_PyCrossInterpreterData_Release(_PyCrossInterpreterData *data)
{
    if (data->data == NULL && data->obj == NULL) {
        // Nothing to release!
        return;
    }

    // Switch to the original interpreter.
    PyInterpreterState *interp = _PyInterpreterState_LookUpID(data->interp);
    if (interp == NULL) {
        // The interpreter was already destroyed.
        if (data->free != NULL) {
            // XXX Someone leaked some memory...
        }
        return;
    }

    // "Release" the data and/or the object.
    struct _gilstate_runtime_state *gilstate = &_PyRuntime.gilstate;
    _call_in_interpreter(gilstate, interp, _release_xidata, data);
}

PyObject *
_PyCrossInterpreterData_NewObject(_PyCrossInterpreterData *data)
{
    return data->new_object(data);
}

/* registry of {type -> crossinterpdatafunc} */

/* For now we use a global registry of shareable classes.  An
   alternative would be to add a tp_* slot for a class's
   crossinterpdatafunc. It would be simpler and more efficient. */

static int
_register_xidata(struct _xidregistry *xidregistry, PyTypeObject *cls,
                 crossinterpdatafunc getdata)
{
    // Note that we effectively replace already registered classes
    // rather than failing.
    struct _xidregitem *newhead = PyMem_RawMalloc(sizeof(struct _xidregitem));
    if (newhead == NULL)
        return -1;
    newhead->cls = cls;
    newhead->getdata = getdata;
    newhead->next = xidregistry->head;
    xidregistry->head = newhead;
    return 0;
}

static void _register_builtins_for_crossinterpreter_data(struct _xidregistry *xidregistry);

int
_PyCrossInterpreterData_RegisterClass(PyTypeObject *cls,
                                       crossinterpdatafunc getdata)
{
    if (!PyType_Check(cls)) {
        PyErr_Format(PyExc_ValueError, "only classes may be registered");
        return -1;
    }
    if (getdata == NULL) {
        PyErr_Format(PyExc_ValueError, "missing 'getdata' func");
        return -1;
    }

    // Make sure the class isn't ever deallocated.
    Py_INCREF((PyObject *)cls);

    struct _xidregistry *xidregistry = &_PyRuntime.xidregistry ;
    PyThread_acquire_lock(xidregistry->mutex, WAIT_LOCK);
    if (xidregistry->head == NULL) {
        _register_builtins_for_crossinterpreter_data(xidregistry);
    }
    int res = _register_xidata(xidregistry, cls, getdata);
    PyThread_release_lock(xidregistry->mutex);
    return res;
}

/* Cross-interpreter objects are looked up by exact match on the class.
   We can reassess this policy when we move from a global registry to a
   tp_* slot. */

crossinterpdatafunc
_PyCrossInterpreterData_Lookup(PyObject *obj)
{
    struct _xidregistry *xidregistry = &_PyRuntime.xidregistry ;
    PyObject *cls = PyObject_Type(obj);
    crossinterpdatafunc getdata = NULL;
    PyThread_acquire_lock(xidregistry->mutex, WAIT_LOCK);
    struct _xidregitem *cur = xidregistry->head;
    if (cur == NULL) {
        _register_builtins_for_crossinterpreter_data(xidregistry);
        cur = xidregistry->head;
    }
    for(; cur != NULL; cur = cur->next) {
        if (cur->cls == (PyTypeObject *)cls) {
            getdata = cur->getdata;
            break;
        }
    }
    Py_DECREF(cls);
    PyThread_release_lock(xidregistry->mutex);
    return getdata;
}

/* cross-interpreter data for builtin types */

struct _shared_bytes_data {
    char *bytes;
    Py_ssize_t len;
};

static PyObject *
_new_bytes_object(_PyCrossInterpreterData *data)
{
    struct _shared_bytes_data *shared = (struct _shared_bytes_data *)(data->data);
    return PyBytes_FromStringAndSize(shared->bytes, shared->len);
}

static int
_bytes_shared(PyObject *obj, _PyCrossInterpreterData *data)
{
    struct _shared_bytes_data *shared = PyMem_NEW(struct _shared_bytes_data, 1);
    if (PyBytes_AsStringAndSize(obj, &shared->bytes, &shared->len) < 0) {
        return -1;
    }
    data->data = (void *)shared;
    Py_INCREF(obj);
    data->obj = obj;  // Will be "released" (decref'ed) when data released.
    data->new_object = _new_bytes_object;
    data->free = PyMem_Free;
    return 0;
}

struct _shared_str_data {
    int kind;
    const void *buffer;
    Py_ssize_t len;
};

static PyObject *
_new_str_object(_PyCrossInterpreterData *data)
{
    struct _shared_str_data *shared = (struct _shared_str_data *)(data->data);
    return PyUnicode_FromKindAndData(shared->kind, shared->buffer, shared->len);
}

static int
_str_shared(PyObject *obj, _PyCrossInterpreterData *data)
{
    struct _shared_str_data *shared = PyMem_NEW(struct _shared_str_data, 1);
    shared->kind = PyUnicode_KIND(obj);
    shared->buffer = PyUnicode_DATA(obj);
    shared->len = PyUnicode_GET_LENGTH(obj);
    data->data = (void *)shared;
    Py_INCREF(obj);
    data->obj = obj;  // Will be "released" (decref'ed) when data released.
    data->new_object = _new_str_object;
    data->free = PyMem_Free;
    return 0;
}

static PyObject *
_new_long_object(_PyCrossInterpreterData *data)
{
    return PyLong_FromSsize_t((Py_ssize_t)(data->data));
}

static int
_long_shared(PyObject *obj, _PyCrossInterpreterData *data)
{
    /* Note that this means the size of shareable ints is bounded by
     * sys.maxsize.  Hence on 32-bit architectures that is half the
     * size of maximum shareable ints on 64-bit.
     */
    Py_ssize_t value = PyLong_AsSsize_t(obj);
    if (value == -1 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            PyErr_SetString(PyExc_OverflowError, "try sending as bytes");
        }
        return -1;
    }
    data->data = (void *)value;
    data->obj = NULL;
    data->new_object = _new_long_object;
    data->free = NULL;
    return 0;
}

static PyObject *
_new_none_object(_PyCrossInterpreterData *data)
{
    // XXX Singleton refcounts are problematic across interpreters...
    Py_INCREF(Py_None);
    return Py_None;
}

static int
_none_shared(PyObject *obj, _PyCrossInterpreterData *data)
{
    data->data = NULL;
    // data->obj remains NULL
    data->new_object = _new_none_object;
    data->free = NULL;  // There is nothing to free.
    return 0;
}

static void
_register_builtins_for_crossinterpreter_data(struct _xidregistry *xidregistry)
{
    // None
    if (_register_xidata(xidregistry, (PyTypeObject *)PyObject_Type(Py_None), _none_shared) != 0) {
        Py_FatalError("could not register None for cross-interpreter sharing");
    }

    // int
    if (_register_xidata(xidregistry, &PyLong_Type, _long_shared) != 0) {
        Py_FatalError("could not register int for cross-interpreter sharing");
    }

    // bytes
    if (_register_xidata(xidregistry, &PyBytes_Type, _bytes_shared) != 0) {
        Py_FatalError("could not register bytes for cross-interpreter sharing");
    }

    // str
    if (_register_xidata(xidregistry, &PyUnicode_Type, _str_shared) != 0) {
        Py_FatalError("could not register str for cross-interpreter sharing");
    }
}


_PyFrameEvalFunction
_PyInterpreterState_GetEvalFrameFunc(PyInterpreterState *interp)
{
    return interp->eval_frame;
}


void
_PyInterpreterState_SetEvalFrameFunc(PyInterpreterState *interp,
                                     _PyFrameEvalFunction eval_frame)
{
    interp->eval_frame = eval_frame;
}


const PyConfig*
_PyInterpreterState_GetConfig(PyInterpreterState *interp)
{
    return &interp->config;
}


int
_PyInterpreterState_GetConfigCopy(PyConfig *config)
{
    PyInterpreterState *interp = PyInterpreterState_Get();

    PyStatus status = _PyConfig_Copy(config, &interp->config);
    if (PyStatus_Exception(status)) {
        _PyErr_SetFromPyStatus(status);
        return -1;
    }
    return 0;
}


const PyConfig*
_Py_GetConfig(void)
{
    assert(PyGILState_Check());
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyInterpreterState_GetConfig(tstate->interp);
}

#ifdef __cplusplus
}
#endif
