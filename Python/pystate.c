
/* Thread and interpreter state structures and their interfaces */

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_audit.h"         // _Py_AuditHookEntry
#include "pycore_ceval.h"         // _PyEval_AcquireLock()
#include "pycore_codecs.h"        // _PyCodec_Fini()
#include "pycore_critical_section.h" // _PyCriticalSection_Resume()
#include "pycore_dtoa.h"          // _dtoa_state_INIT()
#include "pycore_emscripten_trampoline.h" // _Py_EmscriptenTrampoline_Init()
#include "pycore_freelist.h"      // _PyObject_ClearFreeLists()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interpframe.h"   // _PyThreadState_HasStackSpace()
#include "pycore_object.h"        // _PyType_InitCache()
#include "pycore_obmalloc.h"      // _PyMem_obmalloc_state_on_heap()
#include "pycore_optimizer.h"     // JIT_CLEANUP_THRESHOLD
#include "pycore_parking_lot.h"   // _PyParkingLot_AfterFork()
#include "pycore_pyerrors.h"      // _PyErr_Clear()
#include "pycore_pylifecycle.h"   // _PyAST_Fini()
#include "pycore_pymem.h"         // _PyMem_DebugEnabled()
#include "pycore_runtime.h"       // _PyRuntime
#include "pycore_runtime_init.h"  // _PyRuntimeState_INIT
#include "pycore_stackref.h"      // Py_STACKREF_DEBUG
#include "pycore_time.h"          // _PyTime_Init()
#include "pycore_uniqueid.h"      // _PyObject_FinalizePerThreadRefcounts()


/* --------------------------------------------------------------------------
CAUTION

Always use PyMem_RawMalloc() and PyMem_RawFree() directly in this file.  A
number of these functions are advertised as safe to call when the GIL isn't
held, and in a debug build Python redirects (e.g.) PyMem_NEW (etc) to Python's
debugging obmalloc functions.  Those aren't thread-safe (they rely on the GIL
to avoid the expense of doing their own locking).
-------------------------------------------------------------------------- */

#ifdef HAVE_DLOPEN
#  ifdef HAVE_DLFCN_H
#    include <dlfcn.h>
#  endif
#  if !HAVE_DECL_RTLD_LAZY
#    define RTLD_LAZY 1
#  endif
#endif


/****************************************/
/* helpers for the current thread state */
/****************************************/

// API for the current thread state is further down.

/* "current" means one of:
   - bound to the current OS thread
   - holds the GIL
 */

//-------------------------------------------------
// a highly efficient lookup for the current thread
//-------------------------------------------------

/*
   The stored thread state is set by PyThreadState_Swap().

   For each of these functions, the GIL must be held by the current thread.
 */


#ifdef HAVE_THREAD_LOCAL
/* The attached thread state for the current thread. */
_Py_thread_local PyThreadState *_Py_tss_tstate = NULL;

/* The "bound" thread state used by PyGILState_Ensure(),
   also known as a "gilstate." */
_Py_thread_local PyThreadState *_Py_tss_gilstate = NULL;
#endif

static inline PyThreadState *
current_fast_get(void)
{
#ifdef HAVE_THREAD_LOCAL
    return _Py_tss_tstate;
#else
    // XXX Fall back to the PyThread_tss_*() API.
#  error "no supported thread-local variable storage classifier"
#endif
}

static inline void
current_fast_set(_PyRuntimeState *Py_UNUSED(runtime), PyThreadState *tstate)
{
    assert(tstate != NULL);
#ifdef HAVE_THREAD_LOCAL
    _Py_tss_tstate = tstate;
#else
    // XXX Fall back to the PyThread_tss_*() API.
#  error "no supported thread-local variable storage classifier"
#endif
}

static inline void
current_fast_clear(_PyRuntimeState *Py_UNUSED(runtime))
{
#ifdef HAVE_THREAD_LOCAL
    _Py_tss_tstate = NULL;
#else
    // XXX Fall back to the PyThread_tss_*() API.
#  error "no supported thread-local variable storage classifier"
#endif
}

#define tstate_verify_not_active(tstate) \
    if (tstate == current_fast_get()) { \
        _Py_FatalErrorFormat(__func__, "tstate %p is still current", tstate); \
    }

PyThreadState *
_PyThreadState_GetCurrent(void)
{
    return current_fast_get();
}


//---------------------------------------------
// The thread state used by PyGILState_Ensure()
//---------------------------------------------

/*
   The stored thread state is set by bind_tstate() (AKA PyThreadState_Bind().

   The GIL does no need to be held for these.
  */

static inline PyThreadState *
gilstate_get(void)
{
    return _Py_tss_gilstate;
}

static inline void
gilstate_set(PyThreadState *tstate)
{
    assert(tstate != NULL);
    _Py_tss_gilstate = tstate;
}

static inline void
gilstate_clear(void)
{
    _Py_tss_gilstate = NULL;
}


#ifndef NDEBUG
static inline int tstate_is_alive(PyThreadState *tstate);

static inline int
tstate_is_bound(PyThreadState *tstate)
{
    return tstate->_status.bound && !tstate->_status.unbound;
}
#endif  // !NDEBUG

static void bind_gilstate_tstate(PyThreadState *);
static void unbind_gilstate_tstate(PyThreadState *);

static void tstate_mimalloc_bind(PyThreadState *);

static void
bind_tstate(PyThreadState *tstate)
{
    assert(tstate != NULL);
    assert(tstate_is_alive(tstate) && !tstate->_status.bound);
    assert(!tstate->_status.unbound);  // just in case
    assert(!tstate->_status.bound_gilstate);
    assert(tstate != gilstate_get());
    assert(!tstate->_status.active);
    assert(tstate->thread_id == 0);
    assert(tstate->native_thread_id == 0);

    // Currently we don't necessarily store the thread state
    // in thread-local storage (e.g. per-interpreter).

    tstate->thread_id = PyThread_get_thread_ident();
#ifdef PY_HAVE_THREAD_NATIVE_ID
    tstate->native_thread_id = PyThread_get_thread_native_id();
#endif

#ifdef Py_GIL_DISABLED
    // Initialize biased reference counting inter-thread queue. Note that this
    // needs to be initialized from the active thread.
    _Py_brc_init_thread(tstate);
#endif

    // mimalloc state needs to be initialized from the active thread.
    tstate_mimalloc_bind(tstate);

    tstate->_status.bound = 1;
}

static void
unbind_tstate(PyThreadState *tstate)
{
    assert(tstate != NULL);
    assert(tstate_is_bound(tstate));
#ifndef HAVE_PTHREAD_STUBS
    assert(tstate->thread_id > 0);
#endif
#ifdef PY_HAVE_THREAD_NATIVE_ID
    assert(tstate->native_thread_id > 0);
#endif

    // We leave thread_id and native_thread_id alone
    // since they can be useful for debugging.
    // Check the `_status` field to know if these values
    // are still valid.

    // We leave tstate->_status.bound set to 1
    // to indicate it was previously bound.
    tstate->_status.unbound = 1;
}


/* Stick the thread state for this thread in thread specific storage.

   When a thread state is created for a thread by some mechanism
   other than PyGILState_Ensure(), it's important that the GILState
   machinery knows about it so it doesn't try to create another
   thread state for the thread.
   (This is a better fix for SF bug #1010677 than the first one attempted.)

   The only situation where you can legitimately have more than one
   thread state for an OS level thread is when there are multiple
   interpreters.

   Before 3.12, the PyGILState_*() APIs didn't work with multiple
   interpreters (see bpo-10915 and bpo-15751), so this function used
   to set TSS only once.  Thus, the first thread state created for that
   given OS level thread would "win", which seemed reasonable behaviour.
*/

static void
bind_gilstate_tstate(PyThreadState *tstate)
{
    assert(tstate != NULL);
    assert(tstate_is_alive(tstate));
    assert(tstate_is_bound(tstate));
    // XXX assert(!tstate->_status.active);
    assert(!tstate->_status.bound_gilstate);

    PyThreadState *tcur = gilstate_get();
    assert(tstate != tcur);

    if (tcur != NULL) {
        tcur->_status.bound_gilstate = 0;
    }
    gilstate_set(tstate);
    tstate->_status.bound_gilstate = 1;
}

static void
unbind_gilstate_tstate(PyThreadState *tstate)
{
    assert(tstate != NULL);
    // XXX assert(tstate_is_alive(tstate));
    assert(tstate_is_bound(tstate));
    // XXX assert(!tstate->_status.active);
    assert(tstate->_status.bound_gilstate);
    assert(tstate == gilstate_get());
    gilstate_clear();
    tstate->_status.bound_gilstate = 0;
}


//----------------------------------------------
// the thread state that currently holds the GIL
//----------------------------------------------

/* This is not exported, as it is not reliable!  It can only
   ever be compared to the state for the *current* thread.
   * If not equal, then it doesn't matter that the actual
     value may change immediately after comparison, as it can't
     possibly change to the current thread's state.
   * If equal, then the current thread holds the lock, so the value can't
     change until we yield the lock.
*/
static int
holds_gil(PyThreadState *tstate)
{
    // XXX Fall back to tstate->interp->runtime->ceval.gil.last_holder
    // (and tstate->interp->runtime->ceval.gil.locked).
    assert(tstate != NULL);
    /* Must be the tstate for this thread */
    assert(tstate == gilstate_get());
    return tstate == current_fast_get();
}


/****************************/
/* the global runtime state */
/****************************/

//----------
// lifecycle
//----------

/* Suppress deprecation warning for PyBytesObject.ob_shash */
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
/* We use "initial" if the runtime gets re-used
   (e.g. Py_Finalize() followed by Py_Initialize().
   Note that we initialize "initial" relative to _PyRuntime,
   to ensure pre-initialized pointers point to the active
   runtime state (and not "initial"). */
static const _PyRuntimeState initial = _PyRuntimeState_INIT(_PyRuntime, "");
_Py_COMP_DIAG_POP

#define LOCKS_INIT(runtime) \
    { \
        &(runtime)->interpreters.mutex, \
        &(runtime)->xi.data_lookup.registry.mutex, \
        &(runtime)->unicode_state.ids.mutex, \
        &(runtime)->imports.extensions.mutex, \
        &(runtime)->ceval.pending_mainthread.mutex, \
        &(runtime)->ceval.sys_trace_profile_mutex, \
        &(runtime)->atexit.mutex, \
        &(runtime)->audit_hooks.mutex, \
        &(runtime)->allocators.mutex, \
        &(runtime)->_main_interpreter.types.mutex, \
        &(runtime)->_main_interpreter.code_state.mutex, \
    }

static void
init_runtime(_PyRuntimeState *runtime,
             void *open_code_hook, void *open_code_userdata,
             _Py_AuditHookEntry *audit_hook_head,
             Py_ssize_t unicode_next_index)
{
    assert(!runtime->preinitializing);
    assert(!runtime->preinitialized);
    assert(!runtime->core_initialized);
    assert(!runtime->initialized);
    assert(!runtime->_initialized);

    runtime->open_code_hook = open_code_hook;
    runtime->open_code_userdata = open_code_userdata;
    runtime->audit_hooks.head = audit_hook_head;

    PyPreConfig_InitPythonConfig(&runtime->preconfig);

    // Set it to the ID of the main thread of the main interpreter.
    runtime->main_thread = PyThread_get_thread_ident();

    runtime->unicode_state.ids.next_index = unicode_next_index;

#if defined(__EMSCRIPTEN__) && defined(PY_CALL_TRAMPOLINE)
    _Py_EmscriptenTrampoline_Init(runtime);
#endif

    runtime->_initialized = 1;
}

