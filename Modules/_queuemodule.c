#include "Python.h"
#include "structmember.h" /* offsetof */
#include "pythread.h"

/*[clinic input]
module _queue
class _queue.SimpleQueue "simplequeueobject *" "&PySimpleQueueType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=cf49af81bcbbbea6]*/

extern PyTypeObject PySimpleQueueType;  /* forward decl */

static PyObject *EmptyError;


typedef struct {
    PyObject_HEAD
    PyThread_type_lock lock;
    int locked;
    PyObject *lst;
    Py_ssize_t lst_pos;
    PyObject *weakreflist;
} simplequeueobject;


static void
simplequeue_dealloc(simplequeueobject *self)
{
    _PyObject_GC_UNTRACK(self);
    if (self->lock != NULL) {
        /* Unlock the lock so it's safe to free it */
        if (self->locked > 0)
            PyThread_release_lock(self->lock);
        PyThread_free_lock(self->lock);
    }
    Py_XDECREF(self->lst);
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_TYPE(self)->tp_free(self);
}

static int
simplequeue_traverse(simplequeueobject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->lst);
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
        self->lst = PyList_New(0);
        self->lock = PyThread_allocate_lock();
        self->lst_pos = 0;
        if (self->lock == NULL) {
            Py_DECREF(self);
            PyErr_SetString(PyExc_MemoryError, "can't allocate lock");
            return NULL;
        }
        if (self->lst == NULL) {
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
    if (PyList_Append(self->lst, item) < 0)
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

static PyObject *
simplequeue_pop_item(simplequeueobject *self)
{
    Py_ssize_t count, n;
    PyObject *item;

    n = PyList_GET_SIZE(self->lst);
    assert(self->lst_pos < n);

    item = PyList_GET_ITEM(self->lst, self->lst_pos);
    Py_INCREF(Py_None);
    PyList_SET_ITEM(self->lst, self->lst_pos, Py_None);
    self->lst_pos += 1;
    count = n - self->lst_pos;
    if (self->lst_pos > count) {
        /* The list is more than 50% empty, reclaim space at the beginning */
        if (PyList_SetSlice(self->lst, 0, self->lst_pos, NULL)) {
            /* Undo pop */
            self->lst_pos -= 1;
            PyList_SET_ITEM(self->lst, self->lst_pos, item);
            return NULL;
        }
        self->lst_pos = 0;
    }
    return item;
}

/*[clinic input]
_queue.SimpleQueue.get
    block: bool = True
    timeout: object = None

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
_queue_SimpleQueue_get_impl(simplequeueobject *self, int block,
                            PyObject *timeout)
/*[clinic end generated code: output=ec82a7157dcccd1a input=4bf691f9f01fa297]*/
{
    _PyTime_t endtime = 0;
    _PyTime_t timeout_val;
    PyObject *item;
    PyLockStatus r;
    PY_TIMEOUT_T microseconds;

    if (block == 0) {
        /* Non-blocking */
        microseconds = 0;
    }
    else if (timeout != Py_None) {
        /* With timeout */
        if (_PyTime_FromSecondsObject(&timeout_val,
                                      timeout, _PyTime_ROUND_CEILING) < 0)
            return NULL;
        if (timeout_val < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "'timeout' must be a non-negative number");
            return NULL;
        }
        microseconds = _PyTime_AsMicroseconds(timeout_val,
                                              _PyTime_ROUND_CEILING);
        if (microseconds >= PY_TIMEOUT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "timeout value is too large");
            return NULL;
        }
        endtime = _PyTime_GetMonotonicClock() + timeout_val;
    }
    else {
        /* Infinitely blocking */
        microseconds = -1;
    }

    /* put() signals the queue to be non-empty by releasing the lock.
     * So we simply try to acquire the lock in a loop, until the condition
     * (queue non-empty) becomes true.
     */
    while (self->lst_pos == PyList_GET_SIZE(self->lst)) {
        /* First a simple non-blocking try without releasing the GIL */
        r = PyThread_acquire_lock_timed(self->lock, 0, 0);
        if (r == PY_LOCK_FAILURE && microseconds != 0) {
            Py_BEGIN_ALLOW_THREADS
            r = PyThread_acquire_lock_timed(self->lock, microseconds, 1);
            Py_END_ALLOW_THREADS
        }
        if (r == PY_LOCK_INTR && Py_MakePendingCalls() < 0) {
            return NULL;
        }
        if (r == PY_LOCK_FAILURE) {
            /* Timed out */
            PyErr_SetNone(EmptyError);
            return NULL;
        }
        self->locked = 1;
        /* Adjust timeout for next iteration (if any) */
        if (endtime > 0) {
            timeout_val = endtime - _PyTime_GetMonotonicClock();
            microseconds = _PyTime_AsMicroseconds(timeout_val, _PyTime_ROUND_CEILING);
        }
    }
    /* BEGIN GIL-protected critical section */
    assert(self->lst_pos < PyList_GET_SIZE(self->lst));
    item = simplequeue_pop_item(self);
    if (self->locked) {
        PyThread_release_lock(self->lock);
        self->locked = 0;
    }
    /* END GIL-protected critical section */

    return item;
}

