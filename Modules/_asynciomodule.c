#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_freelist.h"      // _Py_FREELIST_POP()
#include "pycore_genobject.h"
#include "pycore_llist.h"         // struct llist_node
#include "pycore_list.h"          // _PyList_AppendTakeRef()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_object.h"        // _PyObject_SetMaybeWeakref
#include "pycore_pylifecycle.h"   // _Py_IsInterpreterFinalizing()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_runtime_init.h"  // _Py_ID()

#include <stddef.h>               // offsetof()


/*[clinic input]
module _asyncio
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=8fd17862aa989c69]*/

typedef enum {
    STATE_PENDING,
    STATE_CANCELLED,
    STATE_FINISHED
} fut_state;

#define FutureObj_HEAD(prefix)                                              \
    PyObject_HEAD                                                           \
    PyObject *prefix##_loop;                                                \
    PyObject *prefix##_callback0;                                           \
    PyObject *prefix##_context0;                                            \
    PyObject *prefix##_callbacks;                                           \
    PyObject *prefix##_exception;                                           \
    PyObject *prefix##_exception_tb;                                        \
    PyObject *prefix##_result;                                              \
    PyObject *prefix##_source_tb;                                           \
    PyObject *prefix##_cancel_msg;                                          \
    PyObject *prefix##_cancelled_exc;                                       \
    PyObject *prefix##_awaited_by;                                          \
    fut_state prefix##_state;                                               \
    /* Used by profilers to make traversing the stack from an external      \
       process faster. */                                                   \
    char prefix##_is_task;                                                  \
    char prefix##_awaited_by_is_set;                                        \
    /* These bitfields need to be at the end of the struct                  \
       so that these and bitfields from TaskObj are contiguous.             \
    */                                                                      \
    unsigned prefix##_log_tb: 1;                                            \
    unsigned prefix##_blocking: 1;                                          \

typedef struct {
    FutureObj_HEAD(fut)
} FutureObj;

typedef struct TaskObj {
    FutureObj_HEAD(task)
    unsigned task_must_cancel: 1;
    unsigned task_log_destroy_pending: 1;
    int task_num_cancels_requested;
    PyObject *task_fut_waiter;
    PyObject *task_coro;
    PyObject *task_name;
    PyObject *task_context;
    struct llist_node task_node;
#ifdef Py_GIL_DISABLED
    // thread id of the thread where this task was created
    uintptr_t task_tid;
#endif
} TaskObj;

typedef struct {
    PyObject_HEAD
    TaskObj *sw_task;
    PyObject *sw_arg;
} TaskStepMethWrapper;

#define Future_CheckExact(state, obj) Py_IS_TYPE(obj, state->FutureType)
#define Task_CheckExact(state, obj) Py_IS_TYPE(obj, state->TaskType)

#define Future_Check(state, obj)                        \
    (Future_CheckExact(state, obj)                      \
     || PyObject_TypeCheck(obj, state->FutureType))

#define Task_Check(state, obj)                          \
    (Task_CheckExact(state, obj)                        \
     || PyObject_TypeCheck(obj, state->TaskType))

// This macro is optimized to quickly return for native Future *or* Task
// objects by inlining fast "exact" checks to be called first.
#define TaskOrFuture_Check(state, obj)                  \
    (Task_CheckExact(state, obj)                        \
     || Future_CheckExact(state, obj)                   \
     || PyObject_TypeCheck(obj, state->FutureType)      \
     || PyObject_TypeCheck(obj, state->TaskType))

typedef struct _Py_AsyncioModuleDebugOffsets {
    struct _asyncio_task_object {
        uint64_t size;
        uint64_t task_name;
        uint64_t task_awaited_by;
        uint64_t task_is_task;
        uint64_t task_awaited_by_is_set;
        uint64_t task_coro;
        uint64_t task_node;
    } asyncio_task_object;
    struct _asyncio_interpreter_state {
        uint64_t size;
        uint64_t asyncio_tasks_head;
    } asyncio_interpreter_state;
    struct _asyncio_thread_state {
        uint64_t size;
        uint64_t asyncio_running_loop;
        uint64_t asyncio_running_task;
        uint64_t asyncio_tasks_head;
    } asyncio_thread_state;
} Py_AsyncioModuleDebugOffsets;

GENERATE_DEBUG_SECTION(AsyncioDebug, Py_AsyncioModuleDebugOffsets _AsyncioDebug)
    = {.asyncio_task_object = {
           .size = sizeof(TaskObj),
           .task_name = offsetof(TaskObj, task_name),
           .task_awaited_by = offsetof(TaskObj, task_awaited_by),
           .task_is_task = offsetof(TaskObj, task_is_task),
           .task_awaited_by_is_set = offsetof(TaskObj, task_awaited_by_is_set),
           .task_coro = offsetof(TaskObj, task_coro),
           .task_node = offsetof(TaskObj, task_node),
       },
       .asyncio_interpreter_state = {
            .size = sizeof(PyInterpreterState),
            .asyncio_tasks_head = offsetof(PyInterpreterState, asyncio_tasks_head),
       },
       .asyncio_thread_state = {
           .size = sizeof(_PyThreadStateImpl),
           .asyncio_running_loop = offsetof(_PyThreadStateImpl, asyncio_running_loop),
           .asyncio_running_task = offsetof(_PyThreadStateImpl, asyncio_running_task),
           .asyncio_tasks_head = offsetof(_PyThreadStateImpl, asyncio_tasks_head),
       }};

/* State of the _asyncio module */
typedef struct {
    PyTypeObject *FutureIterType;
    PyTypeObject *TaskStepMethWrapper_Type;
    PyTypeObject *FutureType;
    PyTypeObject *TaskType;

    PyObject *asyncio_mod;
    PyObject *context_kwname;

    /* WeakSet containing scheduled 3rd party tasks which don't
       inherit from native asyncio.Task */
    PyObject *non_asyncio_tasks;

    /* Set containing all 3rd party eagerly executing tasks which don't
       inherit from native asyncio.Task */
    PyObject *non_asyncio_eager_tasks;

    /* An isinstance type cache for the 'is_coroutine()' function. */
    PyObject *iscoroutine_typecache;

    /* Imports from asyncio.events. */
    PyObject *asyncio_get_event_loop_policy;

    /* Imports from asyncio.base_futures. */
    PyObject *asyncio_future_repr_func;

    /* Imports from asyncio.exceptions. */
    PyObject *asyncio_CancelledError;
    PyObject *asyncio_InvalidStateError;

    /* Imports from asyncio.base_tasks. */
    PyObject *asyncio_task_get_stack_func;
    PyObject *asyncio_task_print_stack_func;
    PyObject *asyncio_task_repr_func;

    /* Imports from asyncio.coroutines. */
    PyObject *asyncio_iscoroutine_func;

    /* Imports from traceback. */
    PyObject *traceback_extract_stack;

    /* Counter for autogenerated Task names */
    uint64_t task_name_counter;

    /* Pointer to the asyncio debug offset to avoid it to be optimized away
       by the compiler */
    void *debug_offsets;

} asyncio_state;

static inline asyncio_state *
get_asyncio_state(PyObject *mod)
{
    asyncio_state *state = _PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static inline asyncio_state *
get_asyncio_state_by_cls(PyTypeObject *cls)
{
    asyncio_state *state = (asyncio_state *)_PyType_GetModuleState(cls);
    assert(state != NULL);
    return state;
}

static struct PyModuleDef _asynciomodule;

static inline asyncio_state *
get_asyncio_state_by_def(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *mod = PyType_GetModuleByDef(tp, &_asynciomodule);
    assert(mod != NULL);
    return get_asyncio_state(mod);
}

#include "clinic/_asynciomodule.c.h"


/*[clinic input]
class _asyncio.Future "FutureObj *" "&Future_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=00d3e4abca711e0f]*/


/* Get FutureIter from Future */
static PyObject * future_new_iter(PyObject *);

static PyObject *
task_step_handle_result_impl(asyncio_state *state, TaskObj *task, PyObject *result);
static void unregister_task(TaskObj *task);

static void
clear_task_coro(TaskObj *task)
{
    Py_CLEAR(task->task_coro);
}


static void
set_task_coro(TaskObj *task, PyObject *coro)
{
    assert(coro != NULL);
    Py_INCREF(coro);
    Py_XSETREF(task->task_coro, coro);
}


static int
_is_coroutine(asyncio_state *state, PyObject *coro)
{
    /* 'coro' is not a native coroutine, call asyncio.iscoroutine()
       to check if it's another coroutine flavour.

       Do this check after 'future_init()'; in case we need to raise
       an error, __del__ needs a properly initialized object.
    */
    PyObject *res = PyObject_CallOneArg(state->asyncio_iscoroutine_func, coro);
    if (res == NULL) {
        return -1;
    }

    int is_res_true = PyObject_IsTrue(res);
    Py_DECREF(res);
    if (is_res_true <= 0) {
        return is_res_true;
    }

    if (PySet_GET_SIZE(state->iscoroutine_typecache) < 100) {
        /* Just in case we don't want to cache more than 100
           positive types.  That shouldn't ever happen, unless
           someone stressing the system on purpose.
        */
        if (PySet_Add(state->iscoroutine_typecache, (PyObject*) Py_TYPE(coro))) {
            return -1;
        }
    }

    return 1;
}


static inline int
is_coroutine(asyncio_state *state, PyObject *coro)
{
    if (PyCoro_CheckExact(coro)) {
        return 1;
    }

    /* Check if `type(coro)` is in the cache.
       Caching makes is_coroutine() function almost as fast as
       PyCoro_CheckExact() for non-native coroutine-like objects
       (like coroutines compiled with Cython).

       asyncio.iscoroutine() has its own type caching mechanism.
       This cache allows us to avoid the cost of even calling
       a pure-Python function in 99.9% cases.
    */
    int has_it = PySet_Contains(
        state->iscoroutine_typecache, (PyObject*) Py_TYPE(coro));
    if (has_it == 0) {
        /* type(coro) is not in iscoroutine_typecache */
        return _is_coroutine(state, coro);
    }

    /* either an error has occurred or
       type(coro) is in iscoroutine_typecache
    */
    return has_it;
}


static PyObject *
get_future_loop(asyncio_state *state, PyObject *fut)
{
    /* Implementation of `asyncio.futures._get_loop` */

    PyObject *getloop;

    if (Future_CheckExact(state, fut) || Task_CheckExact(state, fut)) {
        PyObject *loop = ((FutureObj *)fut)->fut_loop;
        return Py_NewRef(loop);
    }

    if (PyObject_GetOptionalAttr(fut, &_Py_ID(get_loop), &getloop) < 0) {
        return NULL;
    }
    if (getloop != NULL) {
        PyObject *res = PyObject_CallNoArgs(getloop);
        Py_DECREF(getloop);
        return res;
    }

    return PyObject_GetAttr(fut, &_Py_ID(_loop));
}

static PyObject *
get_event_loop(asyncio_state *state)
{
    PyObject *loop;
    PyObject *policy;

    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();
    loop = Py_XNewRef(ts->asyncio_running_loop);

    if (loop != NULL) {
        return loop;
    }

    policy = PyObject_CallNoArgs(state->asyncio_get_event_loop_policy);
    if (policy == NULL) {
        return NULL;
    }

    loop = PyObject_CallMethodNoArgs(policy, &_Py_ID(get_event_loop));
    Py_DECREF(policy);
    return loop;
}


static int
call_soon(asyncio_state *state, PyObject *loop, PyObject *func, PyObject *arg,
          PyObject *ctx)
{
    PyObject *handle;

    if (ctx == NULL) {
        PyObject *stack[] = {loop, func, arg};
        size_t nargsf = 3 | PY_VECTORCALL_ARGUMENTS_OFFSET;
        handle = PyObject_VectorcallMethod(&_Py_ID(call_soon), stack, nargsf, NULL);
    }
    else {
        /* All refs in 'stack' are borrowed. */
        PyObject *stack[4];
        size_t nargs = 2;
        stack[0] = loop;
        stack[1] = func;
        if (arg != NULL) {
            stack[2] = arg;
            nargs++;
        }
        stack[nargs] = (PyObject *)ctx;
        size_t nargsf = nargs | PY_VECTORCALL_ARGUMENTS_OFFSET;
        handle = PyObject_VectorcallMethod(&_Py_ID(call_soon), stack, nargsf,
                                           state->context_kwname);
    }

    if (handle == NULL) {
        return -1;
    }
    Py_DECREF(handle);
    return 0;
}


static inline int
future_is_alive(FutureObj *fut)
{
    return fut->fut_loop != NULL;
}


static inline int
future_ensure_alive(FutureObj *fut)
{
    if (!future_is_alive(fut)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "Future object is not initialized.");
        return -1;
    }
    return 0;
}


#define ENSURE_FUTURE_ALIVE(state, fut)                             \
    do {                                                            \
        assert(Future_Check(state, fut) || Task_Check(state, fut)); \
        (void)state;                                                \
        if (future_ensure_alive((FutureObj*)fut)) {                 \
            return NULL;                                            \
        }                                                           \
    } while(0);


static int
future_schedule_callbacks(asyncio_state *state, FutureObj *fut)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);

    assert(fut->fut_state != STATE_PENDING);

    if (Task_Check(state, fut)) {
        // remove task from linked-list of tasks
        // as it is finished now
        TaskObj *task = (TaskObj *)fut;
        unregister_task(task);
    }

    if (fut->fut_callback0 != NULL) {
        /* There's a 1st callback */

        // Beware: An evil call_soon could alter fut_callback0 or fut_context0.
        // Since we are anyway clearing them after the call, whether call_soon
        // succeeds or not, the idea is to transfer ownership so that external
        // code is not able to alter them during the call.
        PyObject *fut_callback0 = fut->fut_callback0;
        fut->fut_callback0 = NULL;
        PyObject *fut_context0 = fut->fut_context0;
        fut->fut_context0 = NULL;

        int ret = call_soon(state, fut->fut_loop, fut_callback0,
                            (PyObject *)fut, fut_context0);
        Py_CLEAR(fut_callback0);
        Py_CLEAR(fut_context0);
        if (ret) {
            /* If an error occurs in pure-Python implementation,
               all callbacks are cleared. */
            Py_CLEAR(fut->fut_callbacks);
            return ret;
        }

        /* we called the first callback, now try calling
           callbacks from the 'fut_callbacks' list. */
    }

    if (fut->fut_callbacks == NULL) {
        /* No more callbacks, return. */
        return 0;
    }

    // Beware: An evil call_soon could change fut->fut_callbacks.
    // The idea is to transfer the ownership of the callbacks list
    // so that external code is not able to mutate the list during
    // the iteration.
    PyObject *callbacks = fut->fut_callbacks;
    fut->fut_callbacks = NULL;
    Py_ssize_t n = PyList_GET_SIZE(callbacks);
    for (Py_ssize_t i = 0; i < n; i++) {
        assert(PyList_GET_SIZE(callbacks) == n);
        PyObject *cb_tup = PyList_GET_ITEM(callbacks, i);
        PyObject *cb = PyTuple_GET_ITEM(cb_tup, 0);
        PyObject *ctx = PyTuple_GET_ITEM(cb_tup, 1);

        if (call_soon(state, fut->fut_loop, cb, (PyObject *)fut, ctx)) {
            Py_DECREF(callbacks);
            return -1;
        }
    }
    Py_DECREF(callbacks);
    return 0;
}