PyStatus
_PyRuntimeState_Init(_PyRuntimeState *runtime)
{
    /* We preserve the hook across init, because there is
       currently no public API to set it between runtime
       initialization and interpreter initialization. */
    void *open_code_hook = runtime->open_code_hook;
    void *open_code_userdata = runtime->open_code_userdata;
    _Py_AuditHookEntry *audit_hook_head = runtime->audit_hooks.head;
    // bpo-42882: Preserve next_index value if Py_Initialize()/Py_Finalize()
    // is called multiple times.
    Py_ssize_t unicode_next_index = runtime->unicode_state.ids.next_index;

    if (runtime->_initialized) {
        // Py_Initialize() must be running again.
        // Reset to _PyRuntimeState_INIT.
        memcpy(runtime, &initial, sizeof(*runtime));
        // Preserve the cookie from the original runtime.
        memcpy(runtime->debug_offsets.cookie, _Py_Debug_Cookie, 8);
        assert(!runtime->_initialized);
    }

    PyStatus status = _PyTime_Init(&runtime->time);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    init_runtime(runtime, open_code_hook, open_code_userdata, audit_hook_head,
                 unicode_next_index);

    return _PyStatus_OK();
}

void
_PyRuntimeState_Fini(_PyRuntimeState *runtime)
{
#ifdef Py_REF_DEBUG
    /* The count is cleared by _Py_FinalizeRefTotal(). */
    assert(runtime->object_state.interpreter_leaks == 0);
#endif
    gilstate_clear();
}

#ifdef HAVE_FORK
/* This function is called from PyOS_AfterFork_Child to ensure that
   newly created child processes do not share locks with the parent. */
PyStatus
_PyRuntimeState_ReInitThreads(_PyRuntimeState *runtime)
{
    // This was initially set in _PyRuntimeState_Init().
    runtime->main_thread = PyThread_get_thread_ident();

    // Clears the parking lot. Any waiting threads are dead. This must be
    // called before releasing any locks that use the parking lot.
    _PyParkingLot_AfterFork();

    // Re-initialize global locks
    PyMutex *locks[] = LOCKS_INIT(runtime);
    for (size_t i = 0; i < Py_ARRAY_LENGTH(locks); i++) {
        _PyMutex_at_fork_reinit(locks[i]);
    }
#ifdef Py_GIL_DISABLED
    for (PyInterpreterState *interp = runtime->interpreters.head;
         interp != NULL; interp = interp->next)
    {
        for (int i = 0; i < NUM_WEAKREF_LIST_LOCKS; i++) {
            _PyMutex_at_fork_reinit(&interp->weakref_locks[i]);
        }
    }
#endif

    _PyTypes_AfterFork();

    _PyThread_AfterFork(&runtime->threads);

    return _PyStatus_OK();
}
#endif


/*************************************/
/* the per-interpreter runtime state */
/*************************************/

//----------
// lifecycle
//----------

/* Calling this indicates that the runtime is ready to create interpreters. */

PyStatus
_PyInterpreterState_Enable(_PyRuntimeState *runtime)
{
    struct pyinterpreters *interpreters = &runtime->interpreters;
    interpreters->next_id = 0;
    return _PyStatus_OK();
}

static PyInterpreterState *
alloc_interpreter(void)
{
    size_t alignment = _Alignof(PyInterpreterState);
    size_t allocsize = sizeof(PyInterpreterState) + alignment - 1;
    void *mem = PyMem_RawCalloc(1, allocsize);
    if (mem == NULL) {
        return NULL;
    }
    PyInterpreterState *interp = _Py_ALIGN_UP(mem, alignment);
    assert(_Py_IS_ALIGNED(interp, alignment));
    interp->_malloced = mem;
    return interp;
}

static void
free_interpreter(PyInterpreterState *interp)
{
    // The main interpreter is statically allocated so
    // should not be freed.
    if (interp != &_PyRuntime._main_interpreter) {
        if (_PyMem_obmalloc_state_on_heap(interp)) {
            // interpreter has its own obmalloc state, free it
            PyMem_RawFree(interp->obmalloc);
            interp->obmalloc = NULL;
        }
        assert(_Py_IS_ALIGNED(interp, _Alignof(PyInterpreterState)));
        PyMem_RawFree(interp->_malloced);
    }
}

#ifndef NDEBUG
static inline int check_interpreter_whence(long);
#endif

/* Get the interpreter state to a minimal consistent state.
   Further init happens in pylifecycle.c before it can be used.
   All fields not initialized here are expected to be zeroed out,
   e.g. by PyMem_RawCalloc() or memset(), or otherwise pre-initialized.
   The runtime state is not manipulated.  Instead it is assumed that
   the interpreter is getting added to the runtime.

   Note that the main interpreter was statically initialized as part
   of the runtime and most state is already set properly.  That leaves
   a small number of fields to initialize dynamically, as well as some
   that are initialized lazily.

   For subinterpreters we memcpy() the main interpreter in
   PyInterpreterState_New(), leaving it in the same mostly-initialized
   state.  The only difference is that the interpreter has some
   self-referential state that is statically initializexd to the
   main interpreter.  We fix those fields here, in addition
   to the other dynamically initialized fields.
  */
static PyStatus
init_interpreter(PyInterpreterState *interp,
                 _PyRuntimeState *runtime, int64_t id,
                 PyInterpreterState *next,
                 long whence)
{
    if (interp->_initialized) {
        return _PyStatus_ERR("interpreter already initialized");
    }

    assert(interp->_whence == _PyInterpreterState_WHENCE_NOTSET);
    assert(check_interpreter_whence(whence) == 0);
    interp->_whence = whence;

    assert(runtime != NULL);
    interp->runtime = runtime;

    assert(id > 0 || (id == 0 && interp == runtime->interpreters.main));
    interp->id = id;

    interp->id_refcount = 0;

    assert(runtime->interpreters.head == interp);
    assert(next != NULL || (interp == runtime->interpreters.main));
    interp->next = next;

    interp->threads.preallocated = &interp->_initial_thread;

    // We would call _PyObject_InitState() at this point
    // if interp->feature_flags were alredy set.

    _PyEval_InitState(interp);
    _PyGC_InitState(&interp->gc);
    PyConfig_InitPythonConfig(&interp->config);
    _PyType_InitCache(interp);
#ifdef Py_GIL_DISABLED
    _Py_brc_init_state(interp);
#endif
    llist_init(&interp->mem_free_queue.head);
    llist_init(&interp->asyncio_tasks_head);
    interp->asyncio_tasks_lock = (PyMutex){0};
    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        interp->monitors.tools[i] = 0;
    }
    for (int t = 0; t < PY_MONITORING_TOOL_IDS; t++) {
        for (int e = 0; e < _PY_MONITORING_EVENTS; e++) {
            interp->monitoring_callables[t][e] = NULL;

        }
        interp->monitoring_tool_versions[t] = 0;
    }
    interp->sys_profile_initialized = false;
    interp->sys_trace_initialized = false;
    interp->jit = false;
    interp->executor_list_head = NULL;
    interp->executor_deletion_list_head = NULL;
    interp->executor_deletion_list_remaining_capacity = 0;
    interp->trace_run_counter = JIT_CLEANUP_THRESHOLD;
    if (interp != &runtime->_main_interpreter) {
        /* Fix the self-referential, statically initialized fields. */
        interp->dtoa = (struct _dtoa_state)_dtoa_state_INIT(interp);
    }
#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)
    interp->next_stackref = INITIAL_STACKREF_INDEX;
    _Py_hashtable_allocator_t alloc = {
        .malloc = malloc,
        .free = free,
    };
    interp->open_stackrefs_table = _Py_hashtable_new_full(
        _Py_hashtable_hash_ptr,
        _Py_hashtable_compare_direct,
        NULL,
        NULL,
        &alloc
    );
#  ifdef Py_STACKREF_CLOSE_DEBUG
    interp->closed_stackrefs_table = _Py_hashtable_new_full(
        _Py_hashtable_hash_ptr,
        _Py_hashtable_compare_direct,
        NULL,
        NULL,
        &alloc
    );
#  endif
    _Py_stackref_associate(interp, Py_None, PyStackRef_None);
    _Py_stackref_associate(interp, Py_False, PyStackRef_False);
    _Py_stackref_associate(interp, Py_True, PyStackRef_True);
#endif

    interp->_initialized = 1;
    return _PyStatus_OK();
}


PyStatus
_PyInterpreterState_New(PyThreadState *tstate, PyInterpreterState **pinterp)
{
    *pinterp = NULL;

    // Don't get runtime from tstate since tstate can be NULL
    _PyRuntimeState *runtime = &_PyRuntime;

    // tstate is NULL when pycore_create_interpreter() calls
    // _PyInterpreterState_New() to create the main interpreter.
    if (tstate != NULL) {
        if (_PySys_Audit(tstate, "cpython.PyInterpreterState_New", NULL) < 0) {
            return _PyStatus_ERR("sys.audit failed");
        }
    }

    /* We completely serialize creation of multiple interpreters, since
       it simplifies things here and blocking concurrent calls isn't a problem.
       Regardless, we must fully block subinterpreter creation until
       after the main interpreter is created. */
    HEAD_LOCK(runtime);

    struct pyinterpreters *interpreters = &runtime->interpreters;
    int64_t id = interpreters->next_id;
    interpreters->next_id += 1;

    // Allocate the interpreter and add it to the runtime state.
    PyInterpreterState *interp;
    PyStatus status;
    PyInterpreterState *old_head = interpreters->head;
    if (old_head == NULL) {
        // We are creating the main interpreter.
        assert(interpreters->main == NULL);
        assert(id == 0);

        interp = &runtime->_main_interpreter;
        assert(interp->id == 0);
        assert(interp->next == NULL);

        interpreters->main = interp;
    }
    else {
        assert(interpreters->main != NULL);
        assert(id != 0);

        interp = alloc_interpreter();
        if (interp == NULL) {
            status = _PyStatus_NO_MEMORY();
            goto error;
        }
        // Set to _PyInterpreterState_INIT.
        memcpy(interp, &initial._main_interpreter, sizeof(*interp));

        if (id < 0) {
            /* overflow or Py_Initialize() not called yet! */
            status = _PyStatus_ERR("failed to get an interpreter ID");
            goto error;
        }
    }
    interpreters->head = interp;

    long whence = _PyInterpreterState_WHENCE_UNKNOWN;
    status = init_interpreter(interp, runtime,
                              id, old_head, whence);
    if (_PyStatus_EXCEPTION(status)) {
        goto error;
    }

    HEAD_UNLOCK(runtime);

    assert(interp != NULL);
    *pinterp = interp;
    return _PyStatus_OK();

error:
    HEAD_UNLOCK(runtime);

    if (interp != NULL) {
        free_interpreter(interp);
    }
    return status;
}


PyInterpreterState *
PyInterpreterState_New(void)
{
    // tstate can be NULL
    PyThreadState *tstate = current_fast_get();

    PyInterpreterState *interp;
    PyStatus status = _PyInterpreterState_New(tstate, &interp);
    if (_PyStatus_EXCEPTION(status)) {
        Py_ExitStatusException(status);
    }
    assert(interp != NULL);
    return interp;
}

