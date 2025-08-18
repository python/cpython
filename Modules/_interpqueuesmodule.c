/* interpreters module */
/* low-level access to interpreter primitives */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_crossinterp.h"   // _PyXIData_t

#define REGISTERS_HEAP_TYPES
#define HAS_FALLBACK
#define HAS_UNBOUND_ITEMS
#include "_interpreters_common.h"
#undef HAS_UNBOUND_ITEMS
#undef HAS_FALLBACK
#undef REGISTERS_HEAP_TYPES


#define MODULE_NAME _interpqueues
#define MODULE_NAME_STR Py_STRINGIFY(MODULE_NAME)
#define MODINIT_FUNC_NAME RESOLVE_MODINIT_FUNC_NAME(MODULE_NAME)


#define GLOBAL_MALLOC(TYPE) \
    PyMem_RawMalloc(sizeof(TYPE))
#define GLOBAL_FREE(VAR) \
    PyMem_RawFree(VAR)


#define XID_IGNORE_EXC 1
#define XID_FREE 2

static int
_release_xid_data(_PyXIData_t *data, int flags)
{
    int ignoreexc = flags & XID_IGNORE_EXC;
    PyObject *exc;
    if (ignoreexc) {
        exc = PyErr_GetRaisedException();
    }
    int res;
    if (flags & XID_FREE) {
        res = _PyXIData_ReleaseAndRawFree(data);
    }
    else {
        res = _PyXIData_Release(data);
    }
    if (res < 0) {
        /* The owning interpreter is already destroyed. */
        if (ignoreexc) {
            // XXX Emit a warning?
            PyErr_Clear();
        }
    }
    if (flags & XID_FREE) {
        /* Either way, we free the data. */
    }
    if (ignoreexc) {
        PyErr_SetRaisedException(exc);
    }
    return res;
}

static PyInterpreterState *
_get_current_interp(void)
{
    // PyInterpreterState_Get() aborts if lookup fails, so don't need
    // to check the result for NULL.
    return PyInterpreterState_Get();
}

static PyObject *
_get_current_module(void)
{
    PyObject *name = PyUnicode_FromString(MODULE_NAME_STR);
    if (name == NULL) {
        return NULL;
    }
    PyObject *mod = PyImport_GetModule(name);
    Py_DECREF(name);
    if (mod == NULL) {
        return NULL;
    }
    assert(mod != Py_None);
    return mod;
}


struct idarg_int64_converter_data {
    // input:
    const char *label;
    // output:
    int64_t id;
};

static int
idarg_int64_converter(PyObject *arg, void *ptr)
{
    int64_t id;
    struct idarg_int64_converter_data *data = ptr;

    const char *label = data->label;
    if (label == NULL) {
        label = "ID";
    }

    if (PyIndex_Check(arg)) {
        int overflow = 0;
        id = PyLong_AsLongLongAndOverflow(arg, &overflow);
        if (id == -1 && PyErr_Occurred()) {
            return 0;
        }
        else if (id == -1 && overflow == 1) {
            PyErr_Format(PyExc_OverflowError,
                         "max %s is %lld, got %R", label, INT64_MAX, arg);
            return 0;
        }
        else if (id < 0) {
            PyErr_Format(PyExc_ValueError,
                         "%s must be a non-negative int, got %R", label, arg);
            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "%s must be an int, got %.100s",
                     label, Py_TYPE(arg)->tp_name);
        return 0;
    }
    data->id = id;
    return 1;
}


static int
ensure_highlevel_module_loaded(void)
{
    PyObject *highlevel =
            PyImport_ImportModule("concurrent.interpreters._queues");
    if (highlevel == NULL) {
        return -1;
    }
    Py_DECREF(highlevel);
    return 0;
}


/* module state *************************************************************/

typedef struct {
    /* external types (added at runtime by interpreters module) */
    PyTypeObject *queue_type;

    /* QueueError (and its subclasses) */
    PyObject *QueueError;
    PyObject *QueueNotFoundError;
    PyObject *QueueEmpty;
    PyObject *QueueFull;
} module_state;

