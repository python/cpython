#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_MakePendingCalls()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_time.h"          // _PyTime_t

#include <stdbool.h>
#include <stddef.h>               // offsetof()

typedef struct {
    PyTypeObject *SimpleQueueType;
    PyObject *EmptyError;
} simplequeue_state;

static simplequeue_state *
simplequeue_get_state(PyObject *module)
{
    simplequeue_state *state = _PyModule_GetState(module);
    assert(state);
    return state;
}
static struct PyModuleDef queuemodule;
#define simplequeue_get_state_by_type(type) \
    (simplequeue_get_state(PyType_GetModuleByDef(type, &queuemodule)))

static const Py_ssize_t INITIAL_RING_BUF_CAPACITY = 8;

typedef struct {
    // Where to place the next item
    Py_ssize_t put_idx;

    // Where to get the next item
    Py_ssize_t get_idx;

    PyObject **items;

    // Total number of items that may be stored
    Py_ssize_t items_cap;

    // Number of items stored
    Py_ssize_t num_items;
} RingBuf;

static int
RingBuf_Init(RingBuf *buf)
{
    buf->put_idx = 0;
    buf->get_idx = 0;
    buf->items_cap = INITIAL_RING_BUF_CAPACITY;
    buf->num_items = 0;
    buf->items = PyMem_Calloc(buf->items_cap, sizeof(PyObject *));
    if (buf->items == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

static PyObject *
RingBuf_At(RingBuf *buf, Py_ssize_t idx)
{
    assert(idx >= 0 && idx < buf->num_items);
    return buf->items[(buf->get_idx + idx) % buf->items_cap];
}

static void
RingBuf_Fini(RingBuf *buf)
{
    PyObject **items = buf->items;
    Py_ssize_t num_items = buf->num_items;
    Py_ssize_t cap = buf->items_cap;
    Py_ssize_t idx = buf->get_idx;
    buf->items = NULL;
    buf->put_idx = 0;
    buf->get_idx = 0;
    buf->num_items = 0;
    buf->items_cap = 0;
    for (Py_ssize_t n = num_items; n > 0; idx = (idx + 1) % cap, n--) {
        Py_DECREF(items[idx]);
    }
    PyMem_Free(items);
}

// Resize the underlying items array of buf to the new capacity and arrange
// the items contiguously in the new items array.
//
// Returns -1 on allocation failure or 0 on success.
static int
resize_ringbuf(RingBuf *buf, Py_ssize_t capacity)
{
    Py_ssize_t new_capacity = Py_MAX(INITIAL_RING_BUF_CAPACITY, capacity);
    if (new_capacity == buf->items_cap) {
        return 0;
    }
    assert(buf->num_items <= new_capacity);

    PyObject **new_items = PyMem_Calloc(new_capacity, sizeof(PyObject *));
    if (new_items == NULL) {
        return -1;
    }

    // Copy the "tail" of the old items array. This corresponds to "head" of
    // the abstract ring buffer.
    Py_ssize_t tail_size =
        Py_MIN(buf->num_items, buf->items_cap - buf->get_idx);
    if (tail_size > 0) {
        memcpy(new_items, buf->items + buf->get_idx,
               tail_size * sizeof(PyObject *));
    }

    // Copy the "head" of the old items array, if any. This corresponds to the
    // "tail" of the abstract ring buffer.
    Py_ssize_t head_size = buf->num_items - tail_size;
    if (head_size > 0) {
        memcpy(new_items + tail_size, buf->items,
               head_size * sizeof(PyObject *));
    }

    PyMem_Free(buf->items);
    buf->items = new_items;
    buf->items_cap = new_capacity;
    buf->get_idx = 0;
    buf->put_idx = buf->num_items;

    return 0;
}

// Returns a strong reference from the head of the buffer.
static PyObject *
RingBuf_Get(RingBuf *buf)
{
    assert(buf->num_items > 0);

    if (buf->num_items < (buf->items_cap / 4)) {
        // Items is less than 25% occupied, shrink it by 50%. This allows for
        // growth without immediately needing to resize the underlying items
        // array.
        //
        // It's safe it ignore allocation failures here; shrinking is an
        // optimization that isn't required for correctness.
        (void)resize_ringbuf(buf, buf->items_cap / 2);
    }

    PyObject *item = buf->items[buf->get_idx];
    buf->items[buf->get_idx] = NULL;
    buf->get_idx = (buf->get_idx + 1) % buf->items_cap;
    buf->num_items--;
    return item;
}

// Returns 0 on success or -1 if the buffer failed to grow
static int
RingBuf_Put(RingBuf *buf, PyObject *item)
{
    assert(buf->num_items <= buf->items_cap);

    if (buf->num_items == buf->items_cap) {
        // Buffer is full, grow it.
        if (resize_ringbuf(buf, buf->items_cap * 2) < 0) {
            PyErr_NoMemory();
            return -1;
        }
    }
    buf->items[buf->put_idx] = Py_NewRef(item);
    buf->put_idx = (buf->put_idx + 1) % buf->items_cap;
    buf->num_items++;
    return 0;
}

static Py_ssize_t
RingBuf_Len(RingBuf *buf)
{
    return buf->num_items;
}

static bool
RingBuf_IsEmpty(RingBuf *buf)
{
    return buf->num_items == 0;
}

typedef struct {
    PyObject_HEAD
    PyThread_type_lock lock;
    int locked;
    RingBuf buf;
    PyObject *weakreflist;
} simplequeueobject;

/*[clinic input]
module _queue
class _queue.SimpleQueue "simplequeueobject *" "simplequeue_get_state_by_type(type)->SimpleQueueType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=0a4023fe4d198c8d]*/

static int
simplequeue_clear(simplequeueobject *self)
{
    RingBuf_Fini(&self->buf);
    return 0;
}

static void
simplequeue_dealloc(simplequeueobject *self)
{
    PyTypeObject *tp = Py_TYPE(self);

    PyObject_GC_UnTrack(self);
    if (self->lock != NULL) {
        /* Unlock the lock so it's safe to free it */
        if (self->locked > 0)
            PyThread_release_lock(self->lock);
        PyThread_free_lock(self->lock);
    }
    (void)simplequeue_clear(self);
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static int
simplequeue_traverse(simplequeueobject *self, visitproc visit, void *arg)
{
    RingBuf *buf = &self->buf;
    for (Py_ssize_t i = 0, num_items = buf->num_items; i < num_items; i++) {
        Py_VISIT(RingBuf_At(buf, i));
    }
    Py_VISIT(Py_TYPE(self));
    return 0;
}

/*[clinic input]
@classmethod
_queue.SimpleQueue.__new__ as simplequeue_new

Simple, unbounded, reentrant FIFO queue.
[clinic start generated code]*/

static PyObject *
simplequeue_new_impl(PyTypeObject *type)
/*[clinic end generated code: output=ba97740608ba31cd input=a0674a1643e3e2fb]*/
{
    simplequeueobject *self;

    self = (simplequeueobject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->weakreflist = NULL;
        self->lock = PyThread_allocate_lock();
        if (self->lock == NULL) {
            Py_DECREF(self);
            PyErr_SetString(PyExc_MemoryError, "can't allocate lock");
            return NULL;
        }
        if (RingBuf_Init(&self->buf) < 0) {
            Py_DECREF(self);
            return NULL;
        }
    }

    return (PyObject *) self;
}

/*[clinic input]
_queue.SimpleQueue.put
    item: object
    block: bool = True
    timeout: object = None

Put the item on the queue.

The optional 'block' and 'timeout' arguments are ignored, as this method
never blocks.  They are provided for compatibility with the Queue class.

[clinic start generated code]*/

static PyObject *
_queue_SimpleQueue_put_impl(simplequeueobject *self, PyObject *item,
                            int block, PyObject *timeout)
/*[clinic end generated code: output=4333136e88f90d8b input=6e601fa707a782d5]*/
{
    /* BEGIN GIL-protected critical section */
    if (RingBuf_Put(&self->buf, item) < 0)
        return NULL;
    if (self->locked) {
        /* A get() may be waiting, wake it up */
        self->locked = 0;
        PyThread_release_lock(self->lock);
    }
    /* END GIL-protected critical section */
    Py_RETURN_NONE;
}

/*[clinic input]
_queue.SimpleQueue.put_nowait
    item: object

Put an item into the queue without blocking.

This is exactly equivalent to `put(item)` and is only provided
for compatibility with the Queue class.

[clinic start generated code]*/

static PyObject *
_queue_SimpleQueue_put_nowait_impl(simplequeueobject *self, PyObject *item)
/*[clinic end generated code: output=0990536715efb1f1 input=36b1ea96756b2ece]*/
{
    return _queue_SimpleQueue_put_impl(self, item, 0, Py_None);
}

/*[clinic input]
_queue.SimpleQueue.get

    cls: defining_class
    /
    block: bool = True
    timeout as timeout_obj: object = None

Remove and return an item from the queue.

If optional args 'block' is true and 'timeout' is None (the default),
block if necessary until an item is available. If 'timeout' is
a non-negative number, it blocks at most 'timeout' seconds and raises
the Empty exception if no item was available within that time.
Otherwise ('block' is false), return an item if one is immediately
available, else raise the Empty exception ('timeout' is ignored
in that case).

[clinic start generated code]*/

static PyObject *
_queue_SimpleQueue_get_impl(simplequeueobject *self, PyTypeObject *cls,
                            int block, PyObject *timeout_obj)
/*[clinic end generated code: output=5c2cca914cd1e55b input=5b4047bfbc645ec1]*/
{
    _PyTime_t endtime = 0;
    _PyTime_t timeout;
    PyObject *item;
    PyLockStatus r;
    PY_TIMEOUT_T microseconds;
    PyThreadState *tstate = PyThreadState_Get();

    // XXX Use PyThread_ParseTimeoutArg().

    if (block == 0) {
        /* Non-blocking */
        microseconds = 0;
    }
    else if (timeout_obj != Py_None) {
        /* With timeout */
        if (_PyTime_FromSecondsObject(&timeout,
                                      timeout_obj, _PyTime_ROUND_CEILING) < 0) {
            return NULL;
        }
        if (timeout < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "'timeout' must be a non-negative number");
            return NULL;
        }
        microseconds = _PyTime_AsMicroseconds(timeout,
                                              _PyTime_ROUND_CEILING);
        if (microseconds > PY_TIMEOUT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "timeout value is too large");
            return NULL;
        }
        endtime = _PyDeadline_Init(timeout);
    }
    else {
        /* Infinitely blocking */
        microseconds = -1;
    }

    /* put() signals the queue to be non-empty by releasing the lock.
     * So we simply try to acquire the lock in a loop, until the condition
     * (queue non-empty) becomes true.
     */
    while (RingBuf_IsEmpty(&self->buf)) {
        /* First a simple non-blocking try without releasing the GIL */
        r = PyThread_acquire_lock_timed(self->lock, 0, 0);
        if (r == PY_LOCK_FAILURE && microseconds != 0) {
            Py_BEGIN_ALLOW_THREADS
            r = PyThread_acquire_lock_timed(self->lock, microseconds, 1);
            Py_END_ALLOW_THREADS
        }

        if (r == PY_LOCK_INTR && _PyEval_MakePendingCalls(tstate) < 0) {
            return NULL;
        }
        if (r == PY_LOCK_FAILURE) {
            PyObject *module = PyType_GetModule(cls);
            simplequeue_state *state = simplequeue_get_state(module);
            /* Timed out */
            PyErr_SetNone(state->EmptyError);
            return NULL;
        }
        self->locked = 1;

        /* Adjust timeout for next iteration (if any) */
        if (microseconds > 0) {
            timeout = _PyDeadline_Get(endtime);
            microseconds = _PyTime_AsMicroseconds(timeout,
                                                  _PyTime_ROUND_CEILING);
        }
    }

    /* BEGIN GIL-protected critical section */
    item = RingBuf_Get(&self->buf);
    if (self->locked) {
        PyThread_release_lock(self->lock);
        self->locked = 0;
    }
    /* END GIL-protected critical section */

    return item;
}

/*[clinic input]
_queue.SimpleQueue.get_nowait

    cls: defining_class
    /

Remove and return an item from the queue without blocking.

Only get an item if one is immediately available. Otherwise
raise the Empty exception.
[clinic start generated code]*/

static PyObject *
_queue_SimpleQueue_get_nowait_impl(simplequeueobject *self,
                                   PyTypeObject *cls)
/*[clinic end generated code: output=620c58e2750f8b8a input=842f732bf04216d3]*/
{
    return _queue_SimpleQueue_get_impl(self, cls, 0, Py_None);
}

/*[clinic input]
_queue.SimpleQueue.empty -> bool

Return True if the queue is empty, False otherwise (not reliable!).
[clinic start generated code]*/

static int
_queue_SimpleQueue_empty_impl(simplequeueobject *self)
/*[clinic end generated code: output=1a02a1b87c0ef838 input=1a98431c45fd66f9]*/
{
    return RingBuf_IsEmpty(&self->buf);
}

/*[clinic input]
_queue.SimpleQueue.qsize -> Py_ssize_t

Return the approximate size of the queue (not reliable!).
[clinic start generated code]*/

static Py_ssize_t
_queue_SimpleQueue_qsize_impl(simplequeueobject *self)
/*[clinic end generated code: output=f9dcd9d0a90e121e input=7a74852b407868a1]*/
{
    return RingBuf_Len(&self->buf);
}

static int
queue_traverse(PyObject *m, visitproc visit, void *arg)
{
    simplequeue_state *state = simplequeue_get_state(m);
    Py_VISIT(state->SimpleQueueType);
    Py_VISIT(state->EmptyError);
    return 0;
}

static int
queue_clear(PyObject *m)
{
    simplequeue_state *state = simplequeue_get_state(m);
    Py_CLEAR(state->SimpleQueueType);
    Py_CLEAR(state->EmptyError);
    return 0;
}

static void
queue_free(void *m)
{
    queue_clear((PyObject *)m);
}

#include "clinic/_queuemodule.c.h"


static PyMethodDef simplequeue_methods[] = {
    _QUEUE_SIMPLEQUEUE_EMPTY_METHODDEF
    _QUEUE_SIMPLEQUEUE_GET_METHODDEF
    _QUEUE_SIMPLEQUEUE_GET_NOWAIT_METHODDEF
    _QUEUE_SIMPLEQUEUE_PUT_METHODDEF
    _QUEUE_SIMPLEQUEUE_PUT_NOWAIT_METHODDEF
    _QUEUE_SIMPLEQUEUE_QSIZE_METHODDEF
    {"__class_getitem__",    Py_GenericAlias,
    METH_O|METH_CLASS,       PyDoc_STR("See PEP 585")},
    {NULL,           NULL}              /* sentinel */
};

static struct PyMemberDef simplequeue_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(simplequeueobject, weakreflist), Py_READONLY},
    {NULL},
};