#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)
extern void
_Py_stackref_report_leaks(PyInterpreterState *interp);
#endif

static void
interpreter_clear(PyInterpreterState *interp, PyThreadState *tstate)
{
    assert(interp != NULL);
    assert(tstate != NULL);
    _PyRuntimeState *runtime = interp->runtime;

    /* XXX Conditions we need to enforce:

       * the GIL must be held by the current thread
       * tstate must be the "current" thread state (current_fast_get())
       * tstate->interp must be interp
       * for the main interpreter, tstate must be the main thread
     */
    // XXX Ideally, we would not rely on any thread state in this function
    // (and we would drop the "tstate" argument).

    if (_PySys_Audit(tstate, "cpython.PyInterpreterState_Clear", NULL) < 0) {
        _PyErr_Clear(tstate);
    }

    // Clear the current/main thread state last.
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        // See https://github.com/python/cpython/issues/102126
        // Must be called without HEAD_LOCK held as it can deadlock
        // if any finalizer tries to acquire that lock.
        HEAD_UNLOCK(runtime);
        PyThreadState_Clear(p);
        HEAD_LOCK(runtime);
    }
    _Py_FOR_EACH_TSTATE_END(interp);
    if (tstate->interp == interp) {
        /* We fix tstate->_status below when we for sure aren't using it
           (e.g. no longer need the GIL). */
        // XXX Eliminate the need to do this.
        tstate->_status.cleared = 0;
    }

    /* It is possible that any of the objects below have a finalizer
       that runs Python code or otherwise relies on a thread state
       or even the interpreter state.  For now we trust that isn't
       a problem.
     */
    // XXX Make sure we properly deal with problematic finalizers.

    Py_CLEAR(interp->audit_hooks);

    // At this time, all the threads should be cleared so we don't need atomic
    // operations for instrumentation_version or eval_breaker.
    interp->ceval.instrumentation_version = 0;
    tstate->eval_breaker = 0;

    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        interp->monitors.tools[i] = 0;
    }
    for (int t = 0; t < PY_MONITORING_TOOL_IDS; t++) {
        for (int e = 0; e < _PY_MONITORING_EVENTS; e++) {
            Py_CLEAR(interp->monitoring_callables[t][e]);
        }
    }
    interp->sys_profile_initialized = false;
    interp->sys_trace_initialized = false;
    for (int t = 0; t < PY_MONITORING_TOOL_IDS; t++) {
        Py_CLEAR(interp->monitoring_tool_names[t]);
    }

    PyConfig_Clear(&interp->config);
    _PyCodec_Fini(interp);

    assert(interp->imports.modules == NULL);
    assert(interp->imports.modules_by_index == NULL);
    assert(interp->imports.importlib == NULL);
    assert(interp->imports.import_func == NULL);

    Py_CLEAR(interp->sysdict_copy);
    Py_CLEAR(interp->builtins_copy);
    Py_CLEAR(interp->dict);
#ifdef HAVE_FORK
    Py_CLEAR(interp->before_forkers);
    Py_CLEAR(interp->after_forkers_parent);
    Py_CLEAR(interp->after_forkers_child);
#endif


#ifdef _Py_TIER2
    _Py_ClearExecutorDeletionList(interp);
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

#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)
#  ifdef Py_STACKREF_CLOSE_DEBUG
    _Py_hashtable_destroy(interp->closed_stackrefs_table);
    interp->closed_stackrefs_table = NULL;
#  endif
    _Py_stackref_report_leaks(interp);
    _Py_hashtable_destroy(interp->open_stackrefs_table);
    interp->open_stackrefs_table = NULL;
#endif

    if (tstate->interp == interp) {
        /* We are now safe to fix tstate->_status.cleared. */
        // XXX Do this (much) earlier?
        tstate->_status.cleared = 1;
    }

    for (int i=0; i < DICT_MAX_WATCHERS; i++) {
        interp->dict_state.watchers[i] = NULL;
    }

    for (int i=0; i < TYPE_MAX_WATCHERS; i++) {
        interp->type_watchers[i] = NULL;
    }

    for (int i=0; i < FUNC_MAX_WATCHERS; i++) {
        interp->func_watchers[i] = NULL;
    }
    interp->active_func_watchers = 0;

    for (int i=0; i < CODE_MAX_WATCHERS; i++) {
        interp->code_watchers[i] = NULL;
    }
    interp->active_code_watchers = 0;

    for (int i=0; i < CONTEXT_MAX_WATCHERS; i++) {
        interp->context_watchers[i] = NULL;
    }
    interp->active_context_watchers = 0;
    // XXX Once we have one allocator per interpreter (i.e.
    // per-interpreter GC) we must ensure that all of the interpreter's
    // objects have been cleaned up at the point.

    // We could clear interp->threads.freelist here
    // if it held more than just the initial thread state.
}


void
PyInterpreterState_Clear(PyInterpreterState *interp)
{
    // Use the current Python thread state to call audit hooks and to collect
    // garbage. It can be different than the current Python thread state
    // of 'interp'.
    PyThreadState *current_tstate = current_fast_get();
    _PyImport_ClearCore(interp);
    interpreter_clear(interp, current_tstate);
}


void
_PyInterpreterState_Clear(PyThreadState *tstate)
{
    _PyImport_ClearCore(tstate->interp);
    interpreter_clear(tstate->interp, tstate);
}


static inline void tstate_deactivate(PyThreadState *tstate);
static void tstate_set_detached(PyThreadState *tstate, int detached_state);
static void zapthreads(PyInterpreterState *interp);

void
PyInterpreterState_Delete(PyInterpreterState *interp)
{
    _PyRuntimeState *runtime = interp->runtime;
    struct pyinterpreters *interpreters = &runtime->interpreters;

    // XXX Clearing the "current" thread state should happen before
    // we start finalizing the interpreter (or the current thread state).
    PyThreadState *tcur = current_fast_get();
    if (tcur != NULL && interp == tcur->interp) {
        /* Unset current thread.  After this, many C API calls become crashy. */
        _PyThreadState_Detach(tcur);
    }

    zapthreads(interp);

    // XXX These two calls should be done at the end of clear_interpreter(),
    // but currently some objects get decref'ed after that.
#ifdef Py_REF_DEBUG
    _PyInterpreterState_FinalizeRefTotal(interp);
#endif
    _PyInterpreterState_FinalizeAllocatedBlocks(interp);

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
    if (interp->threads.head != NULL) {
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

    _Py_qsbr_fini(interp);

    _PyObject_FiniState(interp);

    free_interpreter(interp);
}


#ifdef HAVE_FORK
/*
 * Delete all interpreter states except the main interpreter.  If there
 * is a current interpreter state, it *must* be the main interpreter.
 */
PyStatus
_PyInterpreterState_DeleteExceptMain(_PyRuntimeState *runtime)
{
    struct pyinterpreters *interpreters = &runtime->interpreters;

    PyThreadState *tstate = _PyThreadState_Swap(runtime, NULL);
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

        // XXX Won't this fail since PyInterpreterState_Clear() requires
        // the "current" tstate to be set?
        PyInterpreterState_Clear(interp);  // XXX must activate?
        zapthreads(interp);
        PyInterpreterState *prev_interp = interp;
        interp = interp->next;
        free_interpreter(prev_interp);
    }
    HEAD_UNLOCK(runtime);

    if (interpreters->head == NULL) {
        return _PyStatus_ERR("missing main interpreter");
    }
    _PyThreadState_Swap(runtime, tstate);
    return _PyStatus_OK();
}
#endif

static inline void
set_main_thread(PyInterpreterState *interp, PyThreadState *tstate)
{
    _Py_atomic_store_ptr_relaxed(&interp->threads.main, tstate);
}

static inline PyThreadState *
get_main_thread(PyInterpreterState *interp)
{
    return _Py_atomic_load_ptr_relaxed(&interp->threads.main);
}

void
_PyErr_SetInterpreterAlreadyRunning(void)
{
    PyErr_SetString(PyExc_InterpreterError, "interpreter already running");
}

int
_PyInterpreterState_SetRunningMain(PyInterpreterState *interp)
{
    if (get_main_thread(interp) != NULL) {
        _PyErr_SetInterpreterAlreadyRunning();
        return -1;
    }
    PyThreadState *tstate = current_fast_get();
    _Py_EnsureTstateNotNULL(tstate);
    if (tstate->interp != interp) {
        PyErr_SetString(PyExc_RuntimeError,
                        "current tstate has wrong interpreter");
        return -1;
    }
    set_main_thread(interp, tstate);

    return 0;
}

void
_PyInterpreterState_SetNotRunningMain(PyInterpreterState *interp)
{
    assert(get_main_thread(interp) == current_fast_get());
    set_main_thread(interp, NULL);
}

int
_PyInterpreterState_IsRunningMain(PyInterpreterState *interp)
{
    if (get_main_thread(interp) != NULL) {
        return 1;
    }
    // Embedders might not know to call _PyInterpreterState_SetRunningMain(),
    // so their main thread wouldn't show it is running the main interpreter's
    // program.  (Py_Main() doesn't have this problem.)  For now this isn't
    // critical.  If it were, we would need to infer "running main" from other
    // information, like if it's the main interpreter.  We used to do that
    // but the naive approach led to some inconsistencies that caused problems.
    return 0;
}

int
_PyThreadState_IsRunningMain(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    // See the note in _PyInterpreterState_IsRunningMain() about
    // possible false negatives here for embedders.
    return get_main_thread(interp) == tstate;
}

void
_PyInterpreterState_ReinitRunningMain(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    if (get_main_thread(interp) != tstate) {
        set_main_thread(interp, NULL);
    }
}


//----------
// accessors
//----------

int
_PyInterpreterState_IsReady(PyInterpreterState *interp)
{
    return interp->_ready;
}

#ifndef NDEBUG
static inline int
check_interpreter_whence(long whence)
{
    if(whence < 0) {
        return -1;
    }
    if (whence > _PyInterpreterState_WHENCE_MAX) {
        return -1;
    }
    return 0;
}
#endif

long
_PyInterpreterState_GetWhence(PyInterpreterState *interp)
{
    assert(check_interpreter_whence(interp->_whence) == 0);
    return interp->_whence;
}

void
_PyInterpreterState_SetWhence(PyInterpreterState *interp, long whence)
{
    assert(interp->_whence != _PyInterpreterState_WHENCE_NOTSET);
    assert(check_interpreter_whence(whence) == 0);
    interp->_whence = whence;
}


PyObject *
_Py_GetMainModule(PyThreadState *tstate)
{
    // We return None to indicate "not found" or "bogus".
    PyObject *modules = _PyImport_GetModulesRef(tstate->interp);
    if (modules == Py_None) {
        return modules;
    }
    PyObject *module = NULL;
    (void)PyMapping_GetOptionalItem(modules, &_Py_ID(__main__), &module);
    Py_DECREF(modules);
    if (module == NULL && !PyErr_Occurred()) {
        Py_RETURN_NONE;
    }
    return module;
}