static inline module_state *
get_module_state(PyObject *mod)
{
    assert(mod != NULL);
    module_state *state = PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static int
traverse_module_state(module_state *state, visitproc visit, void *arg)
{
    /* external types */
    Py_VISIT(state->queue_type);

    /* QueueError */
    Py_VISIT(state->QueueError);
    Py_VISIT(state->QueueNotFoundError);
    Py_VISIT(state->QueueEmpty);
    Py_VISIT(state->QueueFull);

    return 0;
}

static int
clear_module_state(module_state *state)
{
    /* external types */
    if (state->queue_type != NULL) {
        (void)clear_xid_class(state->queue_type);
    }
    Py_CLEAR(state->queue_type);

    /* QueueError */
    Py_CLEAR(state->QueueError);
    Py_CLEAR(state->QueueNotFoundError);
    Py_CLEAR(state->QueueEmpty);
    Py_CLEAR(state->QueueFull);

    return 0;
}


/* error codes **************************************************************/

#define ERR_EXCEPTION_RAISED (-1)
// multi-queue errors
#define ERR_QUEUES_ALLOC (-11)
#define ERR_QUEUE_ALLOC (-12)
#define ERR_NO_NEXT_QUEUE_ID (-13)
#define ERR_QUEUE_NOT_FOUND (-14)
// single-queue errors
#define ERR_QUEUE_EMPTY (-21)
#define ERR_QUEUE_FULL (-22)
#define ERR_QUEUE_NEVER_BOUND (-23)

static int ensure_external_exc_types(module_state *);

static int
resolve_module_errcode(module_state *state, int errcode, int64_t qid,
                       PyObject **p_exctype, PyObject **p_msgobj)
{
    PyObject *exctype = NULL;
    PyObject *msg = NULL;
    switch (errcode) {
    case ERR_NO_NEXT_QUEUE_ID:
        exctype = state->QueueError;
        msg = PyUnicode_FromString("ran out of queue IDs");
        break;
    case ERR_QUEUE_NOT_FOUND:
        exctype = state->QueueNotFoundError;
        msg = PyUnicode_FromFormat("queue %" PRId64 " not found", qid);
        break;
    case ERR_QUEUE_EMPTY:
        if (ensure_external_exc_types(state) < 0) {
            return -1;
        }
        exctype = state->QueueEmpty;
        msg = PyUnicode_FromFormat("queue %" PRId64 " is empty", qid);
        break;
    case ERR_QUEUE_FULL:
        if (ensure_external_exc_types(state) < 0) {
            return -1;
        }
        exctype = state->QueueFull;
        msg = PyUnicode_FromFormat("queue %" PRId64 " is full", qid);
        break;
    case ERR_QUEUE_NEVER_BOUND:
        exctype = state->QueueError;
        msg = PyUnicode_FromFormat("queue %" PRId64 " never bound", qid);
        break;
    default:
        PyErr_Format(PyExc_ValueError,
                     "unsupported error code %d", errcode);
        return -1;
    }

    if (msg == NULL) {
        assert(PyErr_Occurred());
        return -1;
    }
    *p_exctype = exctype;
    *p_msgobj = msg;
    return 0;
}


/* QueueError ***************************************************************/

static int
add_exctype(PyObject *mod, PyObject **p_state_field,
            const char *qualname, const char *doc, PyObject *base)
{
#ifndef NDEBUG
    const char *dot = strrchr(qualname, '.');
    assert(dot != NULL);
    const char *name = dot+1;
    assert(*p_state_field == NULL);
    assert(!PyObject_HasAttrStringWithError(mod, name));
#endif
    PyObject *exctype = PyErr_NewExceptionWithDoc(qualname, doc, base, NULL);
    if (exctype == NULL) {
        return -1;
    }
    if (PyModule_AddType(mod, (PyTypeObject *)exctype) < 0) {
        Py_DECREF(exctype);
        return -1;
    }
    *p_state_field = exctype;
    return 0;
}

static int
add_QueueError(PyObject *mod)
{
    module_state *state = get_module_state(mod);

#define PREFIX "concurrent.interpreters."
#define ADD_EXCTYPE(NAME, BASE, DOC)                                    \
    assert(state->NAME == NULL);                                        \
    if (add_exctype(mod, &state->NAME, PREFIX #NAME, DOC, BASE) < 0) {  \
        return -1;                                                      \
    }
    ADD_EXCTYPE(QueueError, PyExc_RuntimeError,
                "Indicates that a queue-related error happened.")
    ADD_EXCTYPE(QueueNotFoundError, state->QueueError, NULL)
    // QueueEmpty and QueueFull are set by set_external_exc_types().
    state->QueueEmpty = NULL;
    state->QueueFull = NULL;
#undef ADD_EXCTYPE
#undef PREFIX

    return 0;
}

static int
set_external_exc_types(module_state *state,
                       PyObject *emptyerror, PyObject *fullerror)
{
    if (state->QueueEmpty != NULL) {
        assert(state->QueueFull != NULL);
        Py_CLEAR(state->QueueEmpty);
        Py_CLEAR(state->QueueFull);
    }
    else {
        assert(state->QueueFull == NULL);
    }
    assert(PyObject_IsSubclass(emptyerror, state->QueueError));
    assert(PyObject_IsSubclass(fullerror, state->QueueError));
    state->QueueEmpty = Py_NewRef(emptyerror);
    state->QueueFull = Py_NewRef(fullerror);
    return 0;
}

static int
ensure_external_exc_types(module_state *state)
{
    if (state->QueueEmpty != NULL) {
        assert(state->QueueFull != NULL);
        return 0;
    }
    assert(state->QueueFull == NULL);

    // Force the module to be loaded, to register the type.
    if (ensure_highlevel_module_loaded() < 0) {
        return -1;
    }
    assert(state->QueueEmpty != NULL);
    assert(state->QueueFull != NULL);
    return 0;
}

static int
handle_queue_error(int err, PyObject *mod, int64_t qid)
{
    if (err == 0) {
        assert(!PyErr_Occurred());
        return 0;
    }
    assert(err < 0);
    assert((err == -1) == (PyErr_Occurred() != NULL));

    module_state *state;
    switch (err) {
    case ERR_QUEUE_ALLOC: _Py_FALLTHROUGH;
    case ERR_QUEUES_ALLOC:
        PyErr_NoMemory();
        break;
    case -1:
        return -1;
    default:
        state = get_module_state(mod);
        assert(state->QueueError != NULL);
        PyObject *exctype = NULL;
        PyObject *msg = NULL;
        if (resolve_module_errcode(state, err, qid, &exctype, &msg) < 0) {
            return -1;
        }
        PyObject *exc = PyObject_CallOneArg(exctype, msg);
        Py_DECREF(msg);
        if (exc == NULL) {
            return -1;
        }
        PyErr_SetObject(exctype, exc);
        Py_DECREF(exc);
    }
    return 1;
}


/* the basic queue **********************************************************/

struct _queueitem;

typedef struct _queueitem {
    /* The interpreter that added the item to the queue.
       The actual bound interpid is found in item->data.
       This is necessary because item->data might be NULL,
       meaning the interpreter has been destroyed. */
    int64_t interpid;
    _PyXIData_t *data;
    unboundop_t unboundop;
    struct _queueitem *next;
} _queueitem;

static void
_queueitem_init(_queueitem *item,
                int64_t interpid, _PyXIData_t *data, unboundop_t unboundop)
{
    if (interpid < 0) {
        interpid = _get_interpid(data);
    }
    else {
        assert(data == NULL
               || _PyXIData_INTERPID(data) < 0
               || interpid == _PyXIData_INTERPID(data));
    }
    assert(check_unbound(unboundop));
    *item = (_queueitem){
        .interpid = interpid,
        .data = data,
        .unboundop = unboundop,
    };
}

static void
_queueitem_clear_data(_queueitem *item)
{
    if (item->data == NULL) {
        return;
    }
    // It was allocated in queue_put().
    (void)_release_xid_data(item->data, XID_IGNORE_EXC & XID_FREE);
    item->data = NULL;
}

static void
_queueitem_clear(_queueitem *item)
{
    item->next = NULL;
    _queueitem_clear_data(item);
}

static _queueitem *
_queueitem_new(int64_t interpid, _PyXIData_t *data, int unboundop)
{
    _queueitem *item = GLOBAL_MALLOC(_queueitem);
    if (item == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _queueitem_init(item, interpid, data, unboundop);
    return item;
}

static void
_queueitem_free(_queueitem *item)
{
    _queueitem_clear(item);
    GLOBAL_FREE(item);
}

static void
_queueitem_free_all(_queueitem *item)
{
    while (item != NULL) {
        _queueitem *last = item;
        item = item->next;
        _queueitem_free(last);
    }
}

static void
_queueitem_popped(_queueitem *item,
                  _PyXIData_t **p_data, unboundop_t *p_unboundop)
{
    *p_data = item->data;
    *p_unboundop = item->unboundop;
    // We clear them here, so they won't be released in _queueitem_clear().
    item->data = NULL;
    _queueitem_free(item);
}

static int
_queueitem_clear_interpreter(_queueitem *item)
{
    assert(item->interpid >= 0);
    if (item->data == NULL) {
        // Its interpreter was already cleared (or it was never bound).
        // For UNBOUND_REMOVE it should have been freed at that time.
        assert(item->unboundop != UNBOUND_REMOVE);
        return 0;
    }
    assert(_PyXIData_INTERPID(item->data) == item->interpid);

    switch (item->unboundop) {
    case UNBOUND_REMOVE:
        // The caller must free/clear it.
        return 1;
    case UNBOUND_ERROR:
    case UNBOUND_REPLACE:
        // We won't need the cross-interpreter data later
        // so we completely throw it away.
        _queueitem_clear_data(item);
        return 0;
    default:
        Py_FatalError("not reachable");
        return -1;
    }
}


/* the queue */

typedef struct _queue {
    Py_ssize_t num_waiters;  // protected by global lock
    PyThread_type_lock mutex;
    int alive;
    struct _queueitems {
        Py_ssize_t maxsize;
        Py_ssize_t count;
        _queueitem *first;
        _queueitem *last;
    } items;
    struct _queuedefaults {
        xidata_fallback_t fallback;
        int unboundop;
    } defaults;
} _queue;

static int
_queue_init(_queue *queue, Py_ssize_t maxsize, struct _queuedefaults defaults)
{
    assert(check_unbound(defaults.unboundop));
    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        return ERR_QUEUE_ALLOC;
    }
    *queue = (_queue){
        .mutex = mutex,
        .alive = 1,
        .items = {
            .maxsize = maxsize,
        },
        .defaults = defaults,
    };
    return 0;
}

static void
_queue_clear(_queue *queue)
{
    assert(!queue->alive);
    assert(queue->num_waiters == 0);
    _queueitem_free_all(queue->items.first);
    assert(queue->mutex != NULL);
    PyThread_free_lock(queue->mutex);
    *queue = (_queue){0};
}

static void _queue_free(_queue *);

static void
_queue_kill_and_wait(_queue *queue)
{
    // Mark it as dead.
    PyThread_acquire_lock(queue->mutex, WAIT_LOCK);
    assert(queue->alive);
    queue->alive = 0;
    PyThread_release_lock(queue->mutex);

    // Wait for all waiters to fail.
    while (queue->num_waiters > 0) {
        PyThread_acquire_lock(queue->mutex, WAIT_LOCK);
        PyThread_release_lock(queue->mutex);
    };
}

static void
_queue_mark_waiter(_queue *queue, PyThread_type_lock parent_mutex)
{
    if (parent_mutex != NULL) {
        PyThread_acquire_lock(parent_mutex, WAIT_LOCK);
        queue->num_waiters += 1;
        PyThread_release_lock(parent_mutex);
    }
    else {
        // The caller must be holding the parent lock already.
        queue->num_waiters += 1;
    }
}

static void
_queue_unmark_waiter(_queue *queue, PyThread_type_lock parent_mutex)
{
    if (parent_mutex != NULL) {
        PyThread_acquire_lock(parent_mutex, WAIT_LOCK);
        queue->num_waiters -= 1;
        PyThread_release_lock(parent_mutex);
    }
    else {
        // The caller must be holding the parent lock already.
        queue->num_waiters -= 1;
    }
}

static int
_queue_lock(_queue *queue)
{
    // The queue must be marked as a waiter already.
    PyThread_acquire_lock(queue->mutex, WAIT_LOCK);
    if (!queue->alive) {
        PyThread_release_lock(queue->mutex);
        return ERR_QUEUE_NOT_FOUND;
    }
    return 0;
}

static void
_queue_unlock(_queue *queue)
{
    PyThread_release_lock(queue->mutex);
}

static int
_queue_add(_queue *queue, int64_t interpid, _PyXIData_t *data, int unboundop)
{
    int err = _queue_lock(queue);
    if (err < 0) {
        return err;
    }

    Py_ssize_t maxsize = queue->items.maxsize;
    if (maxsize <= 0) {
        maxsize = PY_SSIZE_T_MAX;
    }
    if (queue->items.count >= maxsize) {
        _queue_unlock(queue);
        return ERR_QUEUE_FULL;
    }

    _queueitem *item = _queueitem_new(interpid, data, unboundop);
    if (item == NULL) {
        _queue_unlock(queue);
        return -1;
    }

    queue->items.count += 1;
    if (queue->items.first == NULL) {
        queue->items.first = item;
    }
    else {
        queue->items.last->next = item;
    }
    queue->items.last = item;

    _queue_unlock(queue);
    return 0;
}

static int
_queue_next(_queue *queue, _PyXIData_t **p_data, int *p_unboundop)
{
    int err = _queue_lock(queue);
    if (err < 0) {
        return err;
    }

    assert(queue->items.count >= 0);
    _queueitem *item = queue->items.first;
    if (item == NULL) {
        _queue_unlock(queue);
        return ERR_QUEUE_EMPTY;
    }
    queue->items.first = item->next;
    if (queue->items.last == item) {
        queue->items.last = NULL;
    }
    queue->items.count -= 1;

    _queueitem_popped(item, p_data, p_unboundop);

    _queue_unlock(queue);
    return 0;
}

static int
_queue_get_maxsize(_queue *queue, Py_ssize_t *p_maxsize)
{
    int err = _queue_lock(queue);
    if (err < 0) {
        return err;
    }

    *p_maxsize = queue->items.maxsize;

    _queue_unlock(queue);
    return 0;
}

static int
_queue_is_full(_queue *queue, int *p_is_full)
{
    int err = _queue_lock(queue);
    if (err < 0) {
        return err;
    }

    assert(queue->items.maxsize <= 0
           || queue->items.count <= queue->items.maxsize);
    *p_is_full = queue->items.maxsize > 0
        ? queue->items.count == queue->items.maxsize
        : 0;

    _queue_unlock(queue);
    return 0;
}

static int
_queue_get_count(_queue *queue, Py_ssize_t *p_count)
{
    int err = _queue_lock(queue);
    if (err < 0) {
        return err;
    }

    *p_count = queue->items.count;

    _queue_unlock(queue);
    return 0;
}

static void
_queue_clear_interpreter(_queue *queue, int64_t interpid)
{
    int err = _queue_lock(queue);
    if (err == ERR_QUEUE_NOT_FOUND) {
        // The queue is already destroyed, so there's nothing to clear.
        assert(!PyErr_Occurred());
        return;
    }
    assert(err == 0);  // There should be no other errors.

    _queueitem *prev = NULL;
    _queueitem *next = queue->items.first;
    while (next != NULL) {
        _queueitem *item = next;
        next = item->next;
        int remove = (item->interpid == interpid)
            ? _queueitem_clear_interpreter(item)
            : 0;
        if (remove) {
            _queueitem_free(item);
            if (prev == NULL) {
                queue->items.first = next;
            }
            else {
                prev->next = next;
            }
            queue->items.count -= 1;
        }
        else {
            prev = item;
        }
    }

    _queue_unlock(queue);
}


/* external queue references ************************************************/

struct _queueref;

typedef struct _queueref {
    struct _queueref *next;
    int64_t qid;
    Py_ssize_t refcount;
    _queue *queue;
} _queueref;

static _queueref *
_queuerefs_find(_queueref *first, int64_t qid, _queueref **pprev)
{
    _queueref *prev = NULL;
    _queueref *ref = first;
    while (ref != NULL) {
        if (ref->qid == qid) {
            break;
        }
        prev = ref;
        ref = ref->next;
    }
    if (pprev != NULL) {
        *pprev = prev;
    }
    return ref;
}

static void
_queuerefs_clear(_queueref *head)
{
    _queueref *next = head;
    while (next != NULL) {
        _queueref *ref = next;
        next = ref->next;

#ifdef Py_DEBUG
        int64_t qid = ref->qid;
        fprintf(stderr, "queue %" PRId64 " still exists\n", qid);
#endif
        _queue *queue = ref->queue;
        GLOBAL_FREE(ref);

        _queue_kill_and_wait(queue);
#ifdef Py_DEBUG
    if (queue->items.count > 0) {
        fprintf(stderr, "queue %" PRId64 " still holds %zd items\n",
                qid, queue->items.count);
    }
#endif
        _queue_free(queue);
    }
}


/* a collection of queues ***************************************************/

typedef struct _queues {
    PyThread_type_lock mutex;
    _queueref *head;
    int64_t count;
    int64_t next_id;
} _queues;

static void
_queues_init(_queues *queues, PyThread_type_lock mutex)
{
    assert(mutex != NULL);
    assert(queues->mutex == NULL);
    *queues = (_queues){
        .mutex = mutex,
        .head = NULL,
        .count = 0,
        .next_id = 1,
    };
}

static void
_queues_fini(_queues *queues, PyThread_type_lock *p_mutex)
{
    PyThread_type_lock mutex = queues->mutex;
    assert(mutex != NULL);

    PyThread_acquire_lock(mutex, WAIT_LOCK);
    if (queues->count > 0) {
        assert(queues->head != NULL);
        _queuerefs_clear(queues->head);
    }
    *queues = (_queues){0};
    PyThread_release_lock(mutex);

    *p_mutex = mutex;
}

static int64_t
_queues_next_id(_queues *queues)  // needs lock
{
    int64_t qid = queues->next_id;
    if (qid < 0) {
        /* overflow */
        return ERR_NO_NEXT_QUEUE_ID;
    }
    queues->next_id += 1;
    return qid;
}

static int
_queues_lookup(_queues *queues, int64_t qid, _queue **res)
{
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    _queueref *ref = _queuerefs_find(queues->head, qid, NULL);
    if (ref == NULL) {
        PyThread_release_lock(queues->mutex);
        return ERR_QUEUE_NOT_FOUND;
    }
    assert(ref->queue != NULL);
    _queue *queue = ref->queue;
    _queue_mark_waiter(queue, NULL);
    // The caller must unmark it.

    PyThread_release_lock(queues->mutex);

    *res = queue;
    return 0;
}

static int64_t
_queues_add(_queues *queues, _queue *queue)
{
    int64_t qid = -1;
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    // Create a new ref.
    int64_t _qid = _queues_next_id(queues);
    if (_qid < 0) {
        goto done;
    }
    _queueref *ref = GLOBAL_MALLOC(_queueref);
    if (ref == NULL) {
        qid = ERR_QUEUE_ALLOC;
        goto done;
    }
    *ref = (_queueref){
        .qid = _qid,
        .queue = queue,
    };

    // Add it to the list.
    // We assume that the queue is a new one (not already in the list).
    ref->next = queues->head;
    queues->head = ref;
    queues->count += 1;

    qid = _qid;
done:
    PyThread_release_lock(queues->mutex);
    return qid;
}

static void
_queues_remove_ref(_queues *queues, _queueref *ref, _queueref *prev,
                   _queue **p_queue)
{
    assert(ref->queue != NULL);

    if (ref == queues->head) {
        queues->head = ref->next;
    }
    else {
        prev->next = ref->next;
    }
    ref->next = NULL;
    queues->count -= 1;

    *p_queue = ref->queue;
    ref->queue = NULL;
    GLOBAL_FREE(ref);
}

static int
_queues_remove(_queues *queues, int64_t qid, _queue **p_queue)
{
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    _queueref *prev = NULL;
    _queueref *ref = _queuerefs_find(queues->head, qid, &prev);
    if (ref == NULL) {
        PyThread_release_lock(queues->mutex);
        return ERR_QUEUE_NOT_FOUND;
    }

    _queues_remove_ref(queues, ref, prev, p_queue);
    PyThread_release_lock(queues->mutex);

    return 0;
}

static int
_queues_incref(_queues *queues, int64_t qid)
{
    // XXX Track interpreter IDs?
    int res = -1;
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    _queueref *ref = _queuerefs_find(queues->head, qid, NULL);
    if (ref == NULL) {
        assert(!PyErr_Occurred());
        res = ERR_QUEUE_NOT_FOUND;
        goto done;
    }
    ref->refcount += 1;

    res = 0;
done:
    PyThread_release_lock(queues->mutex);
    return res;
}

static int
_queues_decref(_queues *queues, int64_t qid)
{
    int res = -1;
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    _queueref *prev = NULL;
    _queueref *ref = _queuerefs_find(queues->head, qid, &prev);
    if (ref == NULL) {
        assert(!PyErr_Occurred());
        res = ERR_QUEUE_NOT_FOUND;
        goto finally;
    }
    if (ref->refcount == 0) {
        res = ERR_QUEUE_NEVER_BOUND;
        goto finally;
    }
    assert(ref->refcount > 0);
    ref->refcount -= 1;

    // Destroy if no longer used.
    assert(ref->queue != NULL);
    if (ref->refcount == 0) {
        _queue *queue = NULL;
        _queues_remove_ref(queues, ref, prev, &queue);
        PyThread_release_lock(queues->mutex);

        _queue_kill_and_wait(queue);
        _queue_free(queue);
        return 0;
    }

    res = 0;
finally:
    PyThread_release_lock(queues->mutex);
    return res;
}

struct queue_id_and_info {
    int64_t id;
    struct _queuedefaults defaults;
};

static struct queue_id_and_info *
_queues_list_all(_queues *queues, int64_t *p_count)
{
    struct queue_id_and_info *qids = NULL;
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);
    struct queue_id_and_info *ids = PyMem_NEW(struct queue_id_and_info,
                                              (Py_ssize_t)(queues->count));
    if (ids == NULL) {
        goto done;
    }
    _queueref *ref = queues->head;
    for (int64_t i=0; ref != NULL; ref = ref->next, i++) {
        ids[i].id = ref->qid;
        assert(ref->queue != NULL);
        ids[i].defaults = ref->queue->defaults;
    }
    *p_count = queues->count;

    qids = ids;
done:
    PyThread_release_lock(queues->mutex);
    return qids;
}

static void
_queues_clear_interpreter(_queues *queues, int64_t interpid)
{
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    _queueref *ref = queues->head;
    for (; ref != NULL; ref = ref->next) {
        assert(ref->queue != NULL);
        _queue_clear_interpreter(ref->queue, interpid);
    }

    PyThread_release_lock(queues->mutex);
}


/* "high"-level queue-related functions *************************************/

static void
_queue_free(_queue *queue)
{
    _queue_clear(queue);
    GLOBAL_FREE(queue);
}

// Create a new queue.
static int64_t
queue_create(_queues *queues, Py_ssize_t maxsize,
             struct _queuedefaults defaults)
{
    _queue *queue = GLOBAL_MALLOC(_queue);
    if (queue == NULL) {
        return ERR_QUEUE_ALLOC;
    }
    int err = _queue_init(queue, maxsize, defaults);
    if (err < 0) {
        GLOBAL_FREE(queue);
        return (int64_t)err;
    }
    int64_t qid = _queues_add(queues, queue);
    if (qid < 0) {
        _queue_clear(queue);
        GLOBAL_FREE(queue);
    }
    return qid;
}

// Completely destroy the queue.
static int
queue_destroy(_queues *queues, int64_t qid)
{
    _queue *queue = NULL;
    int err = _queues_remove(queues, qid, &queue);
    if (err < 0) {
        return err;
    }
    _queue_kill_and_wait(queue);
    _queue_free(queue);
    return 0;
}

// Push an object onto the queue.
static int
queue_put(_queues *queues, int64_t qid, PyObject *obj, unboundop_t unboundop,
          xidata_fallback_t fallback)
{
    PyThreadState *tstate = PyThreadState_Get();

    // Look up the queue.
    _queue *queue = NULL;
    int err = _queues_lookup(queues, qid, &queue);
    if (err != 0) {
        return err;
    }
    assert(queue != NULL);

    // Convert the object to cross-interpreter data.
    _PyXIData_t *xidata = _PyXIData_New();
    if (xidata == NULL) {
        _queue_unmark_waiter(queue, queues->mutex);
        return -1;
    }
    if (_PyObject_GetXIData(tstate, obj, fallback, xidata) != 0) {
        _queue_unmark_waiter(queue, queues->mutex);
        GLOBAL_FREE(xidata);
        return -1;
    }
    assert(_PyXIData_INTERPID(xidata) ==
            PyInterpreterState_GetID(tstate->interp));

    // Add the data to the queue.
    int64_t interpid = -1;  // _queueitem_init() will set it.
    int res = _queue_add(queue, interpid, xidata, unboundop);
    _queue_unmark_waiter(queue, queues->mutex);
    if (res != 0) {
        // We may chain an exception here:
        (void)_release_xid_data(xidata, 0);
        GLOBAL_FREE(xidata);
        return res;
    }

    return 0;
}

// Pop the next object off the queue.  Fail if empty.
// XXX Support a "wait" mutex?
static int
queue_get(_queues *queues, int64_t qid,
          PyObject **res, int *p_unboundop)
{
    int err;
    *res = NULL;

    // Look up the queue.
    _queue *queue = NULL;
    err = _queues_lookup(queues, qid, &queue);
    if (err != 0) {
        return err;
    }
    // Past this point we are responsible for releasing the mutex.
    assert(queue != NULL);

    // Pop off the next item from the queue.
    _PyXIData_t *data = NULL;
    err = _queue_next(queue, &data, p_unboundop);
    _queue_unmark_waiter(queue, queues->mutex);
    if (err != 0) {
        return err;
    }
    else if (data == NULL) {
        assert(!PyErr_Occurred());
        return 0;
    }

    // Convert the data back to an object.
    PyObject *obj = _PyXIData_NewObject(data);
    if (obj == NULL) {
        assert(PyErr_Occurred());
        // It was allocated in queue_put(), so we free it.
        (void)_release_xid_data(data, XID_IGNORE_EXC | XID_FREE);
        return -1;
    }
    // It was allocated in queue_put(), so we free it.
    int release_res = _release_xid_data(data, XID_FREE);
    if (release_res < 0) {
        // The source interpreter has been destroyed already.
        assert(PyErr_Occurred());
        Py_DECREF(obj);
        return -1;
    }

    *res = obj;
    return 0;
}

static int
queue_get_defaults(_queues *queues, int64_t qid,
                   struct _queuedefaults *p_defaults)
{
    _queue *queue = NULL;
    int err = _queues_lookup(queues, qid, &queue);
    if (err != 0) {
        return err;
    }
    *p_defaults = queue->defaults;
    _queue_unmark_waiter(queue, queues->mutex);
    return 0;
}

static int
queue_get_maxsize(_queues *queues, int64_t qid, Py_ssize_t *p_maxsize)
{
    _queue *queue = NULL;
    int err = _queues_lookup(queues, qid, &queue);
    if (err < 0) {
        return err;
    }
    err = _queue_get_maxsize(queue, p_maxsize);
    _queue_unmark_waiter(queue, queues->mutex);
    return err;
}

static int
queue_is_full(_queues *queues, int64_t qid, int *p_is_full)
{
    _queue *queue = NULL;
    int err = _queues_lookup(queues, qid, &queue);
    if (err < 0) {
        return err;
    }
    err = _queue_is_full(queue, p_is_full);
    _queue_unmark_waiter(queue, queues->mutex);
    return err;
}

static int
queue_get_count(_queues *queues, int64_t qid, Py_ssize_t *p_count)
{
    _queue *queue = NULL;
    int err = _queues_lookup(queues, qid, &queue);
    if (err < 0) {
        return err;
    }
    err = _queue_get_count(queue, p_count);
    _queue_unmark_waiter(queue, queues->mutex);
    return err;
}


/* external Queue objects ***************************************************/

static int _queueobj_shared(PyThreadState *, PyObject *, _PyXIData_t *);

static int
set_external_queue_type(module_state *state, PyTypeObject *queue_type)
{
    // Clear the old value if the .py module was reloaded.
    if (state->queue_type != NULL) {
        (void)clear_xid_class(state->queue_type);
        Py_CLEAR(state->queue_type);
    }

    // Add and register the new type.
    if (ensure_xid_class(queue_type, GETDATA(_queueobj_shared)) < 0) {
        return -1;
    }
    state->queue_type = (PyTypeObject *)Py_NewRef(queue_type);

    return 0;
}

static PyTypeObject *
get_external_queue_type(PyObject *module)
{
    module_state *state = get_module_state(module);

    PyTypeObject *cls = state->queue_type;
    if (cls == NULL) {
        // Force the module to be loaded, to register the type.
        if (ensure_highlevel_module_loaded() < 0) {
            return NULL;
        }
        cls = state->queue_type;
        assert(cls != NULL);
    }
    return cls;
}


// XXX Use a new __xid__ protocol instead?

struct _queueid_xid {
    int64_t qid;
};

static _queues * _get_global_queues(void);

static void *
_queueid_xid_new(int64_t qid)
{
    _queues *queues = _get_global_queues();
    if (_queues_incref(queues, qid) < 0) {
        return NULL;
    }

    struct _queueid_xid *data = PyMem_RawMalloc(sizeof(struct _queueid_xid));
    if (data == NULL) {
        _queues_decref(queues, qid);
        return NULL;
    }
    data->qid = qid;
    return (void *)data;
}

static void
_queueid_xid_free(void *data)
{
    int64_t qid = ((struct _queueid_xid *)data)->qid;
    PyMem_RawFree(data);
    _queues *queues = _get_global_queues();
    int res = _queues_decref(queues, qid);
    if (res == ERR_QUEUE_NOT_FOUND) {
        // Already destroyed.
        // XXX Warn?
    }
    else {
        assert(res == 0);
    }
}

static PyObject *
_queueobj_from_xid(_PyXIData_t *data)
{
    int64_t qid = *(int64_t *)_PyXIData_DATA(data);
    PyObject *qidobj = PyLong_FromLongLong(qid);
    if (qidobj == NULL) {
        return NULL;
    }

    PyObject *mod = _get_current_module();
    if (mod == NULL) {
        mod = PyImport_ImportModule(MODULE_NAME_STR);
        if (mod == NULL) {
            return NULL;
        }
    }

    PyTypeObject *cls = get_external_queue_type(mod);
    Py_DECREF(mod);
    if (cls == NULL) {
        Py_DECREF(qidobj);
        return NULL;
    }
    PyObject *obj = PyObject_CallOneArg((PyObject *)cls, (PyObject *)qidobj);
    Py_DECREF(qidobj);
    return obj;
}

static int
_queueobj_shared(PyThreadState *tstate, PyObject *queueobj, _PyXIData_t *data)
{
    PyObject *qidobj = PyObject_GetAttrString(queueobj, "_id");
    if (qidobj == NULL) {
        return -1;
    }
    struct idarg_int64_converter_data converted = {
        .label = "queue ID",
    };
    int res = idarg_int64_converter(qidobj, &converted);
    Py_CLEAR(qidobj);
    if (!res) {
        assert(PyErr_Occurred());
        return -1;
    }

    void *raw = _queueid_xid_new(converted.id);
    if (raw == NULL) {
        return -1;
    }
    _PyXIData_Init(data, tstate->interp, raw, NULL, _queueobj_from_xid);
    _PyXIData_SET_FREE(data, _queueid_xid_free);
    return 0;
}


/* module level code ********************************************************/

/* globals is the process-global state for the module.  It holds all
   the data that we need to share between interpreters, so it cannot
   hold PyObject values. */
static struct globals {
    PyMutex mutex;
    int module_count;
    _queues queues;
} _globals = {0};

static int
_globals_init(void)
{
    PyMutex_Lock(&_globals.mutex);
    assert(_globals.module_count >= 0);
    _globals.module_count++;
    if (_globals.module_count == 1) {
        // Called for the first time.
        PyThread_type_lock mutex = PyThread_allocate_lock();
        if (mutex == NULL) {
            _globals.module_count--;
            PyMutex_Unlock(&_globals.mutex);
            return ERR_QUEUES_ALLOC;
        }
        _queues_init(&_globals.queues, mutex);
    }
    PyMutex_Unlock(&_globals.mutex);
    return 0;
}

static void
_globals_fini(void)
{
    PyMutex_Lock(&_globals.mutex);
    assert(_globals.module_count > 0);
    _globals.module_count--;
    if (_globals.module_count == 0) {
        PyThread_type_lock mutex;
        _queues_fini(&_globals.queues, &mutex);
        assert(mutex != NULL);
        PyThread_free_lock(mutex);
    }
    PyMutex_Unlock(&_globals.mutex);
}

static _queues *
_get_global_queues(void)
{
    return &_globals.queues;
}


static void
clear_interpreter(void *data)
{
    if (_globals.module_count == 0) {
        return;
    }
    PyInterpreterState *interp = (PyInterpreterState *)data;
    assert(interp == _get_current_interp());
    int64_t interpid = PyInterpreterState_GetID(interp);
    _queues_clear_interpreter(&_globals.queues, interpid);
}


typedef struct idarg_int64_converter_data qidarg_converter_data;

static int
qidarg_converter(PyObject *arg, void *ptr)
{
    qidarg_converter_data *data = ptr;
    if (data->label == NULL) {
        data->label = "queue ID";
    }
    return idarg_int64_converter(arg, ptr);
}


static PyObject *
queuesmod_create(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"maxsize", "unboundop", "fallback", NULL};
    Py_ssize_t maxsize;
    int unboundarg = -1;
    int fallbackarg = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "n|ii:create", kwlist,
                                     &maxsize, &unboundarg, &fallbackarg))
    {
        return NULL;
    }
    struct _queuedefaults defaults = {0};
    if (resolve_unboundop(unboundarg, UNBOUND_REPLACE,
                          &defaults.unboundop) < 0)
    {
        return NULL;
    }
    if (resolve_fallback(fallbackarg, _PyXIDATA_FULL_FALLBACK,
                         &defaults.fallback) < 0)
    {
        return NULL;
    }

    int64_t qid = queue_create(&_globals.queues, maxsize, defaults);
    if (qid < 0) {
        (void)handle_queue_error((int)qid, self, qid);
        return NULL;
    }

    PyObject *qidobj = PyLong_FromLongLong(qid);
    if (qidobj == NULL) {
        PyObject *exc = PyErr_GetRaisedException();
        int err = queue_destroy(&_globals.queues, qid);
        if (handle_queue_error(err, self, qid)) {
            // XXX issue a warning?
            PyErr_Clear();
        }
        PyErr_SetRaisedException(exc);
        return NULL;
    }

    return qidobj;
}