static PyType_Slot simplequeue_slots[] = {
    {Py_tp_dealloc, simplequeue_dealloc},
    {Py_tp_doc, (void *)simplequeue_new__doc__},
    {Py_tp_traverse, simplequeue_traverse},
    {Py_tp_clear, simplequeue_clear},
    {Py_tp_members, simplequeue_members},
    {Py_tp_methods, simplequeue_methods},
    {Py_tp_new, simplequeue_new},
    {0, NULL},
};

static PyType_Spec simplequeue_spec = {
    .name = "_queue.SimpleQueue",
    .basicsize = sizeof(simplequeueobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = simplequeue_slots,
};


/* Initialization function */

PyDoc_STRVAR(queue_module_doc,
"C implementation of the Python queue module.\n\
This module is an implementation detail, please do not use it directly.");

static int
queuemodule_exec(PyObject *module)
{
    simplequeue_state *state = simplequeue_get_state(module);

    state->EmptyError = PyErr_NewExceptionWithDoc(
        "_queue.Empty",
        "Exception raised by Queue.get(block=0)/get_nowait().",
        NULL, NULL);
    if (state->EmptyError == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "Empty", state->EmptyError) < 0) {
        return -1;
    }

    state->SimpleQueueType = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &simplequeue_spec, NULL);
    if (state->SimpleQueueType == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->SimpleQueueType) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot queuemodule_slots[] = {
    {Py_mod_exec, queuemodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};


static struct PyModuleDef queuemodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_queue",
    .m_doc = queue_module_doc,
    .m_size = sizeof(simplequeue_state),
    .m_slots = queuemodule_slots,
    .m_traverse = queue_traverse,
    .m_clear = queue_clear,
    .m_free = queue_free,
};


PyMODINIT_FUNC
PyInit__queue(void)
{
   return PyModuleDef_Init(&queuemodule);
}