static int
future_init(FutureObj *fut, PyObject *loop)
{
    PyObject *res;
    int is_true;

    Py_CLEAR(fut->fut_loop);
    Py_CLEAR(fut->fut_callback0);
    Py_CLEAR(fut->fut_context0);
    Py_CLEAR(fut->fut_callbacks);
    Py_CLEAR(fut->fut_result);
    Py_CLEAR(fut->fut_exception);
    Py_CLEAR(fut->fut_exception_tb);
    Py_CLEAR(fut->fut_source_tb);
    Py_CLEAR(fut->fut_cancel_msg);
    Py_CLEAR(fut->fut_cancelled_exc);
    Py_CLEAR(fut->fut_awaited_by);

    fut->fut_state = STATE_PENDING;
    fut->fut_log_tb = 0;
    fut->fut_blocking = 0;
    fut->fut_awaited_by_is_set = 0;
    fut->fut_is_task = 0;

    if (loop == Py_None) {
        asyncio_state *state = get_asyncio_state_by_def((PyObject *)fut);
        loop = get_event_loop(state);
        if (loop == NULL) {
            return -1;
        }
    }
    else {
        Py_INCREF(loop);
    }
    fut->fut_loop = loop;

    res = PyObject_CallMethodNoArgs(fut->fut_loop, &_Py_ID(get_debug));
    if (res == NULL) {
        return -1;
    }
    is_true = PyObject_IsTrue(res);
    Py_DECREF(res);
    if (is_true < 0) {
        return -1;
    }
    if (is_true && !_Py_IsInterpreterFinalizing(_PyInterpreterState_GET())) {
        /* Only try to capture the traceback if the interpreter is not being
           finalized.  The original motivation to add a `Py_IsFinalizing()`
           call was to prevent SIGSEGV when a Future is created in a __del__
           method, which is called during the interpreter shutdown and the
           traceback module is already unloaded.
        */
        asyncio_state *state = get_asyncio_state_by_def((PyObject *)fut);
        fut->fut_source_tb = PyObject_CallNoArgs(state->traceback_extract_stack);
        if (fut->fut_source_tb == NULL) {
            return -1;
        }
    }

    return 0;
}

static int
future_awaited_by_add(asyncio_state *state, FutureObj *fut, PyObject *thing)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);
    // We only want to support native asyncio Futures.
    // For further insight see the comment in the Python
    // implementation of "future_add_to_awaited_by()".
    assert(TaskOrFuture_Check(state, fut));
    assert(TaskOrFuture_Check(state, thing));

    /* Most futures/task are only awaited by one entity, so we want
       to avoid always creating a set for `fut_awaited_by`.
    */
    if (fut->fut_awaited_by == NULL) {
        assert(!fut->fut_awaited_by_is_set);
        Py_INCREF(thing);
        fut->fut_awaited_by = thing;
        return 0;
    }

    if (fut->fut_awaited_by_is_set) {
        assert(PySet_CheckExact(fut->fut_awaited_by));
        return PySet_Add(fut->fut_awaited_by, thing);
    }

    PyObject *set = PySet_New(NULL);
    if (set == NULL) {
        return -1;
    }
    if (PySet_Add(set, thing)) {
        Py_DECREF(set);
        return -1;
    }
    if (PySet_Add(set, fut->fut_awaited_by)) {
        Py_DECREF(set);
        return -1;
    }
    Py_SETREF(fut->fut_awaited_by, set);
    fut->fut_awaited_by_is_set = 1;
    return 0;
}

static int
future_awaited_by_discard(asyncio_state *state, FutureObj *fut, PyObject *thing)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);
    // We only want to support native asyncio Futures.
    // For further insight see the comment in the Python
    // implementation of "future_add_to_awaited_by()".
    assert(TaskOrFuture_Check(state, fut));
    assert(TaskOrFuture_Check(state, thing));

    /* Following the semantics of 'set.discard()' here in not
       raising an error if `thing` isn't in the `awaited_by` "set".
    */
    if (fut->fut_awaited_by == NULL) {
        return 0;
    }
    if (fut->fut_awaited_by == thing) {
        Py_CLEAR(fut->fut_awaited_by);
        return 0;
    }
    if (fut->fut_awaited_by_is_set) {
        assert(PySet_CheckExact(fut->fut_awaited_by));
        int err = PySet_Discard(fut->fut_awaited_by, thing);
        if (err < 0) {
            return -1;
        } else {
            return 0;
        }
    }
    return 0;
}


static PyObject *
future_set_result(asyncio_state *state, FutureObj *fut, PyObject *res)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);

    if (future_ensure_alive(fut)) {
        return NULL;
    }

    if (fut->fut_state != STATE_PENDING) {
        PyErr_SetString(state->asyncio_InvalidStateError, "invalid state");
        return NULL;
    }

    assert(!fut->fut_result);
    fut->fut_result = Py_NewRef(res);
    fut->fut_state = STATE_FINISHED;

    if (future_schedule_callbacks(state, fut) == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
future_set_exception(asyncio_state *state, FutureObj *fut, PyObject *exc)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);

    PyObject *exc_val = NULL;

    if (fut->fut_state != STATE_PENDING) {
        PyErr_SetString(state->asyncio_InvalidStateError, "invalid state");
        return NULL;
    }

    if (PyExceptionClass_Check(exc)) {
        exc_val = PyObject_CallNoArgs(exc);
        if (exc_val == NULL) {
            return NULL;
        }
        if (fut->fut_state != STATE_PENDING) {
            Py_DECREF(exc_val);
            PyErr_SetString(state->asyncio_InvalidStateError, "invalid state");
            return NULL;
        }
    }
    else {
        exc_val = Py_NewRef(exc);
    }
    if (!PyExceptionInstance_Check(exc_val)) {
        Py_DECREF(exc_val);
        PyErr_SetString(PyExc_TypeError, "invalid exception object");
        return NULL;
    }
    if (PyErr_GivenExceptionMatches(exc_val, PyExc_StopIteration)) {
        const char *msg = "StopIteration interacts badly with "
                          "generators and cannot be raised into a "
                          "Future";
        PyObject *message = PyUnicode_FromString(msg);
        if (message == NULL) {
            Py_DECREF(exc_val);
            return NULL;
        }
        PyObject *err = PyObject_CallOneArg(PyExc_RuntimeError, message);
        Py_DECREF(message);
        if (err == NULL) {
            Py_DECREF(exc_val);
            return NULL;
        }
        assert(PyExceptionInstance_Check(err));

        PyException_SetCause(err, Py_NewRef(exc_val));
        PyException_SetContext(err, Py_NewRef(exc_val));
        Py_DECREF(exc_val);
        exc_val = err;
    }

    assert(!fut->fut_exception);
    assert(!fut->fut_exception_tb);
    fut->fut_exception = exc_val;
    fut->fut_exception_tb = PyException_GetTraceback(exc_val);
    fut->fut_state = STATE_FINISHED;

    if (future_schedule_callbacks(state, fut) == -1) {
        return NULL;
    }

    fut->fut_log_tb = 1;
    Py_RETURN_NONE;
}

static PyObject *
create_cancelled_error(asyncio_state *state, FutureObj *fut)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);

    PyObject *exc;
    if (fut->fut_cancelled_exc != NULL) {
        /* transfer ownership */
        exc = fut->fut_cancelled_exc;
        fut->fut_cancelled_exc = NULL;
        return exc;
    }
    PyObject *msg = fut->fut_cancel_msg;
    if (msg == NULL || msg == Py_None) {
        exc = PyObject_CallNoArgs(state->asyncio_CancelledError);
    } else {
        exc = PyObject_CallOneArg(state->asyncio_CancelledError, msg);
    }
    return exc;
}

static void
future_set_cancelled_error(asyncio_state *state, FutureObj *fut)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);

    PyObject *exc = create_cancelled_error(state, fut);
    if (exc == NULL) {
        return;
    }
    PyErr_SetObject(state->asyncio_CancelledError, exc);
    Py_DECREF(exc);
}

static int
future_get_result(asyncio_state *state, FutureObj *fut, PyObject **result)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);

    if (fut->fut_state == STATE_CANCELLED) {
        future_set_cancelled_error(state, fut);
        return -1;
    }

    if (fut->fut_state != STATE_FINISHED) {
        PyErr_SetString(state->asyncio_InvalidStateError,
                        "Result is not set.");
        return -1;
    }

    fut->fut_log_tb = 0;
    if (fut->fut_exception != NULL) {
        PyObject *tb = fut->fut_exception_tb;
        if (tb == NULL) {
            tb = Py_None;
        }
        if (PyException_SetTraceback(fut->fut_exception, tb) < 0) {
            return -1;
        }
        *result = Py_NewRef(fut->fut_exception);
        Py_CLEAR(fut->fut_exception_tb);
        return 1;
    }

    *result = Py_NewRef(fut->fut_result);
    return 0;
}

static PyObject *
future_add_done_callback(asyncio_state *state, FutureObj *fut, PyObject *arg,
                         PyObject *ctx)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);

    if (!future_is_alive(fut)) {
        PyErr_SetString(PyExc_RuntimeError, "uninitialized Future object");
        return NULL;
    }

    if (fut->fut_state != STATE_PENDING) {
        /* The future is done/cancelled, so schedule the callback
           right away. */
        if (call_soon(state, fut->fut_loop, arg, (PyObject*) fut, ctx)) {
            return NULL;
        }
    }
    else {
        /* The future is pending, add a callback.

           Callbacks in the future object are stored as follows:

              callback0 -- a pointer to the first callback
              callbacks -- a list of 2nd, 3rd, ... callbacks

           Invariants:

            * callbacks != NULL:
                There are some callbacks in the list.  Just
                add the new callback to it.

            * callbacks == NULL and callback0 == NULL:
                This is the first callback.  Set it to callback0.

            * callbacks == NULL and callback0 != NULL:
                This is a second callback.  Initialize callbacks
                with a new list and add the new callback to it.
        */

        if (fut->fut_callbacks == NULL && fut->fut_callback0 == NULL) {
            fut->fut_callback0 = Py_NewRef(arg);
            fut->fut_context0 = Py_NewRef(ctx);
        }
        else {
            PyObject *tup = PyTuple_New(2);
            if (tup == NULL) {
                return NULL;
            }
            Py_INCREF(arg);
            PyTuple_SET_ITEM(tup, 0, arg);
            Py_INCREF(ctx);
            PyTuple_SET_ITEM(tup, 1, (PyObject *)ctx);

            if (fut->fut_callbacks != NULL) {
                int err = PyList_Append(fut->fut_callbacks, tup);
                if (err) {
                    Py_DECREF(tup);
                    return NULL;
                }
                Py_DECREF(tup);
            }
            else {
                fut->fut_callbacks = PyList_New(1);
                if (fut->fut_callbacks == NULL) {
                    Py_DECREF(tup);
                    return NULL;
                }

                PyList_SET_ITEM(fut->fut_callbacks, 0, tup);  /* borrow */
            }
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
future_cancel(asyncio_state *state, FutureObj *fut, PyObject *msg)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);

    fut->fut_log_tb = 0;

    if (fut->fut_state != STATE_PENDING) {
        Py_RETURN_FALSE;
    }
    fut->fut_state = STATE_CANCELLED;

    Py_XINCREF(msg);
    Py_XSETREF(fut->fut_cancel_msg, msg);

    if (future_schedule_callbacks(state, fut) == -1) {
        return NULL;
    }

    Py_RETURN_TRUE;
}

/*[clinic input]
_asyncio.Future.__init__

    *
    loop: object = None

This class is *almost* compatible with concurrent.futures.Future.

    Differences:

    - result() and exception() do not take a timeout argument and
      raise an exception when the future isn't done yet.

    - Callbacks registered with add_done_callback() are always called
      via the event loop's call_soon_threadsafe().

    - This class is not compatible with the wait() and as_completed()
      methods in the concurrent.futures package.
[clinic start generated code]*/

static int
_asyncio_Future___init___impl(FutureObj *self, PyObject *loop)
/*[clinic end generated code: output=9ed75799eaccb5d6 input=89af317082bc0bf8]*/

{
    return future_init(self, loop);
}

static int
FutureObj_clear(PyObject *op)
{
    FutureObj *fut = (FutureObj*)op;
    Py_CLEAR(fut->fut_loop);
    Py_CLEAR(fut->fut_callback0);
    Py_CLEAR(fut->fut_context0);
    Py_CLEAR(fut->fut_callbacks);
    Py_CLEAR(fut->fut_result);
    Py_CLEAR(fut->fut_exception);
    Py_CLEAR(fut->fut_exception_tb);
    Py_CLEAR(fut->fut_source_tb);
    Py_CLEAR(fut->fut_cancel_msg);
    Py_CLEAR(fut->fut_cancelled_exc);
    Py_CLEAR(fut->fut_awaited_by);
    fut->fut_awaited_by_is_set = 0;
    PyObject_ClearManagedDict((PyObject *)fut);
    return 0;
}

static int
FutureObj_traverse(PyObject *op, visitproc visit, void *arg)
{
    FutureObj *fut = (FutureObj*)op;
    Py_VISIT(Py_TYPE(fut));
    Py_VISIT(fut->fut_loop);
    Py_VISIT(fut->fut_callback0);
    Py_VISIT(fut->fut_context0);
    Py_VISIT(fut->fut_callbacks);
    Py_VISIT(fut->fut_result);
    Py_VISIT(fut->fut_exception);
    Py_VISIT(fut->fut_exception_tb);
    Py_VISIT(fut->fut_source_tb);
    Py_VISIT(fut->fut_cancel_msg);
    Py_VISIT(fut->fut_cancelled_exc);
    Py_VISIT(fut->fut_awaited_by);
    PyObject_VisitManagedDict((PyObject *)fut, visit, arg);
    return 0;
}

/*[clinic input]
@critical_section
_asyncio.Future.result

Return the result this future represents.

If the future has been cancelled, raises CancelledError.  If the
future's result isn't yet available, raises InvalidStateError.  If
the future is done and has an exception set, this exception is raised.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_result_impl(FutureObj *self)
/*[clinic end generated code: output=f35f940936a4b1e5 input=61d89f48e4c8b670]*/
{
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)self);
    PyObject *result;

    if (!future_is_alive(self)) {
        PyErr_SetString(state->asyncio_InvalidStateError,
                        "Future object is not initialized.");
        return NULL;
    }

    int res = future_get_result(state, self, &result);

    if (res == -1) {
        return NULL;
    }

    if (res == 0) {
        return result;
    }

    assert(res == 1);

    PyErr_SetObject(PyExceptionInstance_Class(result), result);
    Py_DECREF(result);
    return NULL;
}