PyDoc_STRVAR(queuesmod_create_doc,
"create(maxsize, unboundop, fallback) -> qid\n\
\n\
Create a new cross-interpreter queue and return its unique generated ID.\n\
It is a new reference as though bind() had been called on the queue.\n\
\n\
The caller is responsible for calling destroy() for the new queue\n\
before the runtime is finalized.");

static PyObject *
queuesmod_destroy(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg = {0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:destroy", kwlist,
                                     qidarg_converter, &qidarg)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    int err = queue_destroy(&_globals.queues, qid);
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(queuesmod_destroy_doc,
"destroy(qid)\n\
\n\
Clear and destroy the queue.  Afterward attempts to use the queue\n\
will behave as though it never existed.");

static PyObject *
queuesmod_list_all(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int64_t count = 0;
    struct queue_id_and_info *qids = _queues_list_all(&_globals.queues, &count);
    if (qids == NULL) {
        if (!PyErr_Occurred() && count == 0) {
            return PyList_New(0);
        }
        return NULL;
    }
    PyObject *ids = PyList_New((Py_ssize_t)count);
    if (ids == NULL) {
        goto finally;
    }
    struct queue_id_and_info *cur = qids;
    for (int64_t i=0; i < count; cur++, i++) {
        PyObject *item = Py_BuildValue("Lii", cur->id,
                                       cur->defaults.unboundop,
                                       cur->defaults.fallback);
        if (item == NULL) {
            Py_SETREF(ids, NULL);
            break;
        }
        PyList_SET_ITEM(ids, (Py_ssize_t)i, item);
    }

finally:
    PyMem_Free(qids);
    return ids;
}

PyDoc_STRVAR(queuesmod_list_all_doc,
"list_all() -> [(qid, unboundop, fallback)]\n\
\n\
Return the list of IDs for all queues.\n\
Each corresponding default unbound op and fallback is also included.");

static PyObject *
queuesmod_put(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", "obj", "unboundop", "fallback", NULL};
    qidarg_converter_data qidarg = {0};
    PyObject *obj;
    int unboundarg = -1;
    int fallbackarg = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O|ii$p:put", kwlist,
                                     qidarg_converter, &qidarg, &obj,
                                     &unboundarg, &fallbackarg))
    {
        return NULL;
    }
    int64_t qid = qidarg.id;
    struct _queuedefaults defaults = {-1, -1};
    if (unboundarg < 0 || fallbackarg < 0) {
        int err = queue_get_defaults(&_globals.queues, qid, &defaults);
        if (handle_queue_error(err, self, qid)) {
            return NULL;
        }
    }
    unboundop_t unboundop;
    if (resolve_unboundop(unboundarg, defaults.unboundop, &unboundop) < 0) {
        return NULL;
    }
    xidata_fallback_t fallback;
    if (resolve_fallback(fallbackarg, defaults.fallback, &fallback) < 0) {
        return NULL;
    }

    /* Queue up the object. */
    int err = queue_put(&_globals.queues, qid, obj, unboundop, fallback);
    // This is the only place that raises QueueFull.
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(queuesmod_put_doc,
"put(qid, obj)\n\
\n\
Add the object's data to the queue.");