int
_Py_CheckMainModule(PyObject *module)
{
    if (module == NULL || module == Py_None) {
        if (!PyErr_Occurred()) {
            (void)_PyErr_SetModuleNotFoundError(&_Py_ID(__main__));
        }
        return -1;
    }
    if (!Py_IS_TYPE(module, &PyModule_Type)) {
        /* The __main__ module has been tampered with. */
        PyObject *msg = PyUnicode_FromString("invalid __main__ module");
        if (msg != NULL) {
            (void)PyErr_SetImportError(msg, &_Py_ID(__main__), NULL);
        }
        return -1;
    }
    return 0;
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


//----------
// interp ID
//----------

int64_t
_PyInterpreterState_ObjectToID(PyObject *idobj)
{
    if (!_PyIndex_Check(idobj)) {
        PyErr_Format(PyExc_TypeError,
                     "interpreter ID must be an int, got %.100s",
                     Py_TYPE(idobj)->tp_name);
        return -1;
    }

    // This may raise OverflowError.
    // For now, we don't worry about if LLONG_MAX < INT64_MAX.
    long long id = PyLong_AsLongLong(idobj);
    if (id == -1 && PyErr_Occurred()) {
        return -1;
    }

    if (id < 0) {
        PyErr_Format(PyExc_ValueError,
                     "interpreter ID must be a non-negative int, got %R",
                     idobj);
        return -1;
    }
#if LLONG_MAX > INT64_MAX
    else if (id > INT64_MAX) {
        PyErr_SetString(PyExc_OverflowError, "int too big to convert");
        return -1;
    }
#endif
    else {
        return (int64_t)id;
    }
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

PyObject *
_PyInterpreterState_GetIDObject(PyInterpreterState *interp)
{
    int64_t interpid = interp->id;
    if (interpid < 0) {
        return NULL;
    }
    assert(interpid < LLONG_MAX);
    return PyLong_FromLongLong(interpid);
}



void
_PyInterpreterState_IDIncref(PyInterpreterState *interp)
{
    _Py_atomic_add_ssize(&interp->id_refcount, 1);
}


void
_PyInterpreterState_IDDecref(PyInterpreterState *interp)
{
    _PyRuntimeState *runtime = interp->runtime;

    Py_ssize_t refcount = _Py_atomic_add_ssize(&interp->id_refcount, -1);

    if (refcount == 1 && interp->requires_idref) {
        PyThreadState *tstate =
            _PyThreadState_NewBound(interp, _PyThreadState_WHENCE_FINI);

        // XXX Possible GILState issues?
        PyThreadState *save_tstate = _PyThreadState_Swap(runtime, tstate);
        Py_EndInterpreter(tstate);
        _PyThreadState_Swap(runtime, save_tstate);
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


//-----------------------------
// look up an interpreter state
//-----------------------------

/* Return the interpreter associated with the current OS thread.

   The GIL must be held.
  */

PyInterpreterState*
PyInterpreterState_Get(void)
{
    PyThreadState *tstate = current_fast_get();
    _Py_EnsureTstateNotNULL(tstate);
    PyInterpreterState *interp = tstate->interp;
    if (interp == NULL) {
        Py_FatalError("no current interpreter");
    }
    return interp;
}


static PyInterpreterState *
interp_look_up_id(_PyRuntimeState *runtime, int64_t requested_id)
{
    PyInterpreterState *interp = runtime->interpreters.head;
    while (interp != NULL) {
        int64_t id = interp->id;
        assert(id >= 0);
        if (requested_id == id) {
            return interp;
        }
        interp = PyInterpreterState_Next(interp);
    }
    return NULL;
}

/* Return the interpreter state with the given ID.

   Fail with RuntimeError if the interpreter is not found. */

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
        PyErr_Format(PyExc_InterpreterNotFoundError,
                     "unrecognized interpreter ID %lld", requested_id);
    }
    return interp;
}

PyInterpreterState *
_PyInterpreterState_LookUpIDObject(PyObject *requested_id)
{
    int64_t id = _PyInterpreterState_ObjectToID(requested_id);
    if (id < 0) {
        return NULL;
    }
    return _PyInterpreterState_LookUpID(id);
}


/********************************/
/* the per-thread runtime state */
/********************************/

#ifndef NDEBUG
static inline int
tstate_is_alive(PyThreadState *tstate)
{
    return (tstate->_status.initialized &&
            !tstate->_status.finalized &&
            !tstate->_status.cleared &&
            !tstate->_status.finalizing);
}
#endif


//----------
// lifecycle
//----------

/* Minimum size of data stack chunk */
#define DATA_STACK_CHUNK_SIZE (16*1024)

static _PyStackChunk*
allocate_chunk(int size_in_bytes, _PyStackChunk* previous)
{
    assert(size_in_bytes % sizeof(PyObject **) == 0);
    _PyStackChunk *res = _PyObject_VirtualAlloc(size_in_bytes);
    if (res == NULL) {
        return NULL;
    }
    res->previous = previous;
    res->size = size_in_bytes;
    res->top = 0;
    return res;
}

static void
reset_threadstate(_PyThreadStateImpl *tstate)
{
    // Set to _PyThreadState_INIT directly?
    memcpy(tstate,
           &initial._main_interpreter._initial_thread,
           sizeof(*tstate));
}

static _PyThreadStateImpl *
alloc_threadstate(PyInterpreterState *interp)
{
    _PyThreadStateImpl *tstate;

    // Try the preallocated tstate first.
    tstate = _Py_atomic_exchange_ptr(&interp->threads.preallocated, NULL);

    // Fall back to the allocator.
    if (tstate == NULL) {
        tstate = PyMem_RawCalloc(1, sizeof(_PyThreadStateImpl));
        if (tstate == NULL) {
            return NULL;
        }
        reset_threadstate(tstate);
    }
    return tstate;
}

static void
free_threadstate(_PyThreadStateImpl *tstate)
{
    PyInterpreterState *interp = tstate->base.interp;
    // The initial thread state of the interpreter is allocated
    // as part of the interpreter state so should not be freed.
    if (tstate == &interp->_initial_thread) {
        // Make it available again.
        reset_threadstate(tstate);
        assert(interp->threads.preallocated == NULL);
        _Py_atomic_store_ptr(&interp->threads.preallocated, tstate);
    }
    else {
        PyMem_RawFree(tstate);
    }
}

static void
decref_threadstate(_PyThreadStateImpl *tstate)
{
    if (_Py_atomic_add_ssize(&tstate->refcount, -1) == 1) {
        // The last reference to the thread state is gone.
        free_threadstate(tstate);
    }
}

/* Get the thread state to a minimal consistent state.
   Further init happens in pylifecycle.c before it can be used.
   All fields not initialized here are expected to be zeroed out,
   e.g. by PyMem_RawCalloc() or memset(), or otherwise pre-initialized.
   The interpreter state is not manipulated.  Instead it is assumed that
   the thread is getting added to the interpreter.
  */

static void
init_threadstate(_PyThreadStateImpl *_tstate,
                 PyInterpreterState *interp, uint64_t id, int whence)
{
    PyThreadState *tstate = (PyThreadState *)_tstate;
    if (tstate->_status.initialized) {
        Py_FatalError("thread state already initialized");
    }

    assert(interp != NULL);
    tstate->interp = interp;
    tstate->eval_breaker =
        _Py_atomic_load_uintptr_relaxed(&interp->ceval.instrumentation_version);

    // next/prev are set in add_threadstate().
    assert(tstate->next == NULL);
    assert(tstate->prev == NULL);

    assert(tstate->_whence == _PyThreadState_WHENCE_NOTSET);
    assert(whence >= 0 && whence <= _PyThreadState_WHENCE_EXEC);
    tstate->_whence = whence;

    assert(id > 0);
    tstate->id = id;

    // thread_id and native_thread_id are set in bind_tstate().

    tstate->py_recursion_limit = interp->ceval.recursion_limit;
    tstate->py_recursion_remaining = interp->ceval.recursion_limit;
    tstate->exc_info = &tstate->exc_state;

    // PyGILState_Release must not try to delete this thread state.
    // This is cleared when PyGILState_Ensure() creates the thread state.
    tstate->gilstate_counter = 1;

    tstate->current_frame = NULL;
    tstate->datastack_chunk = NULL;
    tstate->datastack_top = NULL;
    tstate->datastack_limit = NULL;
    tstate->what_event = -1;
    tstate->current_executor = NULL;
    tstate->dict_global_version = 0;

    _tstate->c_stack_soft_limit = UINTPTR_MAX;
    _tstate->c_stack_top = 0;
    _tstate->c_stack_hard_limit = 0;

    _tstate->asyncio_running_loop = NULL;
    _tstate->asyncio_running_task = NULL;

    tstate->delete_later = NULL;

    llist_init(&_tstate->mem_free_queue);
    llist_init(&_tstate->asyncio_tasks_head);
    if (interp->stoptheworld.requested || _PyRuntime.stoptheworld.requested) {
        // Start in the suspended state if there is an ongoing stop-the-world.
        tstate->state = _Py_THREAD_SUSPENDED;
    }

    tstate->_status.initialized = 1;
}

static void
add_threadstate(PyInterpreterState *interp, PyThreadState *tstate,
                PyThreadState *next)
{
    assert(interp->threads.head != tstate);
    if (next != NULL) {
        assert(next->prev == NULL || next->prev == tstate);
        next->prev = tstate;
    }
    tstate->next = next;
    assert(tstate->prev == NULL);
    interp->threads.head = tstate;
}

static PyThreadState *
new_threadstate(PyInterpreterState *interp, int whence)
{
    // Allocate the thread state.
    _PyThreadStateImpl *tstate = alloc_threadstate(interp);
    if (tstate == NULL) {
        return NULL;
    }

#ifdef Py_GIL_DISABLED
    Py_ssize_t qsbr_idx = _Py_qsbr_reserve(interp);
    if (qsbr_idx < 0) {
        free_threadstate(tstate);
        return NULL;
    }
    int32_t tlbc_idx = _Py_ReserveTLBCIndex(interp);
    if (tlbc_idx < 0) {
        free_threadstate(tstate);
        return NULL;
    }
#endif

    /* We serialize concurrent creation to protect global state. */
    HEAD_LOCK(interp->runtime);

    // Initialize the new thread state.
    interp->threads.next_unique_id += 1;
    uint64_t id = interp->threads.next_unique_id;
    init_threadstate(tstate, interp, id, whence);

    // Add the new thread state to the interpreter.
    PyThreadState *old_head = interp->threads.head;
    add_threadstate(interp, (PyThreadState *)tstate, old_head);

    HEAD_UNLOCK(interp->runtime);

#ifdef Py_GIL_DISABLED
    // Must be called with lock unlocked to avoid lock ordering deadlocks.
    _Py_qsbr_register(tstate, interp, qsbr_idx);
    tstate->tlbc_index = tlbc_idx;
#endif

    return (PyThreadState *)tstate;
}

PyThreadState *
PyThreadState_New(PyInterpreterState *interp)
{
    return _PyThreadState_NewBound(interp, _PyThreadState_WHENCE_UNKNOWN);
}

PyThreadState *
_PyThreadState_NewBound(PyInterpreterState *interp, int whence)
{
    PyThreadState *tstate = new_threadstate(interp, whence);
    if (tstate) {
        bind_tstate(tstate);
        // This makes sure there's a gilstate tstate bound
        // as soon as possible.
        if (gilstate_get() == NULL) {
            bind_gilstate_tstate(tstate);
        }
    }
    return tstate;
}

// This must be followed by a call to _PyThreadState_Bind();
PyThreadState *
_PyThreadState_New(PyInterpreterState *interp, int whence)
{
    return new_threadstate(interp, whence);
}

// We keep this for stable ABI compabibility.
PyAPI_FUNC(PyThreadState*)
_PyThreadState_Prealloc(PyInterpreterState *interp)
{
    return _PyThreadState_New(interp, _PyThreadState_WHENCE_UNKNOWN);
}

// We keep this around for (accidental) stable ABI compatibility.
// Realistically, no extensions are using it.
PyAPI_FUNC(void)
_PyThreadState_Init(PyThreadState *tstate)
{
    Py_FatalError("_PyThreadState_Init() is for internal use only");
}


static void
clear_datastack(PyThreadState *tstate)
{
    _PyStackChunk *chunk = tstate->datastack_chunk;
    tstate->datastack_chunk = NULL;
    while (chunk != NULL) {
        _PyStackChunk *prev = chunk->previous;
        _PyObject_VirtualFree(chunk, chunk->size);
        chunk = prev;
    }
}

void
PyThreadState_Clear(PyThreadState *tstate)
{
    assert(tstate->_status.initialized && !tstate->_status.cleared);
    assert(current_fast_get()->interp == tstate->interp);
    assert(!_PyThreadState_IsRunningMain(tstate));
    // XXX assert(!tstate->_status.bound || tstate->_status.unbound);
    tstate->_status.finalizing = 1;  // just in case

    /* XXX Conditions we need to enforce:

       * the GIL must be held by the current thread
       * current_fast_get()->interp must match tstate->interp
       * for the main interpreter, current_fast_get() must be the main thread
     */

    int verbose = _PyInterpreterState_GetConfig(tstate->interp)->verbose;

    if (verbose && tstate->current_frame != NULL) {
        /* bpo-20526: After the main thread calls
           _PyInterpreterState_SetFinalizing() in Py_FinalizeEx()
           (or in Py_EndInterpreter() for subinterpreters),
           threads must exit when trying to take the GIL.
           If a thread exit in the middle of _PyEval_EvalFrameDefault(),
           tstate->frame is not reset to its previous value.
           It is more likely with daemon threads, but it can happen
           with regular threads if threading._shutdown() fails
           (ex: interrupted by CTRL+C). */
        fprintf(stderr,
          "PyThreadState_Clear: warning: thread still has a frame\n");
    }

    if (verbose && tstate->current_exception != NULL) {
        fprintf(stderr, "PyThreadState_Clear: warning: thread has an exception set\n");
        _PyErr_Print(tstate);
    }

    /* At this point tstate shouldn't be used any more,
       neither to run Python code nor for other uses.

       This is tricky when current_fast_get() == tstate, in the same way
       as noted in interpreter_clear() above.  The below finalizers
       can possibly run Python code or otherwise use the partially
       cleared thread state.  For now we trust that isn't a problem
       in practice.
     */
    // XXX Deal with the possibility of problematic finalizers.

    /* Don't clear tstate->pyframe: it is a borrowed reference */

    Py_CLEAR(tstate->threading_local_key);
    Py_CLEAR(tstate->threading_local_sentinel);

    Py_CLEAR(((_PyThreadStateImpl *)tstate)->asyncio_running_loop);
    Py_CLEAR(((_PyThreadStateImpl *)tstate)->asyncio_running_task);


    PyMutex_Lock(&tstate->interp->asyncio_tasks_lock);
    // merge any lingering tasks from thread state to interpreter's
    // tasks list
    llist_concat(&tstate->interp->asyncio_tasks_head,
                 &((_PyThreadStateImpl *)tstate)->asyncio_tasks_head);
    PyMutex_Unlock(&tstate->interp->asyncio_tasks_lock);

    Py_CLEAR(tstate->dict);
    Py_CLEAR(tstate->async_exc);

    Py_CLEAR(tstate->current_exception);

    Py_CLEAR(tstate->exc_state.exc_value);

    /* The stack of exception states should contain just this thread. */
    if (verbose && tstate->exc_info != &tstate->exc_state) {
        fprintf(stderr,
          "PyThreadState_Clear: warning: thread still has a generator\n");
    }

    if (tstate->c_profilefunc != NULL) {
        tstate->interp->sys_profiling_threads--;
        tstate->c_profilefunc = NULL;
    }
    if (tstate->c_tracefunc != NULL) {
        tstate->interp->sys_tracing_threads--;
        tstate->c_tracefunc = NULL;
    }
    Py_CLEAR(tstate->c_profileobj);
    Py_CLEAR(tstate->c_traceobj);

    Py_CLEAR(tstate->async_gen_firstiter);
    Py_CLEAR(tstate->async_gen_finalizer);

    Py_CLEAR(tstate->context);

#ifdef Py_GIL_DISABLED
    // Each thread should clear own freelists in free-threading builds.
    struct _Py_freelists *freelists = _Py_freelists_GET();
    _PyObject_ClearFreeLists(freelists, 1);

    // Merge our thread-local refcounts into the type's own refcount and
    // free our local refcount array.
    _PyObject_FinalizePerThreadRefcounts((_PyThreadStateImpl *)tstate);

    // Remove ourself from the biased reference counting table of threads.
    _Py_brc_remove_thread(tstate);

    // Release our thread-local copies of the bytecode for reuse by another
    // thread
    _Py_ClearTLBCIndex((_PyThreadStateImpl *)tstate);
#endif

    // Merge our queue of pointers to be freed into the interpreter queue.
    _PyMem_AbandonDelayed(tstate);

    _PyThreadState_ClearMimallocHeaps(tstate);

    tstate->_status.cleared = 1;

    // XXX Call _PyThreadStateSwap(runtime, NULL) here if "current".
    // XXX Do it as early in the function as possible.
}

static void
decrement_stoptheworld_countdown(struct _stoptheworld_state *stw);

/* Common code for PyThreadState_Delete() and PyThreadState_DeleteCurrent() */
static void
tstate_delete_common(PyThreadState *tstate, int release_gil)
{
    assert(tstate->_status.cleared && !tstate->_status.finalized);
    tstate_verify_not_active(tstate);
    assert(!_PyThreadState_IsRunningMain(tstate));

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
        interp->threads.head = tstate->next;
    }
    if (tstate->next) {
        tstate->next->prev = tstate->prev;
    }
    if (tstate->state != _Py_THREAD_SUSPENDED) {
        // Any ongoing stop-the-world request should not wait for us because
        // our thread is getting deleted.
        if (interp->stoptheworld.requested) {
            decrement_stoptheworld_countdown(&interp->stoptheworld);
        }
        if (runtime->stoptheworld.requested) {
            decrement_stoptheworld_countdown(&runtime->stoptheworld);
        }
    }

#if defined(Py_REF_DEBUG) && defined(Py_GIL_DISABLED)
    // Add our portion of the total refcount to the interpreter's total.
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    tstate->interp->object_state.reftotal += tstate_impl->reftotal;
    tstate_impl->reftotal = 0;
    assert(tstate_impl->refcounts.values == NULL);
#endif

    HEAD_UNLOCK(runtime);

    // XXX Unbind in PyThreadState_Clear(), or earlier
    // (and assert not-equal here)?
    if (tstate->_status.bound_gilstate) {
        unbind_gilstate_tstate(tstate);
    }
    if (tstate->_status.bound) {
        unbind_tstate(tstate);
    }

    // XXX Move to PyThreadState_Clear()?
    clear_datastack(tstate);

    if (release_gil) {
        _PyEval_ReleaseLock(tstate->interp, tstate, 1);
    }

#ifdef Py_GIL_DISABLED
    _Py_qsbr_unregister(tstate);
#endif

    tstate->_status.finalized = 1;
}

static void
zapthreads(PyInterpreterState *interp)
{
    PyThreadState *tstate;
    /* No need to lock the mutex here because this should only happen
       when the threads are all really dead (XXX famous last words).

       Cannot use _Py_FOR_EACH_TSTATE_UNLOCKED because we are freeing
       the thread states here.
    */
    while ((tstate = interp->threads.head) != NULL) {
        tstate_verify_not_active(tstate);
        tstate_delete_common(tstate, 0);
        free_threadstate((_PyThreadStateImpl *)tstate);
    }
}


void
PyThreadState_Delete(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);
    tstate_verify_not_active(tstate);
    tstate_delete_common(tstate, 0);
    free_threadstate((_PyThreadStateImpl *)tstate);
}