/*[clinic input]
_queue.SimpleQueue.get_nowait

Remove and return an item from the queue without blocking.

Only get an item if one is immediately available. Otherwise
raise the Empty exception.
[clinic start generated code]*/

static PyObject *
_queue_SimpleQueue_get_nowait_impl(simplequeueobject *self)
/*[clinic end generated code: output=a89731a75dbe4937 input=6fe5102db540a1b9]*/
{
    return _queue_SimpleQueue_get_impl(self, 0, Py_None);
}

/*[clinic input]
_queue.SimpleQueue.empty -> bool

Return True if the queue is empty, False otherwise (not reliable!).
[clinic start generated code]*/

static int
_queue_SimpleQueue_empty_impl(simplequeueobject *self)
/*[clinic end generated code: output=1a02a1b87c0ef838 input=1a98431c45fd66f9]*/
{
    return self->lst_pos == PyList_GET_SIZE(self->lst);
}

/*[clinic input]
_queue.SimpleQueue.qsize -> Py_ssize_t

Return the approximate size of the queue (not reliable!).
[clinic start generated code]*/

static Py_ssize_t
_queue_SimpleQueue_qsize_impl(simplequeueobject *self)
/*[clinic end generated code: output=f9dcd9d0a90e121e input=7a74852b407868a1]*/
{
    return PyList_GET_SIZE(self->lst) - self->lst_pos;
}


#include "clinic/_queuemodule.c.h"


static PyMethodDef simplequeue_methods[] = {
    _QUEUE_SIMPLEQUEUE_EMPTY_METHODDEF
    _QUEUE_SIMPLEQUEUE_GET_METHODDEF
    _QUEUE_SIMPLEQUEUE_GET_NOWAIT_METHODDEF
    _QUEUE_SIMPLEQUEUE_PUT_METHODDEF
    _QUEUE_SIMPLEQUEUE_PUT_NOWAIT_METHODDEF
    _QUEUE_SIMPLEQUEUE_QSIZE_METHODDEF
    {NULL,           NULL}              /* sentinel */
};


PyTypeObject PySimpleQueueType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_queue.SimpleQueue",               /*tp_name*/
    sizeof(simplequeueobject),          /*tp_size*/
    0,                                  /*tp_itemsize*/
    /* methods */
    (destructor)simplequeue_dealloc,    /*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_reserved*/
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_HAVE_GC,           /* tp_flags */
    simplequeue_new__doc__,             /*tp_doc*/
    (traverseproc)simplequeue_traverse, /*tp_traverse*/
    0,                                  /*tp_clear*/
    0,                                  /*tp_richcompare*/
    offsetof(simplequeueobject, weakreflist), /*tp_weaklistoffset*/
    0,                                  /*tp_iter*/
    0,                                  /*tp_iternext*/
    simplequeue_methods,                /*tp_methods*/
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    simplequeue_new                     /* tp_new */
};


/* Initialization function */

PyDoc_STRVAR(queue_module_doc,
"C implementation of the Python queue module.\n\
This module is an implementation detail, please do not use it directly.");

static struct PyModuleDef queuemodule = {
    PyModuleDef_HEAD_INIT,
    "_queue",
    queue_module_doc,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit__queue(void)
{
    PyObject *m;

    /* Create the module */
    m = PyModule_Create(&queuemodule);
    if (m == NULL)
        return NULL;

    EmptyError = PyErr_NewExceptionWithDoc(
        "_queue.Empty",
        "Exception raised by Queue.get(block=0)/get_nowait().",
        NULL, NULL);
    if (EmptyError == NULL)
        return NULL;

    Py_INCREF(EmptyError);
    if (PyModule_AddObject(m, "Empty", EmptyError) < 0)
        return NULL;

    if (PyType_Ready(&PySimpleQueueType) < 0)
        return NULL;
    Py_INCREF(&PySimpleQueueType);
    if (PyModule_AddObject(m, "SimpleQueue", (PyObject *)&PySimpleQueueType) < 0)
        return NULL;

    return m;
}