static PyObject *
queuesmod_get(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg = {0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:get", kwlist,
                                     qidarg_converter, &qidarg)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    PyObject *obj = NULL;
    int unboundop = 0;
    int err = queue_get(&_globals.queues, qid, &obj, &unboundop);
    // This is the only place that raises QueueEmpty.
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }

    if (obj == NULL) {
        return Py_BuildValue("Oi", Py_None, unboundop);
    }
    PyObject *res = Py_BuildValue("OO", obj, Py_None);
    Py_DECREF(obj);
    return res;
}

PyDoc_STRVAR(queuesmod_get_doc,
"get(qid) -> (obj, unboundop)\n\
\n\
Return a new object from the data at the front of the queue.\n\
The unbound op is also returned.\n\
\n\
If there is nothing to receive then raise QueueEmpty.");

static PyObject *
queuesmod_bind(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg = {0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:bind", kwlist,
                                     qidarg_converter, &qidarg)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    // XXX Check module state if bound already.

    int err = _queues_incref(&_globals.queues, qid);
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }

    // XXX Update module state.

    Py_RETURN_NONE;
}

PyDoc_STRVAR(queuesmod_bind_doc,
"bind(qid)\n\
\n\
Take a reference to the identified queue.\n\
The queue is not destroyed until there are no references left.");