void
_PyThreadState_DeleteCurrent(PyThreadState *tstate)
{
    _Py_EnsureTstateNotNULL(tstate);
#ifdef Py_GIL_DISABLED
    _Py_qsbr_detach(((_PyThreadStateImpl *)tstate)->qsbr);
#endif
    current_fast_clear(tstate->interp->runtime);
    tstate_delete_common(tstate, 1);  // release GIL as part of call
    free_threadstate((_PyThreadStateImpl *)tstate);
}

void
PyThreadState_DeleteCurrent(void)
{
    PyThreadState *tstate = current_fast_get();
    _PyThreadState_DeleteCurrent(tstate);
}


// Unlinks and removes all thread states from `tstate->interp`, with the
// exception of the one passed as an argument. However, it does not delete
// these thread states. Instead, it returns the removed thread states as a
// linked list.
//
// Note that if there is a current thread state, it *must* be the one
// passed as argument.  Also, this won't touch any interpreters other
// than the current one, since we don't know which thread state should
// be kept in those other interpreters.
PyThreadState *
_PyThreadState_RemoveExcept(PyThreadState *tstate)
{
    assert(tstate != NULL);
    PyInterpreterState *interp = tstate->interp;
    _PyRuntimeState *runtime = interp->runtime;

#ifdef Py_GIL_DISABLED
    assert(runtime->stoptheworld.world_stopped);
#endif

    HEAD_LOCK(runtime);
    /* Remove all thread states, except tstate, from the linked list of
       thread states. */
    PyThreadState *list = interp->threads.head;
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
    interp->threads.head = tstate;
    HEAD_UNLOCK(runtime);

    return list;
}

// Deletes the thread states in the linked list `list`.
//
// This is intended to be used in conjunction with _PyThreadState_RemoveExcept.
//
// If `is_after_fork` is true, the thread states are immediately freed.
// Otherwise, they are decref'd because they may still be referenced by an
// OS thread.
void
_PyThreadState_DeleteList(PyThreadState *list, int is_after_fork)
{
    // The world can't be stopped because we PyThreadState_Clear() can
    // call destructors.
    assert(!_PyRuntime.stoptheworld.world_stopped);

    PyThreadState *p, *next;
    for (p = list; p; p = next) {
        next = p->next;
        PyThreadState_Clear(p);
        if (is_after_fork) {
            free_threadstate((_PyThreadStateImpl *)p);
        }
        else {
            decref_threadstate((_PyThreadStateImpl *)p);
        }
    }
}


//----------
// accessors
//----------

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
    PyThreadState *tstate = current_fast_get();
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
    _PyInterpreterFrame *f = _PyThreadState_GetFrame(tstate);
    if (f == NULL) {
        return NULL;
    }
    PyFrameObject *frame = _PyFrame_GetFrameObject(f);
    if (frame == NULL) {
        PyErr_Clear();
    }
    return (PyFrameObject*)Py_XNewRef(frame);
}


uint64_t
PyThreadState_GetID(PyThreadState *tstate)
{
    assert(tstate != NULL);
    return tstate->id;
}