/*[clinic input]
@critical_section
_asyncio.Future.exception

    cls: defining_class
    /

Return the exception that was set on this future.

The exception (or None if no exception was set) is returned only if
the future is done.  If the future has been cancelled, raises
CancelledError.  If the future isn't done yet, raises
InvalidStateError.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_exception_impl(FutureObj *self, PyTypeObject *cls)
/*[clinic end generated code: output=ce75576b187c905b input=647d1fd1fc403301]*/
{
    if (!future_is_alive(self)) {
        asyncio_state *state = get_asyncio_state_by_cls(cls);
        PyErr_SetString(state->asyncio_InvalidStateError,
                        "Future object is not initialized.");
        return NULL;
    }

    if (self->fut_state == STATE_CANCELLED) {
        asyncio_state *state = get_asyncio_state_by_cls(cls);
        future_set_cancelled_error(state, self);
        return NULL;
    }

    if (self->fut_state != STATE_FINISHED) {
        asyncio_state *state = get_asyncio_state_by_cls(cls);
        PyErr_SetString(state->asyncio_InvalidStateError,
                        "Exception is not set.");
        return NULL;
    }

    if (self->fut_exception != NULL) {
        self->fut_log_tb = 0;
        return Py_NewRef(self->fut_exception);
    }

    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
_asyncio.Future.set_result

    cls: defining_class
    result: object
    /

Mark the future done and set its result.

If the future is already done when this method is called, raises
InvalidStateError.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_set_result_impl(FutureObj *self, PyTypeObject *cls,
                                PyObject *result)
/*[clinic end generated code: output=99afbbe78f99c32d input=4069306f03a3b6ee]*/
{
    asyncio_state *state = get_asyncio_state_by_cls(cls);
    ENSURE_FUTURE_ALIVE(state, self)
    return future_set_result(state, self, result);
}

/*[clinic input]
@critical_section
_asyncio.Future.set_exception

    cls: defining_class
    exception: object
    /

Mark the future done and set an exception.

If the future is already done when this method is called, raises
InvalidStateError.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_set_exception_impl(FutureObj *self, PyTypeObject *cls,
                                   PyObject *exception)
/*[clinic end generated code: output=0a5e8b5a52f058d6 input=b6eab43a389bc966]*/
{
    asyncio_state *state = get_asyncio_state_by_cls(cls);
    ENSURE_FUTURE_ALIVE(state, self)
    return future_set_exception(state, self, exception);
}

/*[clinic input]
@critical_section
_asyncio.Future.add_done_callback

    cls: defining_class
    fn: object
    /
    *
    context: object = NULL

Add a callback to be run when the future becomes done.

The callback is called with a single argument - the future object. If
the future is already done when this is called, the callback is
scheduled with call_soon.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_add_done_callback_impl(FutureObj *self, PyTypeObject *cls,
                                       PyObject *fn, PyObject *context)
/*[clinic end generated code: output=922e9a4cbd601167 input=37d97f941beb7b3e]*/
{
    asyncio_state *state = get_asyncio_state_by_cls(cls);
    if (context == NULL) {
        context = PyContext_CopyCurrent();
        if (context == NULL) {
            return NULL;
        }
        PyObject *res = future_add_done_callback(state, self, fn, context);
        Py_DECREF(context);
        return res;
    }
    return future_add_done_callback(state, self, fn, context);
}

/*[clinic input]
@critical_section
_asyncio.Future.remove_done_callback

    cls: defining_class
    fn: object
    /

Remove all instances of a callback from the "call when done" list.

Returns the number of callbacks removed.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_remove_done_callback_impl(FutureObj *self, PyTypeObject *cls,
                                          PyObject *fn)
/*[clinic end generated code: output=2da35ccabfe41b98 input=3afbc9f6a673091b]*/
{
    PyObject *newlist;
    Py_ssize_t len, i, j=0;
    Py_ssize_t cleared_callback0 = 0;

    asyncio_state *state = get_asyncio_state_by_cls(cls);
    ENSURE_FUTURE_ALIVE(state, self)

    if (self->fut_callback0 != NULL) {
        // Beware: An evil PyObject_RichCompareBool could free fut_callback0
        // before a recursive call is made with that same arg. For details, see
        // https://github.com/python/cpython/pull/125967#discussion_r1816593340.
        PyObject *fut_callback0 = Py_NewRef(self->fut_callback0);
        int cmp = PyObject_RichCompareBool(fut_callback0, fn, Py_EQ);
        Py_DECREF(fut_callback0);
        if (cmp == -1) {
            return NULL;
        }
        if (cmp == 1) {
            /* callback0 == fn */
            Py_CLEAR(self->fut_callback0);
            Py_CLEAR(self->fut_context0);
            cleared_callback0 = 1;
        }
    }

    if (self->fut_callbacks == NULL) {
        return PyLong_FromSsize_t(cleared_callback0);
    }

    len = PyList_GET_SIZE(self->fut_callbacks);
    if (len == 0) {
        Py_CLEAR(self->fut_callbacks);
        return PyLong_FromSsize_t(cleared_callback0);
    }

    if (len == 1) {
        PyObject *cb_tup = PyList_GET_ITEM(self->fut_callbacks, 0);
        Py_INCREF(cb_tup);
        int cmp = PyObject_RichCompareBool(
            PyTuple_GET_ITEM(cb_tup, 0), fn, Py_EQ);
        Py_DECREF(cb_tup);
        if (cmp == -1) {
            return NULL;
        }
        if (cmp == 1) {
            /* callbacks[0] == fn */
            Py_CLEAR(self->fut_callbacks);
            return PyLong_FromSsize_t(1 + cleared_callback0);
        }
        /* callbacks[0] != fn and len(callbacks) == 1 */
        return PyLong_FromSsize_t(cleared_callback0);
    }

    newlist = PyList_New(len);
    if (newlist == NULL) {
        return NULL;
    }

    // Beware: PyObject_RichCompareBool below may change fut_callbacks.
    // See GH-97592.
    for (i = 0;
         self->fut_callbacks != NULL && i < PyList_GET_SIZE(self->fut_callbacks);
         i++) {
        int ret;
        PyObject *item = PyList_GET_ITEM(self->fut_callbacks, i);
        Py_INCREF(item);
        ret = PyObject_RichCompareBool(PyTuple_GET_ITEM(item, 0), fn, Py_EQ);
        if (ret == 0) {
            if (j < len) {
                PyList_SET_ITEM(newlist, j, item);
                j++;
                continue;
            }
            ret = PyList_Append(newlist, item);
        }
        Py_DECREF(item);
        if (ret < 0) {
            goto fail;
        }
    }

    // Note: fut_callbacks may have been cleared.
    if (j == 0 || self->fut_callbacks == NULL) {
        Py_CLEAR(self->fut_callbacks);
        Py_DECREF(newlist);
        return PyLong_FromSsize_t(len + cleared_callback0);
    }

    if (j < len) {
        Py_SET_SIZE(newlist, j);
    }
    j = PyList_GET_SIZE(newlist);
    len = PyList_GET_SIZE(self->fut_callbacks);
    if (j != len) {
        if (PyList_SetSlice(self->fut_callbacks, 0, len, newlist) < 0) {
            goto fail;
        }
    }
    Py_DECREF(newlist);
    return PyLong_FromSsize_t(len - j + cleared_callback0);

fail:
    Py_DECREF(newlist);
    return NULL;
}

/*[clinic input]
@critical_section
_asyncio.Future.cancel

    cls: defining_class
    /
    msg: object = None

Cancel the future and schedule callbacks.

If the future is already done or cancelled, return False.  Otherwise,
change the future's state to cancelled, schedule the callbacks and
return True.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_cancel_impl(FutureObj *self, PyTypeObject *cls,
                            PyObject *msg)
/*[clinic end generated code: output=074956f35904b034 input=44ab4003da839970]*/
{
    asyncio_state *state = get_asyncio_state_by_cls(cls);
    ENSURE_FUTURE_ALIVE(state, self)
    return future_cancel(state, self, msg);
}

/*[clinic input]
@critical_section
_asyncio.Future.cancelled

Return True if the future was cancelled.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_cancelled_impl(FutureObj *self)
/*[clinic end generated code: output=145197ced586357d input=9b8644819a675416]*/
{
    if (future_is_alive(self) && self->fut_state == STATE_CANCELLED) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

/*[clinic input]
@critical_section
_asyncio.Future.done

Return True if the future is done.

Done means either that a result / exception are available, or that the
future was cancelled.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_done_impl(FutureObj *self)
/*[clinic end generated code: output=244c5ac351145096 input=7204d3cc63bef7f3]*/
{
    if (!future_is_alive(self) || self->fut_state == STATE_PENDING) {
        Py_RETURN_FALSE;
    }
    else {
        Py_RETURN_TRUE;
    }
}

/*[clinic input]
@critical_section
_asyncio.Future.get_loop

    cls: defining_class
    /

Return the event loop the Future is bound to.
[clinic start generated code]*/

static PyObject *
_asyncio_Future_get_loop_impl(FutureObj *self, PyTypeObject *cls)
/*[clinic end generated code: output=f50ea6c374d9ee97 input=f3ce629bfd9f45c1]*/
{
    asyncio_state *state = get_asyncio_state_by_cls(cls);
    ENSURE_FUTURE_ALIVE(state, self)
    return Py_NewRef(self->fut_loop);
}

/*[clinic input]
@critical_section
@getter
_asyncio.Future._asyncio_awaited_by
[clinic start generated code]*/

static PyObject *
_asyncio_Future__asyncio_awaited_by_get_impl(FutureObj *self)
/*[clinic end generated code: output=932af76d385d2e2a input=64c1783df2d44d2b]*/
{
    /* Implementation of a Python getter. */
    if (self->fut_awaited_by == NULL) {
        Py_RETURN_NONE;
    }
    if (self->fut_awaited_by_is_set) {
        /* Already a set, just wrap it into a frozen set and return. */
        assert(PySet_CheckExact(self->fut_awaited_by));
        return PyFrozenSet_New(self->fut_awaited_by);
    }

    PyObject *set = PyFrozenSet_New(NULL);
    if (set == NULL) {
        return NULL;
    }
    if (PySet_Add(set, self->fut_awaited_by)) {
        Py_DECREF(set);
        return NULL;
    }
    return set;
}


/*[clinic input]
@critical_section
@getter
_asyncio.Future._asyncio_future_blocking
[clinic start generated code]*/

static PyObject *
_asyncio_Future__asyncio_future_blocking_get_impl(FutureObj *self)
/*[clinic end generated code: output=a558a2c51e38823b input=58da92efc03b617d]*/
{
    if (future_is_alive(self) && self->fut_blocking) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

/*[clinic input]
@critical_section
@setter
_asyncio.Future._asyncio_future_blocking
[clinic start generated code]*/

static int
_asyncio_Future__asyncio_future_blocking_set_impl(FutureObj *self,
                                                  PyObject *value)
/*[clinic end generated code: output=0686d1cb024a7453 input=3fd4a5f95df788b7]*/

{
    if (future_ensure_alive(self)) {
        return -1;
    }
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }

    int is_true = PyObject_IsTrue(value);
    if (is_true < 0) {
        return -1;
    }
    self->fut_blocking = is_true;
    return 0;
}

/*[clinic input]
@critical_section
@getter
_asyncio.Future._log_traceback
[clinic start generated code]*/

static PyObject *
_asyncio_Future__log_traceback_get_impl(FutureObj *self)
/*[clinic end generated code: output=2724433b238593c7 input=91e5144ea4117d8e]*/
{
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)self);
    ENSURE_FUTURE_ALIVE(state, self)
    if (self->fut_log_tb) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

/*[clinic input]
@critical_section
@setter
_asyncio.Future._log_traceback
[clinic start generated code]*/

static int
_asyncio_Future__log_traceback_set_impl(FutureObj *self, PyObject *value)
/*[clinic end generated code: output=9ce8e19504f42f54 input=30ac8217754b08c2]*/
{
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    int is_true = PyObject_IsTrue(value);
    if (is_true < 0) {
        return -1;
    }
    if (is_true) {
        PyErr_SetString(PyExc_ValueError,
                        "_log_traceback can only be set to False");
        return -1;
    }
    self->fut_log_tb = is_true;
    return 0;
}
/*[clinic input]
@critical_section
@getter
_asyncio.Future._loop
[clinic start generated code]*/

static PyObject *
_asyncio_Future__loop_get_impl(FutureObj *self)
/*[clinic end generated code: output=5ba31563eecfeedf input=0337130bc5781670]*/
{
    if (!future_is_alive(self)) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(self->fut_loop);
}

/*[clinic input]
@critical_section
@getter
_asyncio.Future._callbacks
[clinic start generated code]*/

static PyObject *
_asyncio_Future__callbacks_get_impl(FutureObj *self)
/*[clinic end generated code: output=b40d360505fcc583 input=7a466649530c01bb]*/
{
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)self);
    ENSURE_FUTURE_ALIVE(state, self)

    Py_ssize_t len = 0;
    if (self->fut_callback0 != NULL) {
        len++;
    }
    if (self->fut_callbacks != NULL) {
        len += PyList_GET_SIZE(self->fut_callbacks);
    }

    if (len == 0) {
        Py_RETURN_NONE;
    }

    PyObject *callbacks = PyList_New(len);
    if (callbacks == NULL) {
        return NULL;
    }

    Py_ssize_t i = 0;
    if (self->fut_callback0 != NULL) {
        PyObject *tup0 = PyTuple_New(2);
        if (tup0 == NULL) {
            Py_DECREF(callbacks);
            return NULL;
        }
        PyTuple_SET_ITEM(tup0, 0, Py_NewRef(self->fut_callback0));
        assert(self->fut_context0 != NULL);
        PyTuple_SET_ITEM(tup0, 1, Py_NewRef(self->fut_context0));
        PyList_SET_ITEM(callbacks, i, tup0);
        i++;
    }

    if (self->fut_callbacks != NULL) {
        for (Py_ssize_t j = 0; j < PyList_GET_SIZE(self->fut_callbacks); j++) {
            PyObject *cb = PyList_GET_ITEM(self->fut_callbacks, j);
            Py_INCREF(cb);
            PyList_SET_ITEM(callbacks, i, cb);
            i++;
        }
    }

    return callbacks;
}

/*[clinic input]
@critical_section
@getter
_asyncio.Future._result
[clinic start generated code]*/

static PyObject *
_asyncio_Future__result_get_impl(FutureObj *self)
/*[clinic end generated code: output=6877e8ce97333873 input=624f8e28e67f2636]*/

{
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)self);
    ENSURE_FUTURE_ALIVE(state, self)
    if (self->fut_result == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(self->fut_result);
}

/*[clinic input]
@critical_section
@getter
_asyncio.Future._exception
[clinic start generated code]*/

static PyObject *
_asyncio_Future__exception_get_impl(FutureObj *self)
/*[clinic end generated code: output=32f2c93b9e021a9b input=1828a1fcac929710]*/
{
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)self);
    ENSURE_FUTURE_ALIVE(state, self)
    if (self->fut_exception == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(self->fut_exception);
}

/*[clinic input]
@critical_section
@getter
_asyncio.Future._source_traceback
[clinic start generated code]*/

static PyObject *
_asyncio_Future__source_traceback_get_impl(FutureObj *self)
/*[clinic end generated code: output=d4f12b09af22f61b input=3c831fbde5da90d0]*/
{
    if (!future_is_alive(self) || self->fut_source_tb == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(self->fut_source_tb);
}

/*[clinic input]
@critical_section
@getter
_asyncio.Future._cancel_message
[clinic start generated code]*/

static PyObject *
_asyncio_Future__cancel_message_get_impl(FutureObj *self)
/*[clinic end generated code: output=52ef6444f92cedac input=54c12c67082e4eea]*/
{
    if (self->fut_cancel_msg == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(self->fut_cancel_msg);
}

/*[clinic input]
@critical_section
@setter
_asyncio.Future._cancel_message
[clinic start generated code]*/

static int
_asyncio_Future__cancel_message_set_impl(FutureObj *self, PyObject *value)
/*[clinic end generated code: output=0854b2f77bff2209 input=f461d17f2d891fad]*/
{
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    Py_INCREF(value);
    Py_XSETREF(self->fut_cancel_msg, value);
    return 0;
}

/*[clinic input]
@critical_section
@getter
_asyncio.Future._state
[clinic start generated code]*/

static PyObject *
_asyncio_Future__state_get_impl(FutureObj *self)
/*[clinic end generated code: output=622f560a3fa69c63 input=7c5ad023a93423ff]*/
{
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)self);
    PyObject *ret = NULL;

    ENSURE_FUTURE_ALIVE(state, self)

    switch (self->fut_state) {
    case STATE_PENDING:
        ret = &_Py_ID(PENDING);
        break;
    case STATE_CANCELLED:
        ret = &_Py_ID(CANCELLED);
        break;
    case STATE_FINISHED:
        ret = &_Py_ID(FINISHED);
        break;
    default:
        assert (0);
    }
    assert(_Py_IsImmortal(ret));
    return ret;
}

static PyObject *
FutureObj_repr(PyObject *op)
{
    FutureObj *fut = (FutureObj*)op;
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)fut);
    ENSURE_FUTURE_ALIVE(state, fut)
    return PyObject_CallOneArg(state->asyncio_future_repr_func, (PyObject *)fut);
}