static PyObject *
queuesmod_release(PyObject *self, PyObject *args, PyObject *kwds)
{
    // Note that only the current interpreter is affected.
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg = {0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&:release", kwlist,
                                     qidarg_converter, &qidarg)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    // XXX Check module state if bound already.
    // XXX Update module state.

    int err = _queues_decref(&_globals.queues, qid);
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(queuesmod_release_doc,
"release(qid)\n\
\n\
Release a reference to the queue.\n\
The queue is destroyed once there are no references left.");

static PyObject *
queuesmod_get_maxsize(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg = {0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&:get_maxsize", kwlist,
                                     qidarg_converter, &qidarg)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    Py_ssize_t maxsize = -1;
    int err = queue_get_maxsize(&_globals.queues, qid, &maxsize);
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }
    return PyLong_FromLongLong(maxsize);
}

PyDoc_STRVAR(queuesmod_get_maxsize_doc,
"get_maxsize(qid)\n\
\n\
Return the maximum number of items in the queue.");

static PyObject *
queuesmod_get_queue_defaults(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg = {0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&:get_queue_defaults", kwlist,
                                     qidarg_converter, &qidarg)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    struct _queuedefaults defaults = {0};
    int err = queue_get_defaults(&_globals.queues, qid, &defaults);
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }

    PyObject *res = Py_BuildValue("ii", defaults.unboundop, defaults.fallback);
    return res;
}

