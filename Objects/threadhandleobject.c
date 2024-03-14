/* ThreadHandle object (see Modules/_threadmodule.c) */

#include "Python.h"
#include "pycore_lock.h"          // _PyEventRc
#include "pycore_pystate.h"       // HEAD_LOCK()
#include "pycore_pythread.h"      // ThreadHandleState
#include "pycore_runtime.h"       // _PyRuntime


// ThreadHandleError is just an alias to PyExc_RuntimeError.
#define ThreadHandleError PyExc_RuntimeError


/**********************/
/* thread handle data */
/**********************/

// A handle around an OS thread.
//
// The OS thread is either joined or detached after the handle is destroyed.
//
// Joining the handle is idempotent; the underlying OS thread is joined or
// detached only once. Concurrent join operations are serialized until it is
// their turn to execute or an earlier operation completes successfully. Once a
// join has completed successfully all future joins complete immediately.
struct _PyThread_handle_data {
    struct llist_node fork_node;  // linked list node (see _pythread_runtime_state)

    // The `ident` and `handle` fields are immutable once the object is visible
    // to threads other than its creator, thus they do not need to be accessed
    // atomically.
    PyThread_ident_t ident;
    PyThread_handle_t handle;

    // Holds a value from the `ThreadHandleState` enum.
    int state;

    // Set immediately before `thread_run` returns to indicate that the OS
    // thread is about to exit. This is used to avoid false positives when
    // detecting self-join attempts. See the comment in `ThreadHandle_join()`
    // for a more detailed explanation.
    _PyEventRc *thread_is_exiting;

    // Serializes calls to `join`.
    _PyOnceFlag once;
};


static void track_thread_handle_for_fork(struct _PyThread_handle_data *);