static inline void
tstate_activate(PyThreadState *tstate)
{
    assert(tstate != NULL);
    // XXX assert(tstate_is_alive(tstate));
    assert(tstate_is_bound(tstate));
    assert(!tstate->_status.active);

    assert(!tstate->_status.bound_gilstate ||
           tstate == gilstate_get());
    if (!tstate->_status.bound_gilstate) {
        bind_gilstate_tstate(tstate);
    }

    tstate->_status.active = 1;
}

static inline void
tstate_deactivate(PyThreadState *tstate)
{
    assert(tstate != NULL);
    // XXX assert(tstate_is_alive(tstate));
    assert(tstate_is_bound(tstate));
    assert(tstate->_status.active);

    tstate->_status.active = 0;

    // We do not unbind the gilstate tstate here.
    // It will still be used in PyGILState_Ensure().
}

static int
tstate_try_attach(PyThreadState *tstate)
{
#ifdef Py_GIL_DISABLED
    int expected = _Py_THREAD_DETACHED;
    return _Py_atomic_compare_exchange_int(&tstate->state,
                                           &expected,
                                           _Py_THREAD_ATTACHED);
#else
    assert(tstate->state == _Py_THREAD_DETACHED);
    tstate->state = _Py_THREAD_ATTACHED;
    return 1;
#endif
}

static void
tstate_set_detached(PyThreadState *tstate, int detached_state)
{
    assert(_Py_atomic_load_int_relaxed(&tstate->state) == _Py_THREAD_ATTACHED);
#ifdef Py_GIL_DISABLED
    _Py_atomic_store_int(&tstate->state, detached_state);
#else
    tstate->state = detached_state;
#endif
}

static void
tstate_wait_attach(PyThreadState *tstate)
{
    do {
        int state = _Py_atomic_load_int_relaxed(&tstate->state);
        if (state == _Py_THREAD_SUSPENDED) {
            // Wait until we're switched out of SUSPENDED to DETACHED.
            _PyParkingLot_Park(&tstate->state, &state, sizeof(tstate->state),
                               /*timeout=*/-1, NULL, /*detach=*/0);
        }
        else if (state == _Py_THREAD_SHUTTING_DOWN) {
            // We're shutting down, so we can't attach.
            _PyThreadState_HangThread(tstate);
        }
        else {
            assert(state == _Py_THREAD_DETACHED);
        }
        // Once we're back in DETACHED we can re-attach
    } while (!tstate_try_attach(tstate));
}

void
_PyThreadState_Attach(PyThreadState *tstate)
{
#if defined(Py_DEBUG)
    // This is called from PyEval_RestoreThread(). Similar
    // to it, we need to ensure errno doesn't change.
    int err = errno;
#endif

    _Py_EnsureTstateNotNULL(tstate);
    if (current_fast_get() != NULL) {
        Py_FatalError("non-NULL old thread state");
    }
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    if (_tstate->c_stack_hard_limit == 0) {
        _Py_InitializeRecursionLimits(tstate);
    }

    while (1) {
        _PyEval_AcquireLock(tstate);

        // XXX assert(tstate_is_alive(tstate));
        current_fast_set(&_PyRuntime, tstate);
        if (!tstate_try_attach(tstate)) {
            tstate_wait_attach(tstate);
        }
        tstate_activate(tstate);

#ifdef Py_GIL_DISABLED
        if (_PyEval_IsGILEnabled(tstate) && !tstate->holds_gil) {
            // The GIL was enabled between our call to _PyEval_AcquireLock()
            // and when we attached (the GIL can't go from enabled to disabled
            // here because only a thread holding the GIL can disable
            // it). Detach and try again.
            tstate_set_detached(tstate, _Py_THREAD_DETACHED);
            tstate_deactivate(tstate);
            current_fast_clear(&_PyRuntime);
            continue;
        }
        _Py_qsbr_attach(((_PyThreadStateImpl *)tstate)->qsbr);
#endif
        break;
    }

    // Resume previous critical section. This acquires the lock(s) from the
    // top-most critical section.
    if (tstate->critical_section != 0) {
        _PyCriticalSection_Resume(tstate);
    }

#if defined(Py_DEBUG)
    errno = err;
#endif
}

static void
detach_thread(PyThreadState *tstate, int detached_state)
{
    // XXX assert(tstate_is_alive(tstate) && tstate_is_bound(tstate));
    assert(_Py_atomic_load_int_relaxed(&tstate->state) == _Py_THREAD_ATTACHED);
    assert(tstate == current_fast_get());
    if (tstate->critical_section != 0) {
        _PyCriticalSection_SuspendAll(tstate);
    }
#ifdef Py_GIL_DISABLED
    _Py_qsbr_detach(((_PyThreadStateImpl *)tstate)->qsbr);
#endif
    tstate_deactivate(tstate);
    tstate_set_detached(tstate, detached_state);
    current_fast_clear(&_PyRuntime);
    _PyEval_ReleaseLock(tstate->interp, tstate, 0);
}

void
_PyThreadState_Detach(PyThreadState *tstate)
{
    detach_thread(tstate, _Py_THREAD_DETACHED);
}

void
_PyThreadState_Suspend(PyThreadState *tstate)
{
    _PyRuntimeState *runtime = &_PyRuntime;

    assert(_Py_atomic_load_int_relaxed(&tstate->state) == _Py_THREAD_ATTACHED);

    struct _stoptheworld_state *stw = NULL;
    HEAD_LOCK(runtime);
    if (runtime->stoptheworld.requested) {
        stw = &runtime->stoptheworld;
    }
    else if (tstate->interp->stoptheworld.requested) {
        stw = &tstate->interp->stoptheworld;
    }
    HEAD_UNLOCK(runtime);

    if (stw == NULL) {
        // Switch directly to "detached" if there is no active stop-the-world
        // request.
        detach_thread(tstate, _Py_THREAD_DETACHED);
        return;
    }

    // Switch to "suspended" state.
    detach_thread(tstate, _Py_THREAD_SUSPENDED);

    // Decrease the count of remaining threads needing to park.
    HEAD_LOCK(runtime);
    decrement_stoptheworld_countdown(stw);
    HEAD_UNLOCK(runtime);
}

void
_PyThreadState_SetShuttingDown(PyThreadState *tstate)
{
    _Py_atomic_store_int(&tstate->state, _Py_THREAD_SHUTTING_DOWN);
#ifdef Py_GIL_DISABLED
    _PyParkingLot_UnparkAll(&tstate->state);
#endif
}

// Decrease stop-the-world counter of remaining number of threads that need to
// pause. If we are the final thread to pause, notify the requesting thread.
static void
decrement_stoptheworld_countdown(struct _stoptheworld_state *stw)
{
    assert(stw->thread_countdown > 0);
    if (--stw->thread_countdown == 0) {
        _PyEvent_Notify(&stw->stop_event);
    }
}

#ifdef Py_GIL_DISABLED
// Interpreter for _Py_FOR_EACH_STW_INTERP(). For global stop-the-world events,
// we start with the first interpreter and then iterate over all interpreters.
// For per-interpreter stop-the-world events, we only operate on the one
// interpreter.
static PyInterpreterState *
interp_for_stop_the_world(struct _stoptheworld_state *stw)
{
    return (stw->is_global
        ? PyInterpreterState_Head()
        : _Py_CONTAINER_OF(stw, PyInterpreterState, stoptheworld));
}

// Loops over threads for a stop-the-world event.
// For global: all threads in all interpreters
// For per-interpreter: all threads in the interpreter
#define _Py_FOR_EACH_STW_INTERP(stw, i)                                     \
    for (PyInterpreterState *i = interp_for_stop_the_world((stw));          \
            i != NULL; i = ((stw->is_global) ? i->next : NULL))


// Try to transition threads atomically from the "detached" state to the
// "gc stopped" state. Returns true if all threads are in the "gc stopped"
static bool
park_detached_threads(struct _stoptheworld_state *stw)
{
    int num_parked = 0;
    _Py_FOR_EACH_STW_INTERP(stw, i) {
        _Py_FOR_EACH_TSTATE_UNLOCKED(i, t) {
            int state = _Py_atomic_load_int_relaxed(&t->state);
            if (state == _Py_THREAD_DETACHED) {
                // Atomically transition to "suspended" if in "detached" state.
                if (_Py_atomic_compare_exchange_int(
                                &t->state, &state, _Py_THREAD_SUSPENDED)) {
                    num_parked++;
                }
            }
            else if (state == _Py_THREAD_ATTACHED && t != stw->requester) {
                _Py_set_eval_breaker_bit(t, _PY_EVAL_PLEASE_STOP_BIT);
            }
        }
    }
    stw->thread_countdown -= num_parked;
    assert(stw->thread_countdown >= 0);
    return num_parked > 0 && stw->thread_countdown == 0;
}

static void
stop_the_world(struct _stoptheworld_state *stw)
{
    _PyRuntimeState *runtime = &_PyRuntime;

    PyMutex_Lock(&stw->mutex);
    if (stw->is_global) {
        _PyRWMutex_Lock(&runtime->stoptheworld_mutex);
    }
    else {
        _PyRWMutex_RLock(&runtime->stoptheworld_mutex);
    }

    HEAD_LOCK(runtime);
    stw->requested = 1;
    stw->thread_countdown = 0;
    stw->stop_event = (PyEvent){0};  // zero-initialize (unset)
    stw->requester = _PyThreadState_GET();  // may be NULL

    _Py_FOR_EACH_STW_INTERP(stw, i) {
        _Py_FOR_EACH_TSTATE_UNLOCKED(i, t) {
            if (t != stw->requester) {
                // Count all the other threads (we don't wait on ourself).
                stw->thread_countdown++;
            }
        }
    }

    if (stw->thread_countdown == 0) {
        HEAD_UNLOCK(runtime);
        stw->world_stopped = 1;
        return;
    }

    for (;;) {
        // Switch threads that are detached to the GC stopped state
        bool stopped_all_threads = park_detached_threads(stw);
        HEAD_UNLOCK(runtime);

        if (stopped_all_threads) {
            break;
        }

        PyTime_t wait_ns = 1000*1000;  // 1ms (arbitrary, may need tuning)
        int detach = 0;
        if (PyEvent_WaitTimed(&stw->stop_event, wait_ns, detach)) {
            assert(stw->thread_countdown == 0);
            break;
        }

        HEAD_LOCK(runtime);
    }
    stw->world_stopped = 1;
}

static void
start_the_world(struct _stoptheworld_state *stw)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    assert(PyMutex_IsLocked(&stw->mutex));

    HEAD_LOCK(runtime);
    stw->requested = 0;
    stw->world_stopped = 0;
    // Switch threads back to the detached state.
    _Py_FOR_EACH_STW_INTERP(stw, i) {
        _Py_FOR_EACH_TSTATE_UNLOCKED(i, t) {
            if (t != stw->requester) {
                assert(_Py_atomic_load_int_relaxed(&t->state) ==
                       _Py_THREAD_SUSPENDED);
                _Py_atomic_store_int(&t->state, _Py_THREAD_DETACHED);
                _PyParkingLot_UnparkAll(&t->state);
            }
        }
    }
    stw->requester = NULL;
    HEAD_UNLOCK(runtime);
    if (stw->is_global) {
        _PyRWMutex_Unlock(&runtime->stoptheworld_mutex);
    }
    else {
        _PyRWMutex_RUnlock(&runtime->stoptheworld_mutex);
    }
    PyMutex_Unlock(&stw->mutex);
}
#endif  // Py_GIL_DISABLED