PyDoc_STRVAR(queuesmod_get_queue_defaults_doc,
"get_queue_defaults(qid)\n\
\n\
Return the queue's default values, set when it was created.");

static PyObject *
queuesmod_is_full(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg = {0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&:is_full", kwlist,
                                     qidarg_converter, &qidarg)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    int is_full = 0;
    int err = queue_is_full(&_globals.queues, qid, &is_full);
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }
    if (is_full) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(queuesmod_is_full_doc,
"is_full(qid)\n\
\n\
Return true if the queue has a maxsize and has reached it.");

static PyObject *
queuesmod_get_count(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg = {0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&:get_count", kwlist,
                                     qidarg_converter, &qidarg)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    Py_ssize_t count = -1;
    int err = queue_get_count(&_globals.queues, qid, &count);
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }
    assert(count >= 0);
    return PyLong_FromSsize_t(count);
}

PyDoc_STRVAR(queuesmod_get_count_doc,
"get_count(qid)\n\
\n\
Return the number of items in the queue.");

static PyObject *
queuesmod__register_heap_types(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"queuetype", "emptyerror", "fullerror", NULL};
    PyObject *queuetype;
    PyObject *emptyerror;
    PyObject *fullerror;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OOO:_register_heap_types", kwlist,
                                     &queuetype, &emptyerror, &fullerror)) {
        return NULL;
    }
    if (!PyType_Check(queuetype)) {
        PyErr_SetString(PyExc_TypeError,
                        "expected a type for 'queuetype'");
        return NULL;
    }
    if (!PyExceptionClass_Check(emptyerror)) {
        PyErr_SetString(PyExc_TypeError,
                        "expected an exception type for 'emptyerror'");
        return NULL;
    }
    if (!PyExceptionClass_Check(fullerror)) {
        PyErr_SetString(PyExc_TypeError,
                        "expected an exception type for 'fullerror'");
        return NULL;
    }

    module_state *state = get_module_state(self);

    if (set_external_queue_type(state, (PyTypeObject *)queuetype) < 0) {
        return NULL;
    }
    if (set_external_exc_types(state, emptyerror, fullerror) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef module_functions[] = {
    {"create",                     _PyCFunction_CAST(queuesmod_create),
     METH_VARARGS | METH_KEYWORDS, queuesmod_create_doc},
    {"destroy",                    _PyCFunction_CAST(queuesmod_destroy),
     METH_VARARGS | METH_KEYWORDS, queuesmod_destroy_doc},
    {"list_all",                   queuesmod_list_all,
     METH_NOARGS,                  queuesmod_list_all_doc},
    {"put",                        _PyCFunction_CAST(queuesmod_put),
     METH_VARARGS | METH_KEYWORDS, queuesmod_put_doc},
    {"get",                        _PyCFunction_CAST(queuesmod_get),
     METH_VARARGS | METH_KEYWORDS, queuesmod_get_doc},
    {"bind",                       _PyCFunction_CAST(queuesmod_bind),
     METH_VARARGS | METH_KEYWORDS, queuesmod_bind_doc},
    {"release",                    _PyCFunction_CAST(queuesmod_release),
     METH_VARARGS | METH_KEYWORDS, queuesmod_release_doc},
    {"get_maxsize",                _PyCFunction_CAST(queuesmod_get_maxsize),
     METH_VARARGS | METH_KEYWORDS, queuesmod_get_maxsize_doc},
    {"get_queue_defaults",         _PyCFunction_CAST(queuesmod_get_queue_defaults),
     METH_VARARGS | METH_KEYWORDS, queuesmod_get_queue_defaults_doc},
    {"is_full",                    _PyCFunction_CAST(queuesmod_is_full),
     METH_VARARGS | METH_KEYWORDS, queuesmod_is_full_doc},
    {"get_count",                  _PyCFunction_CAST(queuesmod_get_count),
     METH_VARARGS | METH_KEYWORDS, queuesmod_get_count_doc},
    {"_register_heap_types",       _PyCFunction_CAST(queuesmod__register_heap_types),
     METH_VARARGS | METH_KEYWORDS, NULL},

    {NULL,                        NULL}           /* sentinel */
};


/* initialization function */

PyDoc_STRVAR(module_doc,
"This module provides primitive operations to manage Python interpreters.\n\
The 'interpreters' module provides a more convenient interface.");

static int
module_exec(PyObject *mod)
{
    int err = _globals_init();
    if (handle_queue_error(err, mod, -1)) {
        return -1;
    }

    /* Add exception types */
    if (add_QueueError(mod) < 0) {
        goto error;
    }

    /* Make sure queues drop objects owned by this interpreter. */
    PyInterpreterState *interp = _get_current_interp();
    PyUnstable_AtExit(interp, clear_interpreter, (void *)interp);

    return 0;

error:
    _globals_fini();
    return -1;
}

static struct PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static int
module_traverse(PyObject *mod, visitproc visit, void *arg)
{
    module_state *state = get_module_state(mod);
    return traverse_module_state(state, visit, arg);
}

static int
module_clear(PyObject *mod)
{
    module_state *state = get_module_state(mod);

    // Now we clear the module state.
    return clear_module_state(state);
}

static void
module_free(void *mod)
{
    module_state *state = get_module_state((PyObject *)mod);

    // Now we clear the module state.
    (void)clear_module_state(state);

    _globals_fini();
}

static struct PyModuleDef moduledef = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME_STR,
    .m_doc = module_doc,
    .m_size = sizeof(module_state),
    .m_methods = module_functions,
    .m_slots = module_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = module_free,
};

PyMODINIT_FUNC
MODINIT_FUNC_NAME(void)
{
    return PyModuleDef_Init(&moduledef);
}