/*[clinic input]
@critical_section
_asyncio.Future._make_cancelled_error

Create the CancelledError to raise if the Future is cancelled.

This should only be called once when handling a cancellation since
it erases the context exception value.
[clinic start generated code]*/

static PyObject *
_asyncio_Future__make_cancelled_error_impl(FutureObj *self)
/*[clinic end generated code: output=a5df276f6c1213de input=ccb90df8c3c18bcd]*/
{
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)self);
    return create_cancelled_error(state, self);
}

static void
FutureObj_finalize(PyObject *op)
{
    FutureObj *fut = (FutureObj*)op;
    PyObject *context;
    PyObject *message = NULL;
    PyObject *func;

    if (!fut->fut_log_tb) {
        return;
    }
    assert(fut->fut_exception != NULL);
    fut->fut_log_tb = 0;

    /* Save the current exception, if any. */
    PyObject *exc = PyErr_GetRaisedException();

    context = PyDict_New();
    if (context == NULL) {
        goto finally;
    }

    message = PyUnicode_FromFormat(
        "%s exception was never retrieved", _PyType_Name(Py_TYPE(fut)));
    if (message == NULL) {
        goto finally;
    }

    if (PyDict_SetItem(context, &_Py_ID(message), message) < 0 ||
        PyDict_SetItem(context, &_Py_ID(exception), fut->fut_exception) < 0 ||
        PyDict_SetItem(context, &_Py_ID(future), (PyObject*)fut) < 0) {
        goto finally;
    }
    if (fut->fut_source_tb != NULL) {
        if (PyDict_SetItem(context, &_Py_ID(source_traceback),
                              fut->fut_source_tb) < 0) {
            goto finally;
        }
    }

    func = PyObject_GetAttr(fut->fut_loop, &_Py_ID(call_exception_handler));
    if (func != NULL) {
        PyObject *res = PyObject_CallOneArg(func, context);
        if (res == NULL) {
            PyErr_FormatUnraisable("Exception ignored while calling asyncio "
                                   "function %R", func);
        }
        else {
            Py_DECREF(res);
        }
        Py_DECREF(func);
    }

finally:
    Py_XDECREF(context);
    Py_XDECREF(message);

    /* Restore the saved exception. */
    PyErr_SetRaisedException(exc);
}

static PyMethodDef FutureType_methods[] = {
    _ASYNCIO_FUTURE_RESULT_METHODDEF
    _ASYNCIO_FUTURE_EXCEPTION_METHODDEF
    _ASYNCIO_FUTURE_SET_RESULT_METHODDEF
    _ASYNCIO_FUTURE_SET_EXCEPTION_METHODDEF
    _ASYNCIO_FUTURE_ADD_DONE_CALLBACK_METHODDEF
    _ASYNCIO_FUTURE_REMOVE_DONE_CALLBACK_METHODDEF
    _ASYNCIO_FUTURE_CANCEL_METHODDEF
    _ASYNCIO_FUTURE_CANCELLED_METHODDEF
    _ASYNCIO_FUTURE_DONE_METHODDEF
    _ASYNCIO_FUTURE_GET_LOOP_METHODDEF
    _ASYNCIO_FUTURE__MAKE_CANCELLED_ERROR_METHODDEF
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {NULL, NULL}        /* Sentinel */
};

static PyGetSetDef FutureType_getsetlist[] = {
    _ASYNCIO_FUTURE__STATE_GETSETDEF
    _ASYNCIO_FUTURE__ASYNCIO_FUTURE_BLOCKING_GETSETDEF
    _ASYNCIO_FUTURE__LOOP_GETSETDEF
    _ASYNCIO_FUTURE__CALLBACKS_GETSETDEF
    _ASYNCIO_FUTURE__RESULT_GETSETDEF
    _ASYNCIO_FUTURE__EXCEPTION_GETSETDEF
    _ASYNCIO_FUTURE__LOG_TRACEBACK_GETSETDEF
    _ASYNCIO_FUTURE__SOURCE_TRACEBACK_GETSETDEF
    _ASYNCIO_FUTURE__CANCEL_MESSAGE_GETSETDEF
    _ASYNCIO_FUTURE__ASYNCIO_AWAITED_BY_GETSETDEF
    {NULL} /* Sentinel */
};

static void FutureObj_dealloc(PyObject *self);

static PyType_Slot Future_slots[] = {
    {Py_tp_dealloc, FutureObj_dealloc},
    {Py_tp_repr, FutureObj_repr},
    {Py_tp_doc, (void *)_asyncio_Future___init____doc__},
    {Py_tp_traverse, FutureObj_traverse},
    {Py_tp_clear, FutureObj_clear},
    {Py_tp_iter, future_new_iter},
    {Py_tp_methods, FutureType_methods},
    {Py_tp_getset, FutureType_getsetlist},
    {Py_tp_init, _asyncio_Future___init__},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_finalize, FutureObj_finalize},

    // async slots
    {Py_am_await, future_new_iter},
    {0, NULL},
};

static PyType_Spec Future_spec = {
    .name = "_asyncio.Future",
    .basicsize = sizeof(FutureObj),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_MANAGED_DICT |
              Py_TPFLAGS_MANAGED_WEAKREF),
    .slots = Future_slots,
};

static void
FutureObj_dealloc(PyObject *self)
{
    if (PyObject_CallFinalizerFromDealloc(self) < 0) {
        // resurrected.
        return;
    }

    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);

    PyObject_ClearWeakRefs(self);

    (void)FutureObj_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}


/*********************** Future Iterator **************************/

typedef struct futureiterobject {
    PyObject_HEAD
    FutureObj *future;
} futureiterobject;


static void
FutureIter_dealloc(PyObject *it)
{
    PyTypeObject *tp = Py_TYPE(it);

    assert(_PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE));

    PyObject_GC_UnTrack(it);
    tp->tp_clear(it);

    if (!_Py_FREELIST_PUSH(futureiters, it, Py_futureiters_MAXFREELIST)) {
        PyObject_GC_Del(it);
        Py_DECREF(tp);
    }
}

static PySendResult
FutureIter_am_send_lock_held(futureiterobject *it, PyObject **result)
{
    PyObject *res;
    FutureObj *fut = it->future;
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(fut);

    *result = NULL;

    if (fut->fut_state == STATE_PENDING) {
        if (!fut->fut_blocking) {
            fut->fut_blocking = 1;
            *result = Py_NewRef(fut);
            return PYGEN_NEXT;
        }
        PyErr_SetString(PyExc_RuntimeError,
                        "await wasn't used with future");
        return PYGEN_ERROR;
    }

    res = _asyncio_Future_result_impl(fut);
    if (res != NULL) {
        *result = res;
        return PYGEN_RETURN;
    }

    return PYGEN_ERROR;
}

static PySendResult
FutureIter_am_send(PyObject *op,
                   PyObject *Py_UNUSED(arg),
                   PyObject **result)
{
    futureiterobject *it = (futureiterobject*)op;
    /* arg is unused, see the comment on FutureIter_send for clarification */
    PySendResult res;
    Py_BEGIN_CRITICAL_SECTION(it->future);
    res = FutureIter_am_send_lock_held(it, result);
    Py_END_CRITICAL_SECTION();
    return res;
}


static PyObject *
FutureIter_iternext(PyObject *it)
{
    PyObject *result;
    switch (FutureIter_am_send(it, Py_None, &result)) {
        case PYGEN_RETURN:
            (void)_PyGen_SetStopIterationValue(result);
            Py_DECREF(result);
            return NULL;
        case PYGEN_NEXT:
            return result;
        case PYGEN_ERROR:
            return NULL;
        default:
            Py_UNREACHABLE();
    }
}

static PyObject *
FutureIter_send(PyObject *self, PyObject *unused)
{
    /* Future.__iter__ doesn't care about values that are pushed to the
     * generator, it just returns self.result().
     */
    return FutureIter_iternext(self);
}

static PyObject *
FutureIter_throw(PyObject *op, PyObject *const *args, Py_ssize_t nargs)
{
    futureiterobject *self = (futureiterobject*)op;
    PyObject *type, *val = NULL, *tb = NULL;
    if (!_PyArg_CheckPositional("throw", nargs, 1, 3)) {
        return NULL;
    }
    if (nargs > 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                            "the (type, exc, tb) signature of throw() is deprecated, "
                            "use the single-arg signature instead.",
                            1) < 0) {
            return NULL;
        }
    }

    type = args[0];
    if (nargs == 3) {
        val = args[1];
        tb = args[2];
    }
    else if (nargs == 2) {
        val = args[1];
    }

    if (val == Py_None) {
        val = NULL;
    }
    if (tb == Py_None ) {
        tb = NULL;
    } else if (tb != NULL && !PyTraceBack_Check(tb)) {
        PyErr_SetString(PyExc_TypeError, "throw() third argument must be a traceback");
        return NULL;
    }

    Py_INCREF(type);
    Py_XINCREF(val);
    Py_XINCREF(tb);

    if (PyExceptionClass_Check(type)) {
        PyErr_NormalizeException(&type, &val, &tb);
        /* No need to call PyException_SetTraceback since we'll be calling
           PyErr_Restore for `type`, `val`, and `tb`. */
    } else if (PyExceptionInstance_Check(type)) {
        if (val) {
            PyErr_SetString(PyExc_TypeError,
                            "instance exception may not have a separate value");
            goto fail;
        }
        val = type;
        type = PyExceptionInstance_Class(type);
        Py_INCREF(type);
        if (tb == NULL)
            tb = PyException_GetTraceback(val);
    } else {
        PyErr_SetString(PyExc_TypeError,
                        "exceptions must be classes deriving BaseException or "
                        "instances of such a class");
        goto fail;
    }

    Py_CLEAR(self->future);

    PyErr_Restore(type, val, tb);

    return NULL;

  fail:
    Py_DECREF(type);
    Py_XDECREF(val);
    Py_XDECREF(tb);
    return NULL;
}

static int
FutureIter_clear(PyObject *op)
{
    futureiterobject *it = (futureiterobject*)op;
    Py_CLEAR(it->future);
    return 0;
}

static PyObject *
FutureIter_close(PyObject *self, PyObject *arg)
{
    (void)FutureIter_clear(self);
    Py_RETURN_NONE;
}

static int
FutureIter_traverse(PyObject *op, visitproc visit, void *arg)
{
    futureiterobject *it = (futureiterobject*)op;
    Py_VISIT(Py_TYPE(it));
    Py_VISIT(it->future);
    return 0;
}

static PyMethodDef FutureIter_methods[] = {
    {"send",  FutureIter_send, METH_O, NULL},
    {"throw", _PyCFunction_CAST(FutureIter_throw), METH_FASTCALL, NULL},
    {"close", FutureIter_close, METH_NOARGS, NULL},
    {NULL, NULL}        /* Sentinel */
};

static PyType_Slot FutureIter_slots[] = {
    {Py_tp_dealloc, FutureIter_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, FutureIter_traverse},
    {Py_tp_clear, FutureIter_clear},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, FutureIter_iternext},
    {Py_tp_methods, FutureIter_methods},

    // async methods
    {Py_am_send, FutureIter_am_send},
    {0, NULL},
};

static PyType_Spec FutureIter_spec = {
    .name = "_asyncio.FutureIter",
    .basicsize = sizeof(futureiterobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = FutureIter_slots,
};

static PyObject *
future_new_iter(PyObject *fut)
{
    futureiterobject *it;

    asyncio_state *state = get_asyncio_state_by_def((PyObject *)fut);
    ENSURE_FUTURE_ALIVE(state, fut)

    it = _Py_FREELIST_POP(futureiterobject, futureiters);
    if (it == NULL) {
        it = PyObject_GC_New(futureiterobject, state->FutureIterType);
        if (it == NULL) {
            return NULL;
        }
    }

    it->future = (FutureObj*)Py_NewRef(fut);
    PyObject_GC_Track(it);
    return (PyObject*)it;
}


/*********************** Task **************************/


/*[clinic input]
class _asyncio.Task "TaskObj *" "&Task_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=719dcef0fcc03b37]*/

static int task_call_step_soon(asyncio_state *state, TaskObj *, PyObject *);
static PyObject *task_wakeup(PyObject *op, PyObject *arg);
static PyObject * task_step(asyncio_state *, TaskObj *, PyObject *);
static int task_eager_start(asyncio_state *state, TaskObj *task);

/* ----- Task._step wrapper */

static int
TaskStepMethWrapper_clear(PyObject *op)
{
    TaskStepMethWrapper *o = (TaskStepMethWrapper*)op;
    Py_CLEAR(o->sw_task);
    Py_CLEAR(o->sw_arg);
    return 0;
}

static void
TaskStepMethWrapper_dealloc(PyObject *op)
{
    TaskStepMethWrapper *o = (TaskStepMethWrapper*)op;
    PyTypeObject *tp = Py_TYPE(o);
    PyObject_GC_UnTrack(o);
    (void)TaskStepMethWrapper_clear(op);
    Py_TYPE(o)->tp_free(o);
    Py_DECREF(tp);
}

static PyObject *
TaskStepMethWrapper_call(PyObject *op,
                         PyObject *args, PyObject *kwds)
{
    TaskStepMethWrapper *o = (TaskStepMethWrapper*)op;
    if (kwds != NULL && PyDict_GET_SIZE(kwds) != 0) {
        PyErr_SetString(PyExc_TypeError, "function takes no keyword arguments");
        return NULL;
    }
    if (args != NULL && PyTuple_GET_SIZE(args) != 0) {
        PyErr_SetString(PyExc_TypeError, "function takes no positional arguments");
        return NULL;
    }
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)o);
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(o->sw_task);
    res = task_step(state, o->sw_task, o->sw_arg);
    Py_END_CRITICAL_SECTION();
    return res;
}

static int
TaskStepMethWrapper_traverse(PyObject *op,
                             visitproc visit, void *arg)
{
    TaskStepMethWrapper *o = (TaskStepMethWrapper*)op;
    Py_VISIT(Py_TYPE(o));
    Py_VISIT(o->sw_task);
    Py_VISIT(o->sw_arg);
    return 0;
}

static PyObject *
TaskStepMethWrapper_get___self__(PyObject *op, void *Py_UNUSED(closure))
{
    TaskStepMethWrapper *o = (TaskStepMethWrapper*)op;
    if (o->sw_task) {
        return Py_NewRef(o->sw_task);
    }
    Py_RETURN_NONE;
}

static PyGetSetDef TaskStepMethWrapper_getsetlist[] = {
    {"__self__", TaskStepMethWrapper_get___self__, NULL, NULL},
    {NULL} /* Sentinel */
};

static PyType_Slot TaskStepMethWrapper_slots[] = {
    {Py_tp_getset, TaskStepMethWrapper_getsetlist},
    {Py_tp_dealloc, TaskStepMethWrapper_dealloc},
    {Py_tp_call, TaskStepMethWrapper_call},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, TaskStepMethWrapper_traverse},
    {Py_tp_clear, TaskStepMethWrapper_clear},
    {0, NULL},
};

static PyType_Spec TaskStepMethWrapper_spec = {
    .name = "_asyncio.TaskStepMethWrapper",
    .basicsize = sizeof(TaskStepMethWrapper),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = TaskStepMethWrapper_slots,
};

static PyObject *
TaskStepMethWrapper_new(TaskObj *task, PyObject *arg)
{
    asyncio_state *state = get_asyncio_state_by_def((PyObject *)task);
    TaskStepMethWrapper *o;
    o = PyObject_GC_New(TaskStepMethWrapper, state->TaskStepMethWrapper_Type);
    if (o == NULL) {
        return NULL;
    }

    o->sw_task = (TaskObj*)Py_NewRef(task);
    o->sw_arg = Py_XNewRef(arg);

    PyObject_GC_Track(o);
    return (PyObject*) o;
}