static struct _PyThread_handle_data *
new_thread_handle_data(void)
{
    _PyEventRc *event = _PyEventRc_New();
    if (event == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    struct _PyThread_handle_data *data = \
            PyMem_RawMalloc(sizeof(struct _PyThread_handle_data));
    if (data == NULL) {
        _PyEventRc_Decref(event);
        return NULL;
    }
    *data = (struct _PyThread_handle_data){
        .thread_is_exiting = event,
        .state = THREAD_HANDLE_INVALID,
    };

    track_thread_handle_for_fork(data);

    return data;
}

static inline int get_thread_handle_state(struct _PyThread_handle_data *);
static inline void set_thread_handle_state(
        struct _PyThread_handle_data *, _PyThreadHandleState);
static void untrack_thread_handle_for_fork(struct _PyThread_handle_data *);


static void
free_thread_handle_data(struct _PyThread_handle_data *data)
{
    if (get_thread_handle_state(data) == THREAD_HANDLE_RUNNING) {
        // This is typically short so no need to release the GIL
        if (PyThread_detach_thread(data->handle)) {
            PyErr_SetString(ThreadHandleError, "Failed detaching thread");
            PyErr_WriteUnraisable(NULL);
        }
        else {
            set_thread_handle_state(data, THREAD_HANDLE_DETACHED);
        }
    }
    untrack_thread_handle_for_fork(data);
    _PyEventRc_Decref(data->thread_is_exiting);
    PyMem_RawFree(data);
}


/***************************/
/* internal implementation */
/***************************/

static inline int
get_thread_handle_state(struct _PyThread_handle_data *data)
{
    return _Py_atomic_load_int(&data->state);
}

static inline void
set_thread_handle_state(struct _PyThread_handle_data *data,
                        _PyThreadHandleState state)
{
    _Py_atomic_store_int(&data->state, state);
}

static int
_join_thread(struct _PyThread_handle_data *data)
{
    assert(get_thread_handle_state(data) == THREAD_HANDLE_RUNNING);

    int err;
    Py_BEGIN_ALLOW_THREADS
    err = PyThread_join_thread(data->handle);
    Py_END_ALLOW_THREADS
    if (err) {
        PyErr_SetString(ThreadHandleError, "Failed joining thread");
        return -1;
    }
    set_thread_handle_state(data, THREAD_HANDLE_JOINED);
    return 0;
}

static int
join_thread(struct _PyThread_handle_data *data)
{
    if (get_thread_handle_state(data) == THREAD_HANDLE_INVALID) {
        PyErr_SetString(PyExc_ValueError,
                        "the handle is invalid and thus cannot be joined");
        return -1;
    }

    // We want to perform this check outside of the `_PyOnceFlag` to prevent
    // deadlock in the scenario where another thread joins us and we then
    // attempt to join ourselves. However, it's not safe to check thread
    // identity once the handle's os thread has finished. We may end up reusing
    // the identity stored in the handle and erroneously think we are
    // attempting to join ourselves.
    //
    // To work around this, we set `thread_is_exiting` immediately before
    // `thread_run` returns.  We can be sure that we are not attempting to join
    // ourselves if the handle's thread is about to exit.
    if (!_PyEvent_IsSet(&data->thread_is_exiting->event) &&
        data->ident == PyThread_get_thread_ident_ex())
    {
        // PyThread_join_thread() would deadlock or error out.
        PyErr_SetString(ThreadHandleError, "Cannot join current thread");
        return -1;
    }

    if (_PyOnceFlag_CallOnce(
                &data->once, (_Py_once_fn_t *)_join_thread, data) == -1)
    {
        return -1;
    }
    assert(get_thread_handle_state(data) == THREAD_HANDLE_JOINED);
    return 0;
}


/*********************************/
/* thread handle C-API functions */
/*********************************/

static void
_PyThread_SetStarted(struct _PyThread_handle_data *data,
                     PyThread_handle_t handle, PyThread_ident_t ident)
{
    assert(get_thread_handle_state(data) == THREAD_HANDLE_INVALID);
    data->handle = handle;
    data->ident = ident;
    set_thread_handle_state(data, THREAD_HANDLE_RUNNING);
}


static _PyEventRc *
_PyThread_GetExitingEvent(struct _PyThread_handle_data *data)
{
    return data->thread_is_exiting;
}


/*************************/
/* _ThreadHandle objects */
/*************************/

typedef struct {
    PyObject_HEAD

    struct _PyThread_handle_data *data;
} thandleobject;


PyObject *
_PyThreadHandle_NewObject(void)
{
    thandleobject *self = PyObject_New(thandleobject, &_PyThreadHandle_Type);
    if (self == NULL) {
        return NULL;
    }
    self->data = new_thread_handle_data();
    if (self->data == NULL) {
        Py_DECREF(self);
        return NULL;
    }

    return (PyObject *)self;
}


/* _ThreadHandle instance methods */

static PyObject *
ThreadHandle_join(thandleobject *self, void* ignored)
{
    if (join_thread(self->data) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef ThreadHandle_methods[] =
{
    {"join", (PyCFunction)ThreadHandle_join, METH_NOARGS},
    {0, 0}
};


/* _ThreadHandle instance properties */

static PyObject *
ThreadHandle_get_ident(thandleobject *self, void *ignored)
{
    return PyLong_FromUnsignedLongLong(self->data->ident);
}


static PyGetSetDef ThreadHandle_getsetlist[] = {
    {"ident", (getter)ThreadHandle_get_ident, NULL, NULL},
    {0},
};


/* The _ThreadHandle class */

static void
ThreadHandle_dealloc(thandleobject *self)
{
    // It's safe to access state non-atomically:
    //   1. This is the destructor; nothing else holds a reference.
    //   2. The refcount going to zero is a "synchronizes-with" event;
    //      all changes from other threads are visible.
    free_thread_handle_data(self->data);
    PyObject_Free(self);
}


static PyObject *
ThreadHandle_repr(thandleobject *self)
{
    return PyUnicode_FromFormat("<%s object: ident=%" PY_FORMAT_THREAD_IDENT_T ">",
                                Py_TYPE(self)->tp_name, self->data->ident);
}


PyTypeObject _PyThreadHandle_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "_ThreadHandle",
    .tp_basicsize = sizeof(thandleobject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)ThreadHandle_dealloc,
    .tp_repr = (reprfunc)ThreadHandle_repr,
    .tp_getset = ThreadHandle_getsetlist,
    .tp_methods = ThreadHandle_methods,
};


/************************************/
/* tracking thread handles for fork */
/************************************/

// XXX Track the handles instead of the objects.

static void
track_thread_handle_for_fork(struct _PyThread_handle_data *data)
{
    HEAD_LOCK(&_PyRuntime);
    llist_insert_tail(&_PyRuntime.threads.handles, &data->fork_node);
    HEAD_UNLOCK(&_PyRuntime);
}

static void
untrack_thread_handle_for_fork(struct _PyThread_handle_data *data)
{
    HEAD_LOCK(&_PyRuntime);
    if (data->fork_node.next != NULL) {
        llist_remove(&data->fork_node);
    }
    HEAD_UNLOCK(&_PyRuntime);
}

static void
clear_tracked_thread_handles(struct _pythread_runtime_state *state,
                             PyThread_ident_t current)
{
    struct llist_node *node;
    llist_for_each_safe(node, &state->handles) {
        struct _PyThread_handle_data *data = llist_data(
            node, struct _PyThread_handle_data, fork_node);
        if (data->ident == current) {
            continue;
        }

        // Disallow calls to join() as they could crash. We are the only
        // thread; it's safe to set this without an atomic.
        data->state = THREAD_HANDLE_INVALID;
        llist_remove(node);
    }
}


/*************/
/* other API */
/*************/

void
_PyThreadHandle_SetStarted(PyObject *hobj,
                           PyThread_handle_t handle, PyThread_ident_t ident)
{
    _PyThread_SetStarted(((thandleobject *)hobj)->data, handle, ident);
}


_PyEventRc *
_PyThreadHandle_GetExitingEvent(PyObject *hobj)
{
    return _PyThread_GetExitingEvent(((thandleobject *)hobj)->data);
}


void
_PyThread_AfterFork(struct _pythread_runtime_state *state)
{
    // gh-115035: We mark ThreadHandles as not joinable early in the child's
    // after-fork handler. We do this before calling any Python code to ensure
    // that it happens before any ThreadHandles are deallocated, such as by a
    // GC cycle.
    PyThread_ident_t current = PyThread_get_thread_ident_ex();
    clear_tracked_thread_handles(state, current);
}