void
_PyEval_StopTheWorldAll(_PyRuntimeState *runtime)
{
#ifdef Py_GIL_DISABLED
    stop_the_world(&runtime->stoptheworld);
#endif
}

void
_PyEval_StartTheWorldAll(_PyRuntimeState *runtime)
{
#ifdef Py_GIL_DISABLED
    start_the_world(&runtime->stoptheworld);
#endif
}

void
_PyEval_StopTheWorld(PyInterpreterState *interp)
{
#ifdef Py_GIL_DISABLED
    stop_the_world(&interp->stoptheworld);
#endif
}

void
_PyEval_StartTheWorld(PyInterpreterState *interp)
{
#ifdef Py_GIL_DISABLED
    start_the_world(&interp->stoptheworld);
#endif
}

//----------
// other API
//----------

/* Asynchronously raise an exception in a thread.
   Requested by Just van Rossum and Alex Martelli.
   To prevent naive misuse, you must write your own extension
   to call this, or use ctypes.  Must be called with the GIL held.
   Returns the number of tstates modified (normally 1, but 0 if `id` didn't
   match any known thread id).  Can be called with exc=NULL to clear an
   existing async exception.  This raises no exceptions. */

// XXX Move this to Python/ceval_gil.c?
// XXX Deprecate this.
int
PyThreadState_SetAsyncExc(unsigned long id, PyObject *exc)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();

    /* Although the GIL is held, a few C API functions can be called
     * without the GIL held, and in particular some that create and
     * destroy thread and interpreter states.  Those can mutate the
     * list of thread states we're traversing, so to prevent that we lock
     * head_mutex for the duration.
     */
    PyThreadState *tstate = NULL;
    _Py_FOR_EACH_TSTATE_BEGIN(interp, t) {
        if (t->thread_id == id) {
            tstate = t;
            break;
        }
    }
    _Py_FOR_EACH_TSTATE_END(interp);

    if (tstate != NULL) {
        /* Tricky:  we need to decref the current value
         * (if any) in tstate->async_exc, but that can in turn
         * allow arbitrary Python code to run, including
         * perhaps calls to this function.  To prevent
         * deadlock, we need to release head_mutex before
         * the decref.
         */
        Py_XINCREF(exc);
        PyObject *old_exc = _Py_atomic_exchange_ptr(&tstate->async_exc, exc);

        Py_XDECREF(old_exc);
        _Py_set_eval_breaker_bit(tstate, _PY_ASYNC_EXCEPTION_BIT);
    }

    return tstate != NULL;
}

//---------------------------------
// API for the current thread state
//---------------------------------

PyThreadState *
PyThreadState_GetUnchecked(void)
{
    return current_fast_get();
}


PyThreadState *
PyThreadState_Get(void)
{
    PyThreadState *tstate = current_fast_get();
    _Py_EnsureTstateNotNULL(tstate);
    return tstate;
}

PyThreadState *
_PyThreadState_Swap(_PyRuntimeState *runtime, PyThreadState *newts)
{
    PyThreadState *oldts = current_fast_get();
    if (oldts != NULL) {
        _PyThreadState_Detach(oldts);
    }
    if (newts != NULL) {
        _PyThreadState_Attach(newts);
    }
    return oldts;
}

PyThreadState *
PyThreadState_Swap(PyThreadState *newts)
{
    return _PyThreadState_Swap(&_PyRuntime, newts);
}


void
_PyThreadState_Bind(PyThreadState *tstate)
{
    // gh-104690: If Python is being finalized and PyInterpreterState_Delete()
    // was called, tstate becomes a dangling pointer.
    assert(_PyThreadState_CheckConsistency(tstate));

    bind_tstate(tstate);
    // This makes sure there's a gilstate tstate bound
    // as soon as possible.
    if (gilstate_get() == NULL) {
        bind_gilstate_tstate(tstate);
    }
}

#if defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
uintptr_t
_Py_GetThreadLocal_Addr(void)
{
#ifdef HAVE_THREAD_LOCAL
    // gh-112535: Use the address of the thread-local PyThreadState variable as
    // a unique identifier for the current thread. Each thread has a unique
    // _Py_tss_tstate variable with a unique address.
    return (uintptr_t)&_Py_tss_tstate;
#else
#  error "no supported thread-local variable storage classifier"
#endif
}
#endif

/***********************************/
/* routines for advanced debuggers */
/***********************************/

// (requested by David Beazley)
// Don't use unless you know what you are doing!

PyInterpreterState *
PyInterpreterState_Head(void)
{
    return _PyRuntime.interpreters.head;
}

PyInterpreterState *
PyInterpreterState_Main(void)
{
    return _PyInterpreterState_Main();
}

PyInterpreterState *
PyInterpreterState_Next(PyInterpreterState *interp) {
    return interp->next;
}

PyThreadState *
PyInterpreterState_ThreadHead(PyInterpreterState *interp) {
    return interp->threads.head;
}

PyThreadState *
PyThreadState_Next(PyThreadState *tstate) {
    return tstate->next;
}


/********************************************/
/* reporting execution state of all threads */
/********************************************/