/* ----- Task._wakeup implementation */

static  PyMethodDef TaskWakeupDef = {
    "task_wakeup",
    task_wakeup,
    METH_O,
    NULL
};

/* ----- Task introspection helpers */

static void
register_task(TaskObj *task)
{
    if (task->task_node.next != NULL) {
        // already registered
        assert(task->task_node.prev != NULL);
        return;
    }
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *) _PyThreadState_GET();
    struct llist_node *head = &tstate->asyncio_tasks_head;
    llist_insert_tail(head, &task->task_node);
}

static inline void
unregister_task_safe(TaskObj *task)
{
    if (task->task_node.next == NULL) {
        // not registered
        assert(task->task_node.prev == NULL);
        return;
    }
    llist_remove(&task->task_node);
}

static void
unregister_task(TaskObj *task)
{
#ifdef Py_GIL_DISABLED
    // check if we are in the same thread
    // if so, we can avoid locking
    if (task->task_tid == _Py_ThreadId()) {
        unregister_task_safe(task);
    }
    else {
        // we are in a different thread
        // stop the world then check and remove the task
        PyThreadState *tstate = _PyThreadState_GET();
        _PyEval_StopTheWorld(tstate->interp);
        unregister_task_safe(task);
        _PyEval_StartTheWorld(tstate->interp);
    }
#else
    unregister_task_safe(task);
#endif
}

static int
enter_task(PyObject *loop, PyObject *task)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();

    if (ts->asyncio_running_loop != loop) {
        PyErr_Format(PyExc_RuntimeError, "loop %R is not the running loop", loop);
        return -1;
    }

    if (ts->asyncio_running_task != NULL) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Cannot enter into task %R while another " \
            "task %R is being executed.",
            task, ts->asyncio_running_task, NULL);
        return -1;
    }

    ts->asyncio_running_task = Py_NewRef(task);
    return 0;
}

static int
leave_task(PyObject *loop, PyObject *task)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();

    if (ts->asyncio_running_loop != loop) {
        PyErr_Format(PyExc_RuntimeError, "loop %R is not the running loop", loop);
        return -1;
    }

    if (ts->asyncio_running_task != task) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Invalid attempt to leave task %R while " \
            "task %R is entered.",
            task, ts->asyncio_running_task ? ts->asyncio_running_task : Py_None, NULL);
        return -1;
    }
    Py_CLEAR(ts->asyncio_running_task);
    return 0;
}

static PyObject *
swap_current_task(PyObject *loop, PyObject *task)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();

    if (ts->asyncio_running_loop != loop) {
        PyErr_Format(PyExc_RuntimeError, "loop %R is not the running loop", loop);
        return NULL;
    }

    /* transfer ownership to avoid redundant ref counting */
    PyObject *prev_task = ts->asyncio_running_task;
    if (task != Py_None) {
        ts->asyncio_running_task = Py_NewRef(task);
    } else {
        ts->asyncio_running_task = NULL;
    }
    if (prev_task == NULL) {
        Py_RETURN_NONE;
    }
    return prev_task;
}

/* ----- Task */

/*[clinic input]
_asyncio.Task.__init__

    coro: object
    *
    loop: object = None
    name: object = None
    context: object = None
    eager_start: bool = False

A coroutine wrapped in a Future.
[clinic start generated code]*/

static int
_asyncio_Task___init___impl(TaskObj *self, PyObject *coro, PyObject *loop,
                            PyObject *name, PyObject *context,
                            int eager_start)
/*[clinic end generated code: output=7aced2d27836f1a1 input=18e3f113a51b829d]*/
{
    if (future_init((FutureObj*)self, loop)) {
        return -1;
    }
    self->task_is_task = 1;

    asyncio_state *state = get_asyncio_state_by_def((PyObject *)self);
    int is_coro = is_coroutine(state, coro);
    if (is_coro == -1) {
        return -1;
    }
    if (is_coro == 0) {
        self->task_log_destroy_pending = 0;
        PyErr_Format(PyExc_TypeError,
                     "a coroutine was expected, got %R",
                     coro, NULL);
        return -1;
    }

    if (context == Py_None) {
        Py_XSETREF(self->task_context, PyContext_CopyCurrent());
        if (self->task_context == NULL) {
            return -1;
        }
    } else {
        Py_XSETREF(self->task_context, Py_NewRef(context));
    }

    Py_CLEAR(self->task_fut_waiter);
#ifdef Py_GIL_DISABLED
    self->task_tid = _Py_ThreadId();
#endif
    self->task_must_cancel = 0;
    self->task_log_destroy_pending = 1;
    self->task_num_cancels_requested = 0;
    set_task_coro(self, coro);

    if (name == Py_None) {
        // optimization: defer task name formatting
        // store the task counter as PyLong in the name
        // for deferred formatting in get_name
#ifdef Py_GIL_DISABLED
        unsigned long long counter = _Py_atomic_add_uint64(&state->task_name_counter, 1) + 1;
#else
        unsigned long long counter = ++state->task_name_counter;
#endif
        name = PyLong_FromUnsignedLongLong(counter);
    } else if (!PyUnicode_CheckExact(name)) {
        name = PyObject_Str(name);
    } else {
        Py_INCREF(name);
    }
    Py_XSETREF(self->task_name, name);
    if (self->task_name == NULL) {
        return -1;
    }

    if (eager_start) {
        PyObject *res = PyObject_CallMethodNoArgs(loop, &_Py_ID(is_running));
        if (res == NULL) {
            return -1;
        }
        int is_loop_running = Py_IsTrue(res);
        Py_DECREF(res);
        if (is_loop_running) {
            if (task_eager_start(state, self)) {
                return -1;
            }
            return 0;
        }
    }

    if (task_call_step_soon(state, self, NULL)) {
        return -1;
    }
#ifdef Py_GIL_DISABLED
    // This is required so that _Py_TryIncref(self)
    // works correctly in non-owning threads.
    _PyObject_SetMaybeWeakref((PyObject *)self);
#endif
    register_task(self);
    return 0;
}

static int
TaskObj_clear(PyObject *op)
{
    TaskObj *task = (TaskObj*)op;
    (void)FutureObj_clear(op);
    clear_task_coro(task);
    Py_CLEAR(task->task_context);
    Py_CLEAR(task->task_name);
    Py_CLEAR(task->task_fut_waiter);
    return 0;
}

static int
TaskObj_traverse(PyObject *op, visitproc visit, void *arg)
{
    TaskObj *task = (TaskObj*)op;
    Py_VISIT(Py_TYPE(task));
    Py_VISIT(task->task_context);
    Py_VISIT(task->task_coro);
    Py_VISIT(task->task_name);
    Py_VISIT(task->task_fut_waiter);
    FutureObj *fut = (FutureObj *)task;
    Py_VISIT(fut->fut_loop);
    Py_VISIT(fut->fut_callback0);
    Py_VISIT(fut->fut_context0);
    Py_VISIT(fut->fut_callbacks);
    Py_VISIT(fut->fut_result);
    Py_VISIT(fut->fut_exception);
    Py_VISIT(fut->fut_exception_tb);
    Py_VISIT(fut->fut_source_tb);
    Py_VISIT(fut->fut_cancel_msg);
    Py_VISIT(fut->fut_cancelled_exc);
    Py_VISIT(fut->fut_awaited_by);
    PyObject_VisitManagedDict((PyObject *)fut, visit, arg);
    return 0;
}

/*[clinic input]
@critical_section
@getter
_asyncio.Task._log_destroy_pending
[clinic start generated code]*/

static PyObject *
_asyncio_Task__log_destroy_pending_get_impl(TaskObj *self)
/*[clinic end generated code: output=e6c2a47d029ac93b input=17127298cd4c720b]*/
{
    if (self->task_log_destroy_pending) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

/*[clinic input]
@critical_section
@setter
_asyncio.Task._log_destroy_pending
[clinic start generated code]*/

static int
_asyncio_Task__log_destroy_pending_set_impl(TaskObj *self, PyObject *value)
/*[clinic end generated code: output=7ebc030bb92ec5ce input=49b759c97d1216a4]*/
{
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete attribute");
        return -1;
    }
    int is_true = PyObject_IsTrue(value);
    if (is_true < 0) {
        return -1;
    }
    self->task_log_destroy_pending = is_true;
    return 0;
}


/*[clinic input]
@critical_section
@getter
_asyncio.Task._must_cancel
[clinic start generated code]*/

static PyObject *
_asyncio_Task__must_cancel_get_impl(TaskObj *self)
/*[clinic end generated code: output=70e79b900996c363 input=2d04529fb23feedf]*/
{
    if (self->task_must_cancel) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

/*[clinic input]
@critical_section
@getter
_asyncio.Task._coro
[clinic start generated code]*/

static PyObject *
_asyncio_Task__coro_get_impl(TaskObj *self)
/*[clinic end generated code: output=a2726012ab5fd531 input=323c31a272020624]*/
{
    if (self->task_coro) {
        return Py_NewRef(self->task_coro);
    }

    Py_RETURN_NONE;
}


/*[clinic input]
@critical_section
@getter
_asyncio.Task._fut_waiter
[clinic start generated code]*/

static PyObject *
_asyncio_Task__fut_waiter_get_impl(TaskObj *self)
/*[clinic end generated code: output=c4f966b847fefcdf input=4d1005d725e72db7]*/
{
    if (self->task_fut_waiter) {
        return Py_NewRef(self->task_fut_waiter);
    }

    Py_RETURN_NONE;
}

static PyObject *
TaskObj_repr(PyObject *task)
{
    asyncio_state *state = get_asyncio_state_by_def(task);
    return PyObject_CallOneArg(state->asyncio_task_repr_func, task);
}


/*[clinic input]
@critical_section
_asyncio.Task._make_cancelled_error

Create the CancelledError to raise if the Task is cancelled.

This should only be called once when handling a cancellation since
it erases the context exception value.
[clinic start generated code]*/

static PyObject *
_asyncio_Task__make_cancelled_error_impl(TaskObj *self)
/*[clinic end generated code: output=55a819e8b4276fab input=2d3213be0cb02390]*/
{
    FutureObj *fut = (FutureObj*)self;
    return _asyncio_Future__make_cancelled_error_impl(fut);
}


/*[clinic input]
@critical_section
_asyncio.Task.cancel

    msg: object = None

Request that this task cancel itself.

This arranges for a CancelledError to be thrown into the
wrapped coroutine on the next cycle through the event loop.
The coroutine then has a chance to clean up or even deny
the request using try/except/finally.

Unlike Future.cancel, this does not guarantee that the
task will be cancelled: the exception might be caught and
acted upon, delaying cancellation of the task or preventing
cancellation completely.  The task may also return a value or
raise a different exception.

Immediately after this method is called, Task.cancelled() will
not return True (unless the task was already cancelled).  A
task will be marked as cancelled when the wrapped coroutine
terminates with a CancelledError exception (even if cancel()
was not called).

This also increases the task's count of cancellation requests.
[clinic start generated code]*/

static PyObject *
_asyncio_Task_cancel_impl(TaskObj *self, PyObject *msg)
/*[clinic end generated code: output=c66b60d41c74f9f1 input=6125d45b9a6a5abd]*/
{
    self->task_log_tb = 0;

    if (self->task_state != STATE_PENDING) {
        Py_RETURN_FALSE;
    }

    self->task_num_cancels_requested += 1;

    // These three lines are controversial.  See discussion starting at
    // https://github.com/python/cpython/pull/31394#issuecomment-1053545331
    // and corresponding code in tasks.py.
    // if (self->task_num_cancels_requested > 1) {
    //     Py_RETURN_FALSE;
    // }

    if (self->task_fut_waiter) {
        PyObject *res;
        int is_true;

        res = PyObject_CallMethodOneArg(self->task_fut_waiter,
                                           &_Py_ID(cancel), msg);
        if (res == NULL) {
            return NULL;
        }

        is_true = PyObject_IsTrue(res);
        Py_DECREF(res);
        if (is_true < 0) {
            return NULL;
        }

        if (is_true) {
            Py_RETURN_TRUE;
        }
    }

    self->task_must_cancel = 1;
    Py_XINCREF(msg);
    Py_XSETREF(self->task_cancel_msg, msg);
    Py_RETURN_TRUE;
}

/*[clinic input]
@critical_section
_asyncio.Task.cancelling

Return the count of the task's cancellation requests.

This count is incremented when .cancel() is called
and may be decremented using .uncancel().
[clinic start generated code]*/

static PyObject *
_asyncio_Task_cancelling_impl(TaskObj *self)
/*[clinic end generated code: output=803b3af96f917d7e input=5ef89b1b38f080ee]*/
/*[clinic end generated code]*/
{
    return PyLong_FromLong(self->task_num_cancels_requested);
}

/*[clinic input]
@critical_section
_asyncio.Task.uncancel

Decrement the task's count of cancellation requests.

This should be used by tasks that catch CancelledError
and wish to continue indefinitely until they are cancelled again.

Returns the remaining number of cancellation requests.
[clinic start generated code]*/

static PyObject *
_asyncio_Task_uncancel_impl(TaskObj *self)
/*[clinic end generated code: output=58184d236a817d3c input=cb3220b0e5afd61d]*/
/*[clinic end generated code]*/
{
    if (self->task_num_cancels_requested > 0) {
        self->task_num_cancels_requested -= 1;
        if (self->task_num_cancels_requested == 0) {
            self->task_must_cancel = 0;
        }
    }
    return PyLong_FromLong(self->task_num_cancels_requested);
}

/*[clinic input]
_asyncio.Task.get_stack

    cls: defining_class
    /
    *
    limit: object = None

Return the list of stack frames for this task's coroutine.

If the coroutine is not done, this returns the stack where it is
suspended.  If the coroutine has completed successfully or was
cancelled, this returns an empty list.  If the coroutine was
terminated by an exception, this returns the list of traceback
frames.

The frames are always ordered from oldest to newest.

The optional limit gives the maximum number of frames to
return; by default all available frames are returned.  Its
meaning differs depending on whether a stack or a traceback is
returned: the newest frames of a stack are returned, but the
oldest frames of a traceback are returned.  (This matches the
behavior of the traceback module.)

For reasons beyond our control, only one stack frame is
returned for a suspended coroutine.
[clinic start generated code]*/

static PyObject *
_asyncio_Task_get_stack_impl(TaskObj *self, PyTypeObject *cls,
                             PyObject *limit)
/*[clinic end generated code: output=6774dfc10d3857fa input=8e01c9b2618ae953]*/
{
    asyncio_state *state = get_asyncio_state_by_cls(cls);
    PyObject *stack[] = {(PyObject *)self, limit};
    return PyObject_Vectorcall(state->asyncio_task_get_stack_func,
                               stack, 2, NULL);
}

/*[clinic input]
_asyncio.Task.print_stack

    cls: defining_class
    /
    *
    limit: object = None
    file: object = None

Print the stack or traceback for this task's coroutine.

This produces output similar to that of the traceback module,
for the frames retrieved by get_stack().  The limit argument
is passed to get_stack().  The file argument is an I/O stream
to which the output is written; by default output is written
to sys.stderr.
[clinic start generated code]*/

static PyObject *
_asyncio_Task_print_stack_impl(TaskObj *self, PyTypeObject *cls,
                               PyObject *limit, PyObject *file)
/*[clinic end generated code: output=b38affe9289ec826 input=150b35ba2d3a7dee]*/
{
    asyncio_state *state = get_asyncio_state_by_cls(cls);
    PyObject *stack[] = {(PyObject *)self, limit, file};
    return PyObject_Vectorcall(state->asyncio_task_print_stack_func,
                               stack, 3, NULL);
}

/*[clinic input]
_asyncio.Task.set_result

    result: object
    /
[clinic start generated code]*/

static PyObject *
_asyncio_Task_set_result_impl(TaskObj *self, PyObject *result)
/*[clinic end generated code: output=e9d8e3cdaf18e258 input=9d1a00c07be41bab]*/
{
    PyErr_SetString(PyExc_RuntimeError,
                    "Task does not support set_result operation");
    return NULL;
}

/*[clinic input]
_asyncio.Task.set_exception

    exception: object
    /
[clinic start generated code]*/

static PyObject *
_asyncio_Task_set_exception_impl(TaskObj *self, PyObject *exception)
/*[clinic end generated code: output=96a91790c192cc7d input=9a8f65c83dcf893a]*/
{
    PyErr_SetString(PyExc_RuntimeError,
                    "Task does not support set_exception operation");
    return NULL;
}

/*[clinic input]
@critical_section
_asyncio.Task.get_coro
[clinic start generated code]*/

static PyObject *
_asyncio_Task_get_coro_impl(TaskObj *self)
/*[clinic end generated code: output=bcac27c8cc6c8073 input=a47f81427e39fe0c]*/
{
    if (self->task_coro) {
        return Py_NewRef(self->task_coro);
    }

    Py_RETURN_NONE;
}

/*[clinic input]
_asyncio.Task.get_context
[clinic start generated code]*/

static PyObject *
_asyncio_Task_get_context_impl(TaskObj *self)
/*[clinic end generated code: output=6996f53d3dc01aef input=87c0b209b8fceeeb]*/
{
    return Py_NewRef(self->task_context);
}

/*[clinic input]
@critical_section
_asyncio.Task.get_name
[clinic start generated code]*/

static PyObject *
_asyncio_Task_get_name_impl(TaskObj *self)
/*[clinic end generated code: output=0ecf1570c3b37a8f input=92a8f30c85034249]*/
{
    if (self->task_name) {
        if (PyLong_CheckExact(self->task_name)) {
            PyObject *name = PyUnicode_FromFormat("Task-%S", self->task_name);
            if (name == NULL) {
                return NULL;
            }
            Py_SETREF(self->task_name, name);
        }
        return Py_NewRef(self->task_name);
    }

    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
_asyncio.Task.set_name

    value: object
    /
[clinic start generated code]*/

static PyObject *
_asyncio_Task_set_name_impl(TaskObj *self, PyObject *value)
/*[clinic end generated code: output=f88ff4c0d64a9a6f input=e8d400ad64bad799]*/
{
    if (!PyUnicode_CheckExact(value)) {
        value = PyObject_Str(value);
        if (value == NULL) {
            return NULL;
        }
    } else {
        Py_INCREF(value);
    }

    Py_XSETREF(self->task_name, value);
    Py_RETURN_NONE;
}

static void
TaskObj_finalize(PyObject *op)
{
    TaskObj *task = (TaskObj*)op;
    PyObject *context;
    PyObject *message = NULL;
    PyObject *func;

    if (task->task_state != STATE_PENDING || !task->task_log_destroy_pending) {
        goto done;
    }

    /* Save the current exception, if any. */
    PyObject *exc = PyErr_GetRaisedException();

    context = PyDict_New();
    if (context == NULL) {
        goto finally;
    }

    message = PyUnicode_FromString("Task was destroyed but it is pending!");
    if (message == NULL) {
        goto finally;
    }

    if (PyDict_SetItem(context, &_Py_ID(message), message) < 0 ||
        PyDict_SetItem(context, &_Py_ID(task), (PyObject*)task) < 0)
    {
        goto finally;
    }

    if (task->task_source_tb != NULL) {
        if (PyDict_SetItem(context, &_Py_ID(source_traceback),
                              task->task_source_tb) < 0)
        {
            goto finally;
        }
    }

    func = PyObject_GetAttr(task->task_loop, &_Py_ID(call_exception_handler));
    if (func != NULL) {
        PyObject *res = PyObject_CallOneArg(func, context);
        if (res == NULL) {
            PyErr_FormatUnraisable("Exception ignored while calling asyncio "
                                   "function %R", func);
        }
        else {
            Py_DECREF(res);
        }
        Py_DECREF(func);
    }

finally:
    Py_XDECREF(context);
    Py_XDECREF(message);

    /* Restore the saved exception. */
    PyErr_SetRaisedException(exc);

done:
    FutureObj_finalize((PyObject*)task);
}

static void TaskObj_dealloc(PyObject *);  /* Needs Task_CheckExact */

static PyMethodDef TaskType_methods[] = {
    _ASYNCIO_FUTURE_RESULT_METHODDEF
    _ASYNCIO_FUTURE_EXCEPTION_METHODDEF
    _ASYNCIO_FUTURE_ADD_DONE_CALLBACK_METHODDEF
    _ASYNCIO_FUTURE_REMOVE_DONE_CALLBACK_METHODDEF
    _ASYNCIO_FUTURE_CANCELLED_METHODDEF
    _ASYNCIO_FUTURE_DONE_METHODDEF
    _ASYNCIO_TASK_SET_RESULT_METHODDEF
    _ASYNCIO_TASK_SET_EXCEPTION_METHODDEF
    _ASYNCIO_TASK_CANCEL_METHODDEF
    _ASYNCIO_TASK_CANCELLING_METHODDEF
    _ASYNCIO_TASK_UNCANCEL_METHODDEF
    _ASYNCIO_TASK_GET_STACK_METHODDEF
    _ASYNCIO_TASK_PRINT_STACK_METHODDEF
    _ASYNCIO_TASK__MAKE_CANCELLED_ERROR_METHODDEF
    _ASYNCIO_TASK_GET_NAME_METHODDEF
    _ASYNCIO_TASK_SET_NAME_METHODDEF
    _ASYNCIO_TASK_GET_CORO_METHODDEF
    _ASYNCIO_TASK_GET_CONTEXT_METHODDEF
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {NULL, NULL}        /* Sentinel */
};

static PyGetSetDef TaskType_getsetlist[] = {
    _ASYNCIO_TASK__LOG_DESTROY_PENDING_GETSETDEF
    _ASYNCIO_TASK__MUST_CANCEL_GETSETDEF
    _ASYNCIO_TASK__CORO_GETSETDEF
    _ASYNCIO_TASK__FUT_WAITER_GETSETDEF
    {NULL} /* Sentinel */
};

static PyType_Slot Task_slots[] = {
    {Py_tp_dealloc, TaskObj_dealloc},
    {Py_tp_repr, TaskObj_repr},
    {Py_tp_doc, (void *)_asyncio_Task___init____doc__},
    {Py_tp_traverse, TaskObj_traverse},
    {Py_tp_clear, TaskObj_clear},
    {Py_tp_iter, future_new_iter},
    {Py_tp_methods, TaskType_methods},
    {Py_tp_getset, TaskType_getsetlist},
    {Py_tp_init, _asyncio_Task___init__},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_finalize, TaskObj_finalize},

    // async slots
    {Py_am_await, future_new_iter},
    {0, NULL},
};

static PyType_Spec Task_spec = {
    .name = "_asyncio.Task",
    .basicsize = sizeof(TaskObj),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_MANAGED_DICT |
              Py_TPFLAGS_MANAGED_WEAKREF),
    .slots = Task_slots,
};

static void
TaskObj_dealloc(PyObject *self)
{
    _PyObject_ResurrectStart(self);
    // Unregister the task here so that even if any subclass of Task
    // which doesn't end up calling TaskObj_finalize not crashes.
    unregister_task((TaskObj *)self);

    PyObject_CallFinalizer(self);

    if (_PyObject_ResurrectEnd(self)) {
        return;
    }

    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);

    PyObject_ClearWeakRefs(self);

    (void)TaskObj_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int
task_call_step_soon(asyncio_state *state, TaskObj *task, PyObject *arg)
{
    PyObject *cb = TaskStepMethWrapper_new(task, arg);
    if (cb == NULL) {
        return -1;
    }

    // Beware: An evil call_soon could alter task_context.
    // See: https://github.com/python/cpython/issues/126080.
    PyObject *task_context = Py_NewRef(task->task_context);
    int ret = call_soon(state, task->task_loop, cb, NULL, task_context);
    Py_DECREF(task_context);
    Py_DECREF(cb);
    return ret;
}

static PyObject *
task_set_error_soon(asyncio_state *state, TaskObj *task, PyObject *et,
                    const char *format, ...)
{
    PyObject* msg;

    va_list vargs;
    va_start(vargs, format);
    msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);

    if (msg == NULL) {
        return NULL;
    }

    PyObject *e = PyObject_CallOneArg(et, msg);
    Py_DECREF(msg);
    if (e == NULL) {
        return NULL;
    }

    if (task_call_step_soon(state, task, e) == -1) {
        Py_DECREF(e);
        return NULL;
    }

    Py_DECREF(e);
    Py_RETURN_NONE;
}

static inline int
gen_status_from_result(PyObject **result)
{
    if (*result != NULL) {
        return PYGEN_NEXT;
    }
    if (_PyGen_FetchStopIterationValue(result) == 0) {
        return PYGEN_RETURN;
    }

    assert(PyErr_Occurred());
    return PYGEN_ERROR;
}

static PyObject *
task_step_impl(asyncio_state *state, TaskObj *task, PyObject *exc)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(task);

    int clear_exc = 0;
    PyObject *result = NULL;
    PyObject *coro;
    PyObject *o;

    if (task->task_state != STATE_PENDING) {
        PyErr_Format(state->asyncio_InvalidStateError,
                     "__step(): already done: %R %R",
                     task,
                     exc ? exc : Py_None);
        goto fail;
    }

    if (task->task_must_cancel) {
        assert(exc != Py_None);

        if (!exc || !PyErr_GivenExceptionMatches(exc, state->asyncio_CancelledError)) {
            /* exc was not a CancelledError */
            exc = create_cancelled_error(state, (FutureObj*)task);

            if (!exc) {
                goto fail;
            }
            clear_exc = 1;
        }

        task->task_must_cancel = 0;
    }

    Py_CLEAR(task->task_fut_waiter);

    coro = task->task_coro;
    if (coro == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "uninitialized Task object");
        if (clear_exc) {
            /* We created 'exc' during this call */
            Py_DECREF(exc);
        }
        return NULL;
    }

    int gen_status = PYGEN_ERROR;
    if (exc == NULL) {
        gen_status = PyIter_Send(coro, Py_None, &result);
    }
    else {
        result = PyObject_CallMethodOneArg(coro, &_Py_ID(throw), exc);
        gen_status = gen_status_from_result(&result);
        if (clear_exc) {
            /* We created 'exc' during this call */
            Py_DECREF(exc);
        }
    }

    if (gen_status == PYGEN_RETURN || gen_status == PYGEN_ERROR) {
        if (result != NULL) {
            /* The error is StopIteration and that means that
               the underlying coroutine has resolved */

            PyObject *tmp;
            if (task->task_must_cancel) {
                // Task is cancelled right before coro stops.
                task->task_must_cancel = 0;
                tmp = future_cancel(state, (FutureObj*)task,
                                    task->task_cancel_msg);
            }
            else {
                tmp = future_set_result(state, (FutureObj*)task, result);
            }

            Py_DECREF(result);

            if (tmp == NULL) {
                return NULL;
            }
            Py_DECREF(tmp);
            Py_RETURN_NONE;
        }

        if (PyErr_ExceptionMatches(state->asyncio_CancelledError)) {
            /* CancelledError */

            PyObject *exc = PyErr_GetRaisedException();
            assert(exc);

            FutureObj *fut = (FutureObj*)task;
            /* transfer ownership */
            fut->fut_cancelled_exc = exc;

            return future_cancel(state, fut, NULL);
        }

        /* Some other exception; pop it and call Task.set_exception() */
        PyObject *exc = PyErr_GetRaisedException();
        assert(exc);

        o = future_set_exception(state, (FutureObj*)task, exc);
        if (!o) {
            /* An exception in Task.set_exception() */
            Py_DECREF(exc);
            goto fail;
        }
        assert(o == Py_None);
        Py_DECREF(o);

        if (PyErr_GivenExceptionMatches(exc, PyExc_KeyboardInterrupt) ||
            PyErr_GivenExceptionMatches(exc, PyExc_SystemExit))
        {
            /* We've got a KeyboardInterrupt or a SystemError; re-raise it */
            PyErr_SetRaisedException(exc);
            goto fail;
        }

        Py_DECREF(exc);

        Py_RETURN_NONE;
    }

    PyObject *ret = task_step_handle_result_impl(state, task, result);
    return ret;

fail:
    return NULL;
}