/* The implementation of sys._current_frames().  This is intended to be
   called with the GIL held, as it will be when called via
   sys._current_frames().  It's possible it would work fine even without
   the GIL held, but haven't thought enough about that.
*/
PyObject *
_PyThread_CurrentFrames(void)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    PyThreadState *tstate = current_fast_get();
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
    _PyEval_StopTheWorldAll(runtime);
    HEAD_LOCK(runtime);
    PyInterpreterState *i;
    for (i = runtime->interpreters.head; i != NULL; i = i->next) {
        _Py_FOR_EACH_TSTATE_UNLOCKED(i, t) {
            _PyInterpreterFrame *frame = t->current_frame;
            frame = _PyFrame_GetFirstComplete(frame);
            if (frame == NULL) {
                continue;
            }
            PyObject *id = PyLong_FromUnsignedLong(t->thread_id);
            if (id == NULL) {
                goto fail;
            }
            PyObject *frameobj = (PyObject *)_PyFrame_GetFrameObject(frame);
            if (frameobj == NULL) {
                Py_DECREF(id);
                goto fail;
            }
            int stat = PyDict_SetItem(result, id, frameobj);
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
    _PyEval_StartTheWorldAll(runtime);
    return result;
}

/* The implementation of sys._current_exceptions().  This is intended to be
   called with the GIL held, as it will be when called via
   sys._current_exceptions().  It's possible it would work fine even without
   the GIL held, but haven't thought enough about that.
*/
PyObject *
_PyThread_CurrentExceptions(void)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    PyThreadState *tstate = current_fast_get();

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
    _PyEval_StopTheWorldAll(runtime);
    HEAD_LOCK(runtime);
    PyInterpreterState *i;
    for (i = runtime->interpreters.head; i != NULL; i = i->next) {
        _Py_FOR_EACH_TSTATE_UNLOCKED(i, t) {
            _PyErr_StackItem *err_info = _PyErr_GetTopmostException(t);
            if (err_info == NULL) {
                continue;
            }
            PyObject *id = PyLong_FromUnsignedLong(t->thread_id);
            if (id == NULL) {
                goto fail;
            }
            PyObject *exc = err_info->exc_value;
            assert(exc == NULL ||
                   exc == Py_None ||
                   PyExceptionInstance_Check(exc));

            int stat = PyDict_SetItem(result, id, exc == NULL ? Py_None : exc);
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
    _PyEval_StartTheWorldAll(runtime);
    return result;
}


/***********************************/
/* Python "auto thread state" API. */
/***********************************/

/* Internal initialization/finalization functions called by
   Py_Initialize/Py_FinalizeEx
*/
PyStatus
_PyGILState_Init(PyInterpreterState *interp)
{
    if (!_Py_IsMainInterpreter(interp)) {
        /* Currently, PyGILState is shared by all interpreters. The main
         * interpreter is responsible to initialize it. */
        return _PyStatus_OK();
    }
    _PyRuntimeState *runtime = interp->runtime;
    assert(gilstate_get() == NULL);
    assert(runtime->gilstate.autoInterpreterState == NULL);
    runtime->gilstate.autoInterpreterState = interp;
    return _PyStatus_OK();
}

void
_PyGILState_Fini(PyInterpreterState *interp)
{
    if (!_Py_IsMainInterpreter(interp)) {
        /* Currently, PyGILState is shared by all interpreters. The main
         * interpreter is responsible to initialize it. */
        return;
    }
    interp->runtime->gilstate.autoInterpreterState = NULL;
}


// XXX Drop this.
void
_PyGILState_SetTstate(PyThreadState *tstate)
{
    /* must init with valid states */
    assert(tstate != NULL);
    assert(tstate->interp != NULL);

    if (!_Py_IsMainInterpreter(tstate->interp)) {
        /* Currently, PyGILState is shared by all interpreters. The main
         * interpreter is responsible to initialize it. */
        return;
    }

#ifndef NDEBUG
    _PyRuntimeState *runtime = tstate->interp->runtime;

    assert(runtime->gilstate.autoInterpreterState == tstate->interp);
    assert(gilstate_get() == tstate);
    assert(tstate->gilstate_counter == 1);
#endif
}

PyInterpreterState *
_PyGILState_GetInterpreterStateUnsafe(void)
{
    return _PyRuntime.gilstate.autoInterpreterState;
}

/* The public functions */

PyThreadState *
PyGILState_GetThisThreadState(void)
{
    return gilstate_get();
}

int
PyGILState_Check(void)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    if (!runtime->gilstate.check_enabled) {
        return 1;
    }

    PyThreadState *tstate = current_fast_get();
    if (tstate == NULL) {
        return 0;
    }

    PyThreadState *tcur = gilstate_get();
    return (tstate == tcur);
}

PyGILState_STATE
PyGILState_Ensure(void)
{
    _PyRuntimeState *runtime = &_PyRuntime;

    /* Note that we do not auto-init Python here - apart from
       potential races with 2 threads auto-initializing, pep-311
       spells out other issues.  Embedders are expected to have
       called Py_Initialize(). */

    /* Ensure that _PyEval_InitThreads() and _PyGILState_Init() have been
       called by Py_Initialize()

       TODO: This isn't thread-safe. There's no protection here against
       concurrent finalization of the interpreter; it's simply a guard
       for *after* the interpreter has finalized.
     */
    if (!_PyEval_ThreadsInitialized() || runtime->gilstate.autoInterpreterState == NULL) {
        PyThread_hang_thread();
    }

    PyThreadState *tcur = gilstate_get();
    int has_gil;
    if (tcur == NULL) {
        /* Create a new Python thread state for this thread */
        // XXX Use PyInterpreterState_EnsureThreadState()?
        tcur = new_threadstate(runtime->gilstate.autoInterpreterState,
                               _PyThreadState_WHENCE_GILSTATE);
        if (tcur == NULL) {
            Py_FatalError("Couldn't create thread-state for new thread");
        }
        bind_tstate(tcur);
        bind_gilstate_tstate(tcur);

        /* This is our thread state!  We'll need to delete it in the
           matching call to PyGILState_Release(). */
        assert(tcur->gilstate_counter == 1);
        tcur->gilstate_counter = 0;
        has_gil = 0; /* new thread state is never current */
    }
    else {
        has_gil = holds_gil(tcur);
    }

    if (!has_gil) {
        PyEval_RestoreThread(tcur);
    }

    /* Update our counter in the thread-state - no need for locks:
       - tcur will remain valid as we hold the GIL.
       - the counter is safe as we are the only thread "allowed"
         to modify this value
    */
    ++tcur->gilstate_counter;

    return has_gil ? PyGILState_LOCKED : PyGILState_UNLOCKED;
}

void
PyGILState_Release(PyGILState_STATE oldstate)
{
    PyThreadState *tstate = gilstate_get();
    if (tstate == NULL) {
        Py_FatalError("auto-releasing thread-state, "
                      "but no thread-state for this thread");
    }

    /* We must hold the GIL and have our thread state current */
    if (!holds_gil(tstate)) {
        _Py_FatalErrorFormat(__func__,
                             "thread state %p must be current when releasing",
                             tstate);
    }
    --tstate->gilstate_counter;
    assert(tstate->gilstate_counter >= 0); /* illegal counter value */

    /* If we're going to destroy this thread-state, we must
     * clear it while the GIL is held, as destructors may run.
     */
    if (tstate->gilstate_counter == 0) {
        /* can't have been locked when we created it */
        assert(oldstate == PyGILState_UNLOCKED);
        // XXX Unbind tstate here.
        // gh-119585: `PyThreadState_Clear()` may call destructors that
        // themselves use PyGILState_Ensure and PyGILState_Release, so make
        // sure that gilstate_counter is not zero when calling it.
        ++tstate->gilstate_counter;
        PyThreadState_Clear(tstate);
        --tstate->gilstate_counter;
        /* Delete the thread-state.  Note this releases the GIL too!
         * It's vital that the GIL be held here, to avoid shutdown
         * races; see bugs 225673 and 1061968 (that nasty bug has a
         * habit of coming back).
         */
        assert(tstate->gilstate_counter == 0);
        assert(current_fast_get() == tstate);
        _PyThreadState_DeleteCurrent(tstate);
    }
    /* Release the lock if necessary */
    else if (oldstate == PyGILState_UNLOCKED) {
        PyEval_SaveThread();
    }
}


/*************/
/* Other API */
/*************/

_PyFrameEvalFunction
_PyInterpreterState_GetEvalFrameFunc(PyInterpreterState *interp)
{
    if (interp->eval_frame == NULL) {
        return _PyEval_EvalFrameDefault;
    }
    return interp->eval_frame;
}


void
_PyInterpreterState_SetEvalFrameFunc(PyInterpreterState *interp,
                                     _PyFrameEvalFunction eval_frame)
{
    if (eval_frame == _PyEval_EvalFrameDefault) {
        eval_frame = NULL;
    }
    if (eval_frame == interp->eval_frame) {
        return;
    }
#ifdef _Py_TIER2
    if (eval_frame != NULL) {
        _Py_Executors_InvalidateAll(interp, 1);
    }
#endif
    RARE_EVENT_INC(set_eval_frame_func);
    _PyEval_StopTheWorld(interp);
    interp->eval_frame = eval_frame;
    _PyEval_StartTheWorld(interp);
}


const PyConfig*
_PyInterpreterState_GetConfig(PyInterpreterState *interp)
{
    return &interp->config;
}


const PyConfig*
_Py_GetConfig(void)
{
    PyThreadState *tstate = current_fast_get();
    _Py_EnsureTstateNotNULL(tstate);
    return _PyInterpreterState_GetConfig(tstate->interp);
}


int
_PyInterpreterState_HasFeature(PyInterpreterState *interp, unsigned long feature)
{
    return ((interp->feature_flags & feature) != 0);
}


#define MINIMUM_OVERHEAD 1000

static PyObject **
push_chunk(PyThreadState *tstate, int size)
{
    int allocate_size = DATA_STACK_CHUNK_SIZE;
    while (allocate_size < (int)sizeof(PyObject*)*(size + MINIMUM_OVERHEAD)) {
        allocate_size *= 2;
    }
    _PyStackChunk *new = allocate_chunk(allocate_size, tstate->datastack_chunk);
    if (new == NULL) {
        return NULL;
    }
    if (tstate->datastack_chunk) {
        tstate->datastack_chunk->top = tstate->datastack_top -
                                       &tstate->datastack_chunk->data[0];
    }
    tstate->datastack_chunk = new;
    tstate->datastack_limit = (PyObject **)(((char *)new) + allocate_size);
    // When new is the "root" chunk (i.e. new->previous == NULL), we can keep
    // _PyThreadState_PopFrame from freeing it later by "skipping" over the
    // first element:
    PyObject **res = &new->data[new->previous == NULL];
    tstate->datastack_top = res + size;
    return res;
}

_PyInterpreterFrame *
_PyThreadState_PushFrame(PyThreadState *tstate, size_t size)
{
    assert(size < INT_MAX/sizeof(PyObject *));
    if (_PyThreadState_HasStackSpace(tstate, (int)size)) {
        _PyInterpreterFrame *res = (_PyInterpreterFrame *)tstate->datastack_top;
        tstate->datastack_top += size;
        return res;
    }
    return (_PyInterpreterFrame *)push_chunk(tstate, (int)size);
}

void
_PyThreadState_PopFrame(PyThreadState *tstate, _PyInterpreterFrame * frame)
{
    assert(tstate->datastack_chunk);
    PyObject **base = (PyObject **)frame;
    if (base == &tstate->datastack_chunk->data[0]) {
        _PyStackChunk *chunk = tstate->datastack_chunk;
        _PyStackChunk *previous = chunk->previous;
        // push_chunk ensures that the root chunk is never popped:
        assert(previous);
        tstate->datastack_top = &previous->data[previous->top];
        tstate->datastack_chunk = previous;
        _PyObject_VirtualFree(chunk, chunk->size);
        tstate->datastack_limit = (PyObject **)(((char *)previous) + previous->size);
    }
    else {
        assert(tstate->datastack_top);
        assert(tstate->datastack_top >= base);
        tstate->datastack_top = base;
    }
}


#ifndef NDEBUG
// Check that a Python thread state valid. In practice, this function is used
// on a Python debug build to check if 'tstate' is a dangling pointer, if the
// PyThreadState memory has been freed.
//
// Usage:
//
//     assert(_PyThreadState_CheckConsistency(tstate));
int
_PyThreadState_CheckConsistency(PyThreadState *tstate)
{
    assert(!_PyMem_IsPtrFreed(tstate));
    assert(!_PyMem_IsPtrFreed(tstate->interp));
    return 1;
}
#endif


// Check if a Python thread must call _PyThreadState_HangThread(), rather than
// taking the GIL or attaching to the interpreter if Py_Finalize() has been
// called.
//
// When this function is called by a daemon thread after Py_Finalize() has been
// called, the GIL may no longer exist.
//
// tstate must be non-NULL.
int
_PyThreadState_MustExit(PyThreadState *tstate)
{
    int state = _Py_atomic_load_int_relaxed(&tstate->state);
    return state == _Py_THREAD_SHUTTING_DOWN;
}

void
_PyThreadState_HangThread(PyThreadState *tstate)
{
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    decref_threadstate(tstate_impl);
    PyThread_hang_thread();
}

/********************/
/* mimalloc support */
/********************/

static void
tstate_mimalloc_bind(PyThreadState *tstate)
{
#ifdef Py_GIL_DISABLED
    struct _mimalloc_thread_state *mts = &((_PyThreadStateImpl*)tstate)->mimalloc;

    // Initialize the mimalloc thread state. This must be called from the
    // same thread that will use the thread state. The "mem" heap doubles as
    // the "backing" heap.
    mi_tld_t *tld = &mts->tld;
    _mi_tld_init(tld, &mts->heaps[_Py_MIMALLOC_HEAP_MEM]);
    llist_init(&mts->page_list);

    // Exiting threads push any remaining in-use segments to the abandoned
    // pool to be re-claimed later by other threads. We use per-interpreter
    // pools to keep Python objects from different interpreters separate.
    tld->segments.abandoned = &tstate->interp->mimalloc.abandoned_pool;

    // Don't fill in the first N bytes up to ob_type in debug builds. We may
    // access ob_tid and the refcount fields in the dict and list lock-less
    // accesses, so they must remain valid for a while after deallocation.
    size_t base_offset = offsetof(PyObject, ob_type);
    if (_PyMem_DebugEnabled()) {
        // The debug allocator adds two words at the beginning of each block.
        base_offset += 2 * sizeof(size_t);
    }
    size_t debug_offsets[_Py_MIMALLOC_HEAP_COUNT] = {
        [_Py_MIMALLOC_HEAP_OBJECT] = base_offset,
        [_Py_MIMALLOC_HEAP_GC] = base_offset,
        [_Py_MIMALLOC_HEAP_GC_PRE] = base_offset + 2 * sizeof(PyObject *),
    };

    // Initialize each heap
    for (uint8_t i = 0; i < _Py_MIMALLOC_HEAP_COUNT; i++) {
        _mi_heap_init_ex(&mts->heaps[i], tld, _mi_arena_id_none(), false, i);
        mts->heaps[i].debug_offset = (uint8_t)debug_offsets[i];
    }

    // Heaps that store Python objects should use QSBR to delay freeing
    // mimalloc pages while there may be concurrent lock-free readers.
    mts->heaps[_Py_MIMALLOC_HEAP_OBJECT].page_use_qsbr = true;
    mts->heaps[_Py_MIMALLOC_HEAP_GC].page_use_qsbr = true;
    mts->heaps[_Py_MIMALLOC_HEAP_GC_PRE].page_use_qsbr = true;

    // By default, object allocations use _Py_MIMALLOC_HEAP_OBJECT.
    // _PyObject_GC_New() and similar functions temporarily override this to
    // use one of the GC heaps.
    mts->current_object_heap = &mts->heaps[_Py_MIMALLOC_HEAP_OBJECT];

    _Py_atomic_store_int(&mts->initialized, 1);
#endif
}

void
_PyThreadState_ClearMimallocHeaps(PyThreadState *tstate)
{
#ifdef Py_GIL_DISABLED
    if (!tstate->_status.bound) {
        // The mimalloc heaps are only initialized when the thread is bound.
        return;
    }

    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    for (Py_ssize_t i = 0; i < _Py_MIMALLOC_HEAP_COUNT; i++) {
        // Abandon all segments in use by this thread. This pushes them to
        // a shared pool to later be reclaimed by other threads. It's important
        // to do this before the thread state is destroyed so that objects
        // remain visible to the GC.
        _mi_heap_collect_abandon(&tstate_impl->mimalloc.heaps[i]);
    }
#endif
}


int
_Py_IsMainThread(void)
{
    unsigned long thread = PyThread_get_thread_ident();
    return (thread == _PyRuntime.main_thread);
}


PyInterpreterState *
_PyInterpreterState_Main(void)
{
    return _PyRuntime.interpreters.main;
}


int
_Py_IsMainInterpreterFinalizing(PyInterpreterState *interp)
{
    /* bpo-39877: Access _PyRuntime directly rather than using
       tstate->interp->runtime to support calls from Python daemon threads.
       After Py_Finalize() has been called, tstate can be a dangling pointer:
       point to PyThreadState freed memory. */
    return (_PyRuntimeState_GetFinalizing(&_PyRuntime) != NULL &&
            interp == &_PyRuntime._main_interpreter);
}


const PyConfig *
_Py_GetMainConfig(void)
{
    PyInterpreterState *interp = _PyInterpreterState_Main();
    if (interp == NULL) {
        return NULL;
    }
    return _PyInterpreterState_GetConfig(interp);
}