static PyObject *
task_step_handle_result_impl(asyncio_state *state, TaskObj *task, PyObject *result)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(task);

    int res;
    PyObject *o;

    if (result == (PyObject*)task) {
        /* We have a task that wants to await on itself */
        goto self_await;
    }

    /* Check if `result` is FutureObj or TaskObj (and not a subclass) */
    if (Future_CheckExact(state, result) || Task_CheckExact(state, result)) {
        PyObject *wrapper;
        PyObject *tmp;
        FutureObj *fut = (FutureObj*)result;

        /* Check if `result` future is attached to a different loop */
        if (fut->fut_loop != task->task_loop) {
            goto different_loop;
        }

        if (!fut->fut_blocking) {
            goto yield_insteadof_yf;
        }
        int res;
        Py_BEGIN_CRITICAL_SECTION(result);
        res = future_awaited_by_add(state, (FutureObj *)result, (PyObject *)task);
        Py_END_CRITICAL_SECTION();
        if (res) {
            goto fail;
        }

        fut->fut_blocking = 0;

        /* result.add_done_callback(task._wakeup) */
        wrapper = PyCFunction_New(&TaskWakeupDef, (PyObject *)task);
        if (wrapper == NULL) {
            goto fail;
        }
        Py_BEGIN_CRITICAL_SECTION(result);
        tmp = future_add_done_callback(state,
            (FutureObj*)result, wrapper, task->task_context);
        Py_END_CRITICAL_SECTION();
        Py_DECREF(wrapper);
        if (tmp == NULL) {
            goto fail;
        }
        Py_DECREF(tmp);

        /* task._fut_waiter = result */
        task->task_fut_waiter = result;  /* no incref is necessary */

        if (task->task_must_cancel) {
            PyObject *r;
            int is_true;

            // Beware: An evil `__getattribute__` could
            // prematurely delete task->task_cancel_msg before the
            // task is cancelled, thereby causing a UAF crash.
            //
            // See https://github.com/python/cpython/issues/126138
            PyObject *task_cancel_msg = Py_NewRef(task->task_cancel_msg);
            r = PyObject_CallMethodOneArg(result, &_Py_ID(cancel),
                                          task_cancel_msg);
            Py_DECREF(task_cancel_msg);

            if (r == NULL) {
                return NULL;
            }
            is_true = PyObject_IsTrue(r);
            Py_DECREF(r);
            if (is_true < 0) {
                return NULL;
            }
            else if (is_true) {
                task->task_must_cancel = 0;
            }
        }

        Py_RETURN_NONE;
    }

    /* Check if `result` is None */
    if (result == Py_None) {
        /* Bare yield relinquishes control for one event loop iteration. */
        if (task_call_step_soon(state, task, NULL)) {
            goto fail;
        }
        return result;
    }

    /* Check if `result` is a Future-compatible object */
    if (PyObject_GetOptionalAttr(result, &_Py_ID(_asyncio_future_blocking), &o) < 0) {
        goto fail;
    }
    if (o != NULL && o != Py_None) {
        /* `result` is a Future-compatible object */
        PyObject *wrapper;
        PyObject *tmp;

        int blocking = PyObject_IsTrue(o);
        Py_DECREF(o);
        if (blocking < 0) {
            goto fail;
        }

        /* Check if `result` future is attached to a different loop */
        PyObject *oloop = get_future_loop(state, result);
        if (oloop == NULL) {
            goto fail;
        }
        if (oloop != task->task_loop) {
            Py_DECREF(oloop);
            goto different_loop;
        }
        Py_DECREF(oloop);

        if (!blocking) {
            goto yield_insteadof_yf;
        }

        if (TaskOrFuture_Check(state, result)) {
            int res;
            Py_BEGIN_CRITICAL_SECTION(result);
            res = future_awaited_by_add(state, (FutureObj *)result, (PyObject *)task);
            Py_END_CRITICAL_SECTION();
            if (res) {
                goto fail;
            }
        }

        /* result._asyncio_future_blocking = False */
        if (PyObject_SetAttr(
                result, &_Py_ID(_asyncio_future_blocking), Py_False) == -1) {
            goto fail;
        }

        wrapper = PyCFunction_New(&TaskWakeupDef, (PyObject *)task);
        if (wrapper == NULL) {
            goto fail;
        }

        /* result.add_done_callback(task._wakeup) */
        PyObject *add_cb = PyObject_GetAttr(
            result, &_Py_ID(add_done_callback));
        if (add_cb == NULL) {
            Py_DECREF(wrapper);
            goto fail;
        }
        PyObject *stack[2];
        stack[0] = wrapper;
        stack[1] = (PyObject *)task->task_context;
        EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, add_cb);
        tmp = PyObject_Vectorcall(add_cb, stack, 1, state->context_kwname);
        Py_DECREF(add_cb);
        Py_DECREF(wrapper);
        if (tmp == NULL) {
            goto fail;
        }
        Py_DECREF(tmp);

        /* task._fut_waiter = result */
        task->task_fut_waiter = result;  /* no incref is necessary */

        if (task->task_must_cancel) {
            PyObject *r;
            int is_true;

            // Beware: An evil `__getattribute__` could
            // prematurely delete task->task_cancel_msg before the
            // task is cancelled, thereby causing a UAF crash.
            //
            // See https://github.com/python/cpython/issues/126138
            PyObject *task_cancel_msg = Py_NewRef(task->task_cancel_msg);
            r = PyObject_CallMethodOneArg(result, &_Py_ID(cancel),
                                          task_cancel_msg);
            Py_DECREF(task_cancel_msg);

            if (r == NULL) {
                return NULL;
            }
            is_true = PyObject_IsTrue(r);
            Py_DECREF(r);
            if (is_true < 0) {
                return NULL;
            }
            else if (is_true) {
                task->task_must_cancel = 0;
            }
        }

        Py_RETURN_NONE;
    }

    Py_XDECREF(o);
    /* Check if `result` is a generator */
    res = PyObject_IsInstance(result, (PyObject*)&PyGen_Type);
    if (res < 0) {
        goto fail;
    }
    if (res) {
        /* `result` is a generator */
        o = task_set_error_soon(
            state, task, PyExc_RuntimeError,
            "yield was used instead of yield from for "
            "generator in task %R with %R", task, result);
        Py_DECREF(result);
        return o;
    }

    /* The `result` is none of the above */
    o = task_set_error_soon(
        state, task, PyExc_RuntimeError, "Task got bad yield: %R", result);
    Py_DECREF(result);
    return o;

self_await:
    o = task_set_error_soon(
        state, task, PyExc_RuntimeError,
        "Task cannot await on itself: %R", task);
    Py_DECREF(result);
    return o;

yield_insteadof_yf:
    o = task_set_error_soon(
        state, task, PyExc_RuntimeError,
        "yield was used instead of yield from "
        "in task %R with %R",
        task, result);
    Py_DECREF(result);
    return o;

different_loop:
    o = task_set_error_soon(
        state, task, PyExc_RuntimeError,
        "Task %R got Future %R attached to a different loop",
        task, result);
    Py_DECREF(result);
    return o;

fail:
    Py_XDECREF(result);
    return NULL;
}

static PyObject *
task_step(asyncio_state *state, TaskObj *task, PyObject *exc)
{
    PyObject *res;

    if (enter_task(task->task_loop, (PyObject*)task) < 0) {
        return NULL;
    }

    res = task_step_impl(state, task, exc);

    if (res == NULL) {
        PyObject *exc = PyErr_GetRaisedException();
        leave_task(task->task_loop, (PyObject*)task);
        _PyErr_ChainExceptions1(exc);
        return NULL;
    }
    else {
        if (leave_task(task->task_loop, (PyObject*)task) < 0) {
            Py_DECREF(res);
            return NULL;
        }
        else {
            return res;
        }
    }
}

static int
task_eager_start(asyncio_state *state, TaskObj *task)
{
    assert(task != NULL);
    PyObject *prevtask = swap_current_task(task->task_loop, (PyObject *)task);
    if (prevtask == NULL) {
        return -1;
    }
    // register the task into the linked list of tasks
    // if the task completes eagerly (without suspending) then it will unregister itself
    // in future_schedule_callbacks when done, otherwise
    // it will continue as a regular (non-eager) asyncio task
    register_task(task);

    if (PyContext_Enter(task->task_context) == -1) {
        Py_DECREF(prevtask);
        return -1;
    }

    int retval = 0;

    PyObject *stepres;
    Py_BEGIN_CRITICAL_SECTION(task);
    stepres = task_step_impl(state, task, NULL);
    Py_END_CRITICAL_SECTION();
    if (stepres == NULL) {
        PyObject *exc = PyErr_GetRaisedException();
        _PyErr_ChainExceptions1(exc);
        retval = -1;
    } else {
        Py_DECREF(stepres);
    }

    PyObject *curtask = swap_current_task(task->task_loop, prevtask);
    Py_DECREF(prevtask);
    if (curtask == NULL) {
        retval = -1;
    } else {
        assert(curtask == (PyObject *)task);
        Py_DECREF(curtask);
    }

    if (PyContext_Exit(task->task_context) == -1) {
        retval = -1;
    }

    if (task->task_state != STATE_PENDING) {
        // This seems to really help performance on pyperformance benchmarks
        clear_task_coro(task);
    }

    return retval;
}

static PyObject *
task_wakeup_lock_held(TaskObj *task, PyObject *o)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(task);

    PyObject *result;
    assert(o);

    asyncio_state *state = get_asyncio_state_by_def((PyObject *)task);

    if (TaskOrFuture_Check(state, o)) {
        int res;
        Py_BEGIN_CRITICAL_SECTION(o);
        res = future_awaited_by_discard(state, (FutureObj *)o, (PyObject *)task);
        Py_END_CRITICAL_SECTION();
        if (res) {
            return NULL;
        }
    }

    if (Future_CheckExact(state, o) || Task_CheckExact(state, o)) {
        PyObject *fut_result = NULL;
        int res;
        Py_BEGIN_CRITICAL_SECTION(o);
        res = future_get_result(state, (FutureObj*)o, &fut_result);
        Py_END_CRITICAL_SECTION();
        switch(res) {
        case -1:
            assert(fut_result == NULL);
            break; /* exception raised */
        case 0:
            Py_DECREF(fut_result);
            return task_step(state, task, NULL);
        default:
            assert(res == 1);
            result = task_step(state, task, fut_result);
            Py_DECREF(fut_result);
            return result;
        }
    }
    else {
        PyObject *fut_result = PyObject_CallMethod(o, "result", NULL);
        if (fut_result != NULL) {
            Py_DECREF(fut_result);
            return task_step(state, task, NULL);
        }
        /* exception raised */
    }

    PyObject *exc = PyErr_GetRaisedException();
    assert(exc);

    result = task_step(state, task, exc);

    Py_DECREF(exc);

    return result;
}

static PyObject *
task_wakeup(PyObject *op, PyObject *arg)
{
    TaskObj *task = (TaskObj*)op;
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(task);
    res = task_wakeup_lock_held(task, arg);
    Py_END_CRITICAL_SECTION();
    return res;
}


/*********************** Functions **************************/


/*[clinic input]
_asyncio._get_running_loop

Return the running event loop or None.

This is a low-level function intended to be used by event loops.
This function is thread-specific.

[clinic start generated code]*/

static PyObject *
_asyncio__get_running_loop_impl(PyObject *module)
/*[clinic end generated code: output=b4390af721411a0a input=0a21627e25a4bd43]*/
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();
    PyObject *loop = Py_XNewRef(ts->asyncio_running_loop);
    if (loop == NULL) {
        /* There's no currently running event loop */
        Py_RETURN_NONE;
    }
    return loop;
}

/*[clinic input]
_asyncio._set_running_loop
    loop: 'O'
    /

Set the running event loop.

This is a low-level function intended to be used by event loops.
This function is thread-specific.
[clinic start generated code]*/

static PyObject *
_asyncio__set_running_loop(PyObject *module, PyObject *loop)
/*[clinic end generated code: output=ae56bf7a28ca189a input=4c9720233d606604]*/
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();
    if (loop == Py_None) {
        loop = NULL;
    }
    Py_XSETREF(ts->asyncio_running_loop, Py_XNewRef(loop));
    Py_RETURN_NONE;
}

/*[clinic input]
_asyncio.get_event_loop

Return an asyncio event loop.

When called from a coroutine or a callback (e.g. scheduled with
call_soon or similar API), this function will always return the
running event loop.

If there is no running event loop set, the function will return
the result of `get_event_loop_policy().get_event_loop()` call.
[clinic start generated code]*/

static PyObject *
_asyncio_get_event_loop_impl(PyObject *module)
/*[clinic end generated code: output=2a2d8b2f824c648b input=9364bf2916c8655d]*/
{
    asyncio_state *state = get_asyncio_state(module);
    return get_event_loop(state);
}

/*[clinic input]
_asyncio.get_running_loop

Return the running event loop.  Raise a RuntimeError if there is none.

This function is thread-specific.
[clinic start generated code]*/

static PyObject *
_asyncio_get_running_loop_impl(PyObject *module)
/*[clinic end generated code: output=c247b5f9e529530e input=2a3bf02ba39f173d]*/
{
    PyObject *loop;
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();
    loop = Py_XNewRef(ts->asyncio_running_loop);
    if (loop == NULL) {
        /* There's no currently running event loop */
        PyErr_SetString(
            PyExc_RuntimeError, "no running event loop");
        return NULL;
    }
    return loop;
}

/*[clinic input]
_asyncio._register_task

    task: object

Register a new task in asyncio as executed by loop.

Returns None.
[clinic start generated code]*/

static PyObject *
_asyncio__register_task_impl(PyObject *module, PyObject *task)
/*[clinic end generated code: output=8672dadd69a7d4e2 input=21075aaea14dfbad]*/
{
    asyncio_state *state = get_asyncio_state(module);
    if (Task_Check(state, task)) {
        // task is an asyncio.Task instance or subclass, use efficient
        // linked-list implementation.
        register_task((TaskObj *)task);
        Py_RETURN_NONE;
    }
    // As task does not inherit from asyncio.Task, fallback to less efficient
    // weakset implementation.
    PyObject *res = PyObject_CallMethodOneArg(state->non_asyncio_tasks,
                                              &_Py_ID(add), task);
    if (res == NULL) {
        return NULL;
    }
    Py_DECREF(res);
    Py_RETURN_NONE;
}

/*[clinic input]
_asyncio._register_eager_task

    task: object

Register a new task in asyncio as executed by loop.

Returns None.
[clinic start generated code]*/

static PyObject *
_asyncio__register_eager_task_impl(PyObject *module, PyObject *task)
/*[clinic end generated code: output=dfe1d45367c73f1a input=237f684683398c51]*/
{
    asyncio_state *state = get_asyncio_state(module);

    if (Task_Check(state, task)) {
        // task is an asyncio.Task instance or subclass, use efficient
        // linked-list implementation.
        register_task((TaskObj *)task);
        Py_RETURN_NONE;
    }

    if (PySet_Add(state->non_asyncio_eager_tasks, task) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


/*[clinic input]
_asyncio._unregister_task

    task: object

Unregister a task.

Returns None.
[clinic start generated code]*/

static PyObject *
_asyncio__unregister_task_impl(PyObject *module, PyObject *task)
/*[clinic end generated code: output=6e5585706d568a46 input=28fb98c3975f7bdc]*/
{
    asyncio_state *state = get_asyncio_state(module);
    if (Task_Check(state, task)) {
        unregister_task((TaskObj *)task);
        Py_RETURN_NONE;
    }
    PyObject *res = PyObject_CallMethodOneArg(state->non_asyncio_tasks,
                                              &_Py_ID(discard), task);
    if (res == NULL) {
        return NULL;
    }
    Py_DECREF(res);
    Py_RETURN_NONE;
}

/*[clinic input]
_asyncio._unregister_eager_task

    task: object

Unregister a task.

Returns None.
[clinic start generated code]*/

static PyObject *
_asyncio__unregister_eager_task_impl(PyObject *module, PyObject *task)
/*[clinic end generated code: output=a426922bd07f23d1 input=9d07401ef14ee048]*/
{
    asyncio_state *state = get_asyncio_state(module);
    if (Task_Check(state, task)) {
        // task is an asyncio.Task instance or subclass, use efficient
        // linked-list implementation.
        unregister_task((TaskObj *)task);
        Py_RETURN_NONE;
    }

    if (PySet_Discard(state->non_asyncio_eager_tasks, task) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}


/*[clinic input]
_asyncio._enter_task

    loop: object
    task: object

Enter into task execution or resume suspended task.

Task belongs to loop.

Returns None.
[clinic start generated code]*/

static PyObject *
_asyncio__enter_task_impl(PyObject *module, PyObject *loop, PyObject *task)
/*[clinic end generated code: output=a22611c858035b73 input=de1b06dca70d8737]*/
{
    if (enter_task(loop, task) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_asyncio._leave_task

    loop: object
    task: object

Leave task execution or suspend a task.

Task belongs to loop.

Returns None.
[clinic start generated code]*/

static PyObject *
_asyncio__leave_task_impl(PyObject *module, PyObject *loop, PyObject *task)
/*[clinic end generated code: output=0ebf6db4b858fb41 input=51296a46313d1ad8]*/
{
    if (leave_task(loop, task) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_asyncio._swap_current_task

    loop: object
    task: object

Temporarily swap in the supplied task and return the original one (or None).

This is intended for use during eager coroutine execution.

[clinic start generated code]*/

static PyObject *
_asyncio__swap_current_task_impl(PyObject *module, PyObject *loop,
                                 PyObject *task)
/*[clinic end generated code: output=9f88de958df74c7e input=c9c72208d3d38b6c]*/
{
    return swap_current_task(loop, task);
}


/*[clinic input]
_asyncio.current_task

    loop: object = None

Return a currently executed task.

[clinic start generated code]*/

static PyObject *
_asyncio_current_task_impl(PyObject *module, PyObject *loop)
/*[clinic end generated code: output=fe15ac331a7f981a input=58910f61a5627112]*/
{
    if (loop == Py_None) {
        loop = _asyncio_get_running_loop_impl(module);
        if (loop == NULL) {
            return NULL;
        }
    } else {
        Py_INCREF(loop);
    }

    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();
    // Fast path for the current running loop of current thread
    // no locking or stop the world pause is required
    if (ts->asyncio_running_loop == loop) {
        if (ts->asyncio_running_task != NULL) {
            Py_DECREF(loop);
            return Py_NewRef(ts->asyncio_running_task);
        }
        Py_DECREF(loop);
        Py_RETURN_NONE;
    }

    PyObject *ret = Py_None;
    // Stop the world and traverse the per-thread current tasks
    // and return the task if the loop matches
    PyInterpreterState *interp = ts->base.interp;
    _PyEval_StopTheWorld(interp);
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        ts = (_PyThreadStateImpl *)p;
        if (ts->asyncio_running_loop == loop) {
            if (ts->asyncio_running_task != NULL) {
                ret = Py_NewRef(ts->asyncio_running_task);
            }
            goto exit;
        }
    }
exit:
    _Py_FOR_EACH_TSTATE_END(interp);
    _PyEval_StartTheWorld(interp);
    Py_DECREF(loop);
    return ret;
}


static inline int
add_one_task(asyncio_state *state, PyObject *tasks, PyObject *task, PyObject *loop)
{
    assert(PySet_CheckExact(tasks));
    if (Task_CheckExact(state, task)) {
        int pending = 0;
        Py_BEGIN_CRITICAL_SECTION(task);
        pending = ((TaskObj *)task)->task_state == STATE_PENDING && ((TaskObj *)task)->task_loop == loop;
        Py_END_CRITICAL_SECTION();
        if (pending) {
            if (PySet_Add(tasks, task) < 0) {
                return -1;
            }
        }
        return 0;
    }

    PyObject *done = PyObject_CallMethodNoArgs(task, &_Py_ID(done));
    if (done == NULL) {
        return -1;
    }
    if (Py_IsTrue(done)) {
        return 0;
    }
    Py_DECREF(done);
    PyObject *task_loop = get_future_loop(state, task);
    if (task_loop == NULL) {
        return -1;
    }
    if (task_loop == loop) {
        if (PySet_Add(tasks, task) < 0) {
            Py_DECREF(task_loop);
            return -1;
        }
    }
    Py_DECREF(task_loop);
    return 0;
}

static inline int
add_tasks_llist(struct llist_node *head, PyListObject *tasks)
{
    struct llist_node *node;
    llist_for_each_safe(node, head) {
        TaskObj *task = llist_data(node, TaskObj, task_node);
        assert(task->task_state == STATE_PENDING);
        // The linked list holds borrowed references to task
        // as such it is possible that the task is concurrently
        // deallocated while added to this list.
        // To protect against concurrent deallocations,
        // we first try to incref the task which would fail
        // if it is concurrently getting deallocated in another thread,
        // otherwise it gets added to the list.
        if (_Py_TryIncref((PyObject *)task)) {
            if (_PyList_AppendTakeRef(tasks, (PyObject *)task) < 0) {
                // do not call any escaping calls here while the world is stopped.
                return -1;
            }
        }
    }
    return 0;
}

static inline int
add_tasks_interp(PyInterpreterState *interp, PyListObject *tasks)
{
#ifdef Py_GIL_DISABLED
    assert(interp->stoptheworld.world_stopped);
#endif
    // Start traversing from interpreter's linked list
    struct llist_node *head = &interp->asyncio_tasks_head;

    if (add_tasks_llist(head, tasks) < 0) {
        return -1;
    }

    int ret = 0;
    // traverse the task lists of thread states
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        _PyThreadStateImpl *ts = (_PyThreadStateImpl *)p;
        head = &ts->asyncio_tasks_head;
        if (add_tasks_llist(head, tasks) < 0) {
            ret = -1;
            goto exit;
        }
    }
exit:
    _Py_FOR_EACH_TSTATE_END(interp);
    return ret;
}

/*********************** Module **************************/

/*[clinic input]
_asyncio.all_tasks

    loop: object = None

Return a set of all tasks for the loop.

[clinic start generated code]*/

static PyObject *
_asyncio_all_tasks_impl(PyObject *module, PyObject *loop)
/*[clinic end generated code: output=0e107cbb7f72aa7b input=43a1b423c2d95bfa]*/
{
    asyncio_state *state = get_asyncio_state(module);
    if (loop == Py_None) {
        loop = _asyncio_get_running_loop_impl(module);
        if (loop == NULL) {
            return NULL;
        }
    } else {
        Py_INCREF(loop);
    }
    // First add eager tasks to the list so that we don't miss
    // any tasks which graduates from eager to non-eager
    // We first add all the tasks to `tasks` list and then filter
    // out the tasks which are done and return it as a set.
    PyObject *tasks = PyList_New(0);
    if (tasks == NULL) {
        Py_DECREF(loop);
        return NULL;
    }
    if (PyList_Extend(tasks, state->non_asyncio_eager_tasks) < 0) {
        Py_DECREF(tasks);
        Py_DECREF(loop);
        return NULL;
    }
    if (PyList_Extend(tasks, state->non_asyncio_tasks) < 0) {
        Py_DECREF(tasks);
        Py_DECREF(loop);
        return NULL;
    }

    PyInterpreterState *interp = PyInterpreterState_Get();
    // Stop the world and traverse the per-thread linked list
    // of asyncio tasks for every thread, as well as the
    // interpreter's linked list, and add them to `tasks`.
    // The interpreter linked list is used for any lingering tasks
    // whose thread state has been deallocated while the task was
    // still alive. This can happen if a task is referenced by
    // a different thread, in which case the task is moved to
    // the interpreter's linked list from the thread's linked
    // list before deallocation. See PyThreadState_Clear.
    //
    // The stop-the-world pause is required so that no thread
    // modifies its linked list while being iterated here
    // in parallel. This design allows for lock-free
    // register_task/unregister_task for loops running in parallel
    // in different threads (the general case).
    _PyEval_StopTheWorld(interp);
    int ret = add_tasks_interp(interp, (PyListObject *)tasks);
    _PyEval_StartTheWorld(interp);
    if (ret < 0) {
        // call any escaping calls after starting the world to avoid any deadlocks.
        Py_DECREF(tasks);
        Py_DECREF(loop);
        return NULL;
    }

    // All the tasks are now in the list, now filter the tasks which are done
    PyObject *res = PySet_New(NULL);
    if (res == NULL) {
        Py_DECREF(tasks);
        Py_DECREF(loop);
        return NULL;
    }

    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(tasks); i++) {
        PyObject *task = PyList_GET_ITEM(tasks, i);
        if (add_one_task(state, res, task, loop) < 0) {
            Py_DECREF(res);
            Py_DECREF(tasks);
            Py_DECREF(loop);
            return NULL;
        }
    }

    Py_DECREF(tasks);
    Py_DECREF(loop);
    return res;
}

/*[clinic input]
_asyncio.future_add_to_awaited_by

    fut: object
    waiter: object
    /

Record that `fut` is awaited on by `waiter`.

[clinic start generated code]*/

static PyObject *
_asyncio_future_add_to_awaited_by_impl(PyObject *module, PyObject *fut,
                                       PyObject *waiter)
/*[clinic end generated code: output=0ab9a1a63389e4df input=06e6eaac51f532b9]*/
{
    asyncio_state *state = get_asyncio_state(module);
    if (TaskOrFuture_Check(state, fut) && TaskOrFuture_Check(state, waiter)) {
        int res;
        Py_BEGIN_CRITICAL_SECTION(fut);
        res = future_awaited_by_add(state, (FutureObj *)fut, waiter);
        Py_END_CRITICAL_SECTION();
        if (res) {
            return NULL;
        }
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_asyncio.future_discard_from_awaited_by

    fut: object
    waiter: object
    /

[clinic start generated code]*/

static PyObject *
_asyncio_future_discard_from_awaited_by_impl(PyObject *module, PyObject *fut,
                                             PyObject *waiter)
/*[clinic end generated code: output=a03b0b4323b779de input=3833f7639e88e483]*/
{
    asyncio_state *state = get_asyncio_state(module);
    if (TaskOrFuture_Check(state, fut) && TaskOrFuture_Check(state, waiter)) {
        int res;
        Py_BEGIN_CRITICAL_SECTION(fut);
        res = future_awaited_by_discard(state, (FutureObj *)fut, waiter);
        Py_END_CRITICAL_SECTION();
        if (res) {
            return NULL;
        }
    }
    Py_RETURN_NONE;
}

static int
module_traverse(PyObject *mod, visitproc visit, void *arg)
{
    asyncio_state *state = get_asyncio_state(mod);

    Py_VISIT(state->FutureIterType);
    Py_VISIT(state->TaskStepMethWrapper_Type);
    Py_VISIT(state->FutureType);
    Py_VISIT(state->TaskType);

    Py_VISIT(state->asyncio_mod);
    Py_VISIT(state->traceback_extract_stack);
    Py_VISIT(state->asyncio_future_repr_func);
    Py_VISIT(state->asyncio_get_event_loop_policy);
    Py_VISIT(state->asyncio_iscoroutine_func);
    Py_VISIT(state->asyncio_task_get_stack_func);
    Py_VISIT(state->asyncio_task_print_stack_func);
    Py_VISIT(state->asyncio_task_repr_func);
    Py_VISIT(state->asyncio_InvalidStateError);
    Py_VISIT(state->asyncio_CancelledError);

    Py_VISIT(state->non_asyncio_tasks);
    Py_VISIT(state->non_asyncio_eager_tasks);
    Py_VISIT(state->iscoroutine_typecache);

    Py_VISIT(state->context_kwname);

    return 0;
}

static int
module_clear(PyObject *mod)
{
    asyncio_state *state = get_asyncio_state(mod);

    Py_CLEAR(state->FutureIterType);
    Py_CLEAR(state->TaskStepMethWrapper_Type);
    Py_CLEAR(state->FutureType);
    Py_CLEAR(state->TaskType);

    Py_CLEAR(state->asyncio_mod);
    Py_CLEAR(state->traceback_extract_stack);
    Py_CLEAR(state->asyncio_future_repr_func);
    Py_CLEAR(state->asyncio_get_event_loop_policy);
    Py_CLEAR(state->asyncio_iscoroutine_func);
    Py_CLEAR(state->asyncio_task_get_stack_func);
    Py_CLEAR(state->asyncio_task_print_stack_func);
    Py_CLEAR(state->asyncio_task_repr_func);
    Py_CLEAR(state->asyncio_InvalidStateError);
    Py_CLEAR(state->asyncio_CancelledError);

    Py_CLEAR(state->non_asyncio_tasks);
    Py_CLEAR(state->non_asyncio_eager_tasks);
    Py_CLEAR(state->iscoroutine_typecache);

    Py_CLEAR(state->context_kwname);
    // Clear the ref to running loop so that finalizers can run early.
    // If there are other running loops in different threads,
    // those get cleared in PyThreadState_Clear.
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();
    Py_CLEAR(ts->asyncio_running_loop);
    Py_CLEAR(ts->asyncio_running_task);

    return 0;
}

static void
module_free(void *mod)
{
    (void)module_clear((PyObject *)mod);
}

static int
module_init(asyncio_state *state)
{
    PyObject *module = NULL;

    state->asyncio_mod = PyImport_ImportModule("asyncio");
    if (state->asyncio_mod == NULL) {
        goto fail;
    }

    state->iscoroutine_typecache = PySet_New(NULL);
    if (state->iscoroutine_typecache == NULL) {
        goto fail;
    }

    state->context_kwname = Py_BuildValue("(s)", "context");
    if (state->context_kwname == NULL) {
        goto fail;
    }

#define WITH_MOD(NAME) \
    Py_CLEAR(module); \
    module = PyImport_ImportModule(NAME); \
    if (module == NULL) { \
        goto fail; \
    }

#define GET_MOD_ATTR(VAR, NAME) \
    VAR = PyObject_GetAttrString(module, NAME); \
    if (VAR == NULL) { \
        goto fail; \
    }

    WITH_MOD("asyncio.events")
    GET_MOD_ATTR(state->asyncio_get_event_loop_policy, "_get_event_loop_policy")

    WITH_MOD("asyncio.base_futures")
    GET_MOD_ATTR(state->asyncio_future_repr_func, "_future_repr")

    WITH_MOD("asyncio.exceptions")
    GET_MOD_ATTR(state->asyncio_InvalidStateError, "InvalidStateError")
    GET_MOD_ATTR(state->asyncio_CancelledError, "CancelledError")

    WITH_MOD("asyncio.base_tasks")
    GET_MOD_ATTR(state->asyncio_task_repr_func, "_task_repr")
    GET_MOD_ATTR(state->asyncio_task_get_stack_func, "_task_get_stack")
    GET_MOD_ATTR(state->asyncio_task_print_stack_func, "_task_print_stack")

    WITH_MOD("asyncio.coroutines")
    GET_MOD_ATTR(state->asyncio_iscoroutine_func, "iscoroutine")

    WITH_MOD("traceback")
    GET_MOD_ATTR(state->traceback_extract_stack, "extract_stack")

    PyObject *weak_set;
    WITH_MOD("weakref")
    GET_MOD_ATTR(weak_set, "WeakSet");
    state->non_asyncio_tasks = PyObject_CallNoArgs(weak_set);
    Py_CLEAR(weak_set);
    if (state->non_asyncio_tasks == NULL) {
        goto fail;
    }

    state->non_asyncio_eager_tasks = PySet_New(NULL);
    if (state->non_asyncio_eager_tasks == NULL) {
        goto fail;
    }

    state->debug_offsets = &_AsyncioDebug;

    Py_DECREF(module);
    return 0;

fail:
    Py_CLEAR(module);
    return -1;

#undef WITH_MOD
#undef GET_MOD_ATTR
}

PyDoc_STRVAR(module_doc, "Accelerator module for asyncio");

static PyMethodDef asyncio_methods[] = {
    _ASYNCIO_CURRENT_TASK_METHODDEF
    _ASYNCIO_GET_EVENT_LOOP_METHODDEF
    _ASYNCIO_GET_RUNNING_LOOP_METHODDEF
    _ASYNCIO__GET_RUNNING_LOOP_METHODDEF
    _ASYNCIO__SET_RUNNING_LOOP_METHODDEF
    _ASYNCIO__REGISTER_TASK_METHODDEF
    _ASYNCIO__REGISTER_EAGER_TASK_METHODDEF
    _ASYNCIO__UNREGISTER_TASK_METHODDEF
    _ASYNCIO__UNREGISTER_EAGER_TASK_METHODDEF
    _ASYNCIO__ENTER_TASK_METHODDEF
    _ASYNCIO__LEAVE_TASK_METHODDEF
    _ASYNCIO__SWAP_CURRENT_TASK_METHODDEF
    _ASYNCIO_ALL_TASKS_METHODDEF
    _ASYNCIO_FUTURE_ADD_TO_AWAITED_BY_METHODDEF
    _ASYNCIO_FUTURE_DISCARD_FROM_AWAITED_BY_METHODDEF
    {NULL, NULL}
};

static int
module_exec(PyObject *mod)
{
    asyncio_state *state = get_asyncio_state(mod);


#define CREATE_TYPE(m, tp, spec, base)                                  \
    do {                                                                \
        tp = (PyTypeObject *)PyType_FromMetaclass(NULL, m, spec,        \
                                                  (PyObject *)base);    \
        if (tp == NULL) {                                               \
            return -1;                                                  \
        }                                                               \
    } while (0)

    CREATE_TYPE(mod, state->TaskStepMethWrapper_Type, &TaskStepMethWrapper_spec, NULL);
    CREATE_TYPE(mod, state->FutureIterType, &FutureIter_spec, NULL);
    CREATE_TYPE(mod, state->FutureType, &Future_spec, NULL);
    CREATE_TYPE(mod, state->TaskType, &Task_spec, state->FutureType);

#undef CREATE_TYPE

    if (PyModule_AddType(mod, state->FutureType) < 0) {
        return -1;
    }

    if (PyModule_AddType(mod, state->TaskType) < 0) {
        return -1;
    }
    // Must be done after types are added to avoid a circular dependency
    if (module_init(state) < 0) {
        return -1;
    }

    return 0;
}

static struct PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef _asynciomodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_asyncio",
    .m_doc = module_doc,
    .m_size = sizeof(asyncio_state),
    .m_methods = asyncio_methods,
    .m_slots = module_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = module_free,
};

PyMODINIT_FUNC
PyInit__asyncio(void)
{
    return PyModuleDef_Init(&_asynciomodule);
}
