/* interpreters module */
/* low-level access to interpreter primitives */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_crossinterp.h"   // struct _xid


#define MODULE_NAME "_xxinterpqueues"


#define GLOBAL_MALLOC(TYPE) \
    PyMem_RawMalloc(sizeof(TYPE))
#define GLOBAL_FREE(VAR) \
    PyMem_RawFree(VAR)


#define XID_IGNORE_EXC 1
#define XID_FREE 2

static int
_release_xid_data(_PyCrossInterpreterData *data, int flags)
{
    int ignoreexc = flags & XID_IGNORE_EXC;
    PyObject *exc;
    if (ignoreexc) {
        exc = PyErr_GetRaisedException();
    }
    int res;
    if (flags & XID_FREE) {
        res = _PyCrossInterpreterData_ReleaseAndRawFree(data);
    }
    else {
        res = _PyCrossInterpreterData_Release(data);
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
add_new_exception(PyObject *mod, const char *name, PyObject *base)
{
    assert(!PyObject_HasAttrStringWithError(mod, name));
    PyObject *exctype = PyErr_NewException(name, base, NULL);
    if (exctype == NULL) {
        return NULL;
    }
    int res = PyModule_AddType(mod, (PyTypeObject *)exctype);
    if (res < 0) {
        Py_DECREF(exctype);
        return NULL;
    }
    return exctype;
}

#define ADD_NEW_EXCEPTION(MOD, NAME, BASE) \
    add_new_exception(MOD, MODULE_NAME "." Py_STRINGIFY(NAME), BASE)


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


/* module state *************************************************************/

typedef struct {
    /* exceptions */
    PyObject *QueueError;
    PyObject *QueueNotFoundError;
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
    /* exceptions */
    Py_VISIT(state->QueueError);
    Py_VISIT(state->QueueNotFoundError);

    return 0;
}

static int
clear_module_state(module_state *state)
{
    /* exceptions */
    Py_CLEAR(state->QueueError);
    Py_CLEAR(state->QueueNotFoundError);

    return 0;
}


/* queue-specific code ******************************************************/

/* queue errors */

#define ERR_QUEUE_NOT_FOUND -2
#define ERR_QUEUE_EMPTY -5
#define ERR_QUEUE_MUTEX_INIT -7
#define ERR_QUEUES_MUTEX_INIT -8
#define ERR_NO_NEXT_QUEUE_ID -9

static int
exceptions_init(PyObject *mod)
{
    module_state *state = get_module_state(mod);
    if (state == NULL) {
        return -1;
    }

#define ADD(NAME, BASE) \
    do { \
        assert(state->NAME == NULL); \
        state->NAME = ADD_NEW_EXCEPTION(mod, NAME, BASE); \
        if (state->NAME == NULL) { \
            return -1; \
        } \
    } while (0)

    // A queue-related operation failed.
    ADD(QueueError, PyExc_RuntimeError);
    // An operation tried to use a queue that doesn't exist.
    ADD(QueueNotFoundError, state->QueueError);
#undef ADD

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
    module_state *state = get_module_state(mod);
    assert(state != NULL);
    if (err == ERR_QUEUE_NOT_FOUND) {
        PyErr_Format(state->QueueNotFoundError,
                     "queue %" PRId64 " not found", qid);
    }
    else if (err == ERR_QUEUE_EMPTY) {
        // XXX
        PyErr_Format(state->QueueError,
        //PyErr_Format(state->QueueEmpty,
                     "queue %" PRId64 " is empty", qid);
    }
    else if (err == ERR_QUEUE_MUTEX_INIT) {
        PyErr_SetString(state->QueueError,
                        "can't initialize mutex for new queue");
    }
    else if (err == ERR_QUEUES_MUTEX_INIT) {
        PyErr_SetString(state->QueueError,
                        "can't initialize mutex for queue management");
    }
    else if (err == ERR_NO_NEXT_QUEUE_ID) {
        PyErr_SetString(state->QueueError,
                        "ran out of queue IDs");
    }
    else {
        assert(PyErr_Occurred());
    }
    return 1;
}


/* the channel queue */

typedef uintptr_t _queueitem_id_t;

struct _queueitem;

typedef struct _queueitem {
    _PyCrossInterpreterData *data;
    struct _queueitem *next;
} _queueitem;

static inline _queueitem_id_t
_queueitem_ID(_queueitem *item)
{
    return (_queueitem_id_t)item;
}

static void
_queueitem_init(_queueitem *item, _PyCrossInterpreterData *data)
{
    *item = (_queueitem){
        .data = data,
    };
}

static void
_queueitem_clear(_queueitem *item)
{
    item->next = NULL;

    if (item->data != NULL) {
        // It was allocated in channel_send().
        (void)_release_xid_data(item->data, XID_IGNORE_EXC & XID_FREE);
        item->data = NULL;
    }
}

static _queueitem *
_queueitem_new(_PyCrossInterpreterData *data)
{
    _queueitem *item = GLOBAL_MALLOC(_queueitem);
    if (item == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _queueitem_init(item, data);
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
_queueitem_popped(_queueitem *item, _PyCrossInterpreterData **p_data)
{
    *p_data = item->data;
    // We clear them here, so they won't be released in _queueitem_clear().
    item->data = NULL;
    _queueitem_free(item);
}

typedef struct _queueitems {
    int64_t count;
    _queueitem *first;
    _queueitem *last;
} _queueitems;

static _queueitems *
_queueitems_new(void)
{
    _queueitems *queue = GLOBAL_MALLOC(_queueitems);
    if (queue == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    queue->count = 0;
    queue->first = NULL;
    queue->last = NULL;
    return queue;
}

static void
_queueitems_clear(_queueitems *queue)
{
    _queueitem_free_all(queue->first);
    queue->count = 0;
    queue->first = NULL;
    queue->last = NULL;
}

static void
_queueitems_free(_queueitems *queue)
{
    _queueitems_clear(queue);
    GLOBAL_FREE(queue);
}

static int
_queueitems_put(_queueitems *queue,
                  _PyCrossInterpreterData *data)
{
    _queueitem *item = _queueitem_new(data);
    if (item == NULL) {
        return -1;
    }

    queue->count += 1;
    if (queue->first == NULL) {
        queue->first = item;
    }
    else {
        queue->last->next = item;
    }
    queue->last = item;

    return 0;
}

static int
_queueitems_get(_queueitems *queue, _PyCrossInterpreterData **p_data)
{
    _queueitem *item = queue->first;
    if (item == NULL) {
        return ERR_QUEUE_EMPTY;
    }
    queue->first = item->next;
    if (queue->last == item) {
        queue->last = NULL;
    }
    queue->count -= 1;

    _queueitem_popped(item, p_data);
    return 0;
}

static void
_queueitems_clear_interpreter(_queueitems *queue, int64_t interpid)
{
    _queueitem *prev = NULL;
    _queueitem *next = queue->first;
    while (next != NULL) {
        _queueitem *item = next;
        next = item->next;
        if (item->data->interpid == interpid) {
            if (prev == NULL) {
                queue->first = item->next;
            }
            else {
                prev->next = item->next;
            }
            _queueitem_free(item);
            queue->count -= 1;
        }
        else {
            prev = item;
        }
    }
}


/* each channel's state */

struct _queue;

typedef struct _queue {
    PyThread_type_lock mutex;
    _queueitems *items;
} _queue_state;

static _queue_state *
_queue_new(PyThread_type_lock mutex)
{
    _queue_state *queue = GLOBAL_MALLOC(_queue_state);
    if (queue == NULL) {
        return NULL;
    }
    queue->mutex = mutex;
    queue->items = _queueitems_new();
    if (queue->items == NULL) {
        GLOBAL_FREE(queue);
        return NULL;
    }
    return queue;
}

static void
_queue_free(_queue_state *queue)
{
    PyThread_acquire_lock(queue->mutex, WAIT_LOCK);
    _queueitems_free(queue->items);
    PyThread_release_lock(queue->mutex);

    PyThread_free_lock(queue->mutex);
    GLOBAL_FREE(queue);
}

static int
_queue_add(_queue_state *queue, _PyCrossInterpreterData *data)
{
    int res = -1;
    PyThread_acquire_lock(queue->mutex, WAIT_LOCK);

    if (_queueitems_put(queue->items, data) != 0) {
        goto done;
    }

    res = 0;
done:
    PyThread_release_lock(queue->mutex);
    return res;
}

static int
_queue_next(_queue_state *queue, _PyCrossInterpreterData **p_data)
{
    int err = 0;
    PyThread_acquire_lock(queue->mutex, WAIT_LOCK);

#ifdef NDEBUG
    (void)_queueitems_get(queue->items, p_data);
#else
    int empty = _queueitems_get(queue->items, p_data);
    assert(empty == 0 || empty == ERR_QUEUE_EMPTY);
#endif
    assert(!PyErr_Occurred());

    PyThread_release_lock(queue->mutex);
    return err;
}

static void
_queue_clear_interpreter(_queue_state *queue, int64_t interpid)
{
    PyThread_acquire_lock(queue->mutex, WAIT_LOCK);

    _queueitems_clear_interpreter(queue->items, interpid);

    PyThread_release_lock(queue->mutex);
}


/* the set of channels */

struct _queueref;

typedef struct _queueref {
    struct _queueref *next;
    int64_t qid;
    Py_ssize_t refcount;
    _queue_state *queue;
} _queueref;

static _queueref *
_queueref_new(int64_t qid, _queue_state *queue)
{
    _queueref *ref = GLOBAL_MALLOC(_queueref);
    if (ref == NULL) {
        return NULL;
    }
    ref->next = NULL;
    ref->qid = qid;
    ref->refcount = 0;
    ref->queue = queue;
    return ref;
}

static void
_queueref_free(_queueref *ref)
{
    assert(ref->next == NULL);
    // ref->queue is freed by the caller.
    GLOBAL_FREE(ref);
}

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


typedef struct _queues {
    PyThread_type_lock mutex;
    _queueref *head;
    int64_t count;
    int64_t next_id;
} _queues;

static void
_queues_init(_queues *queues, PyThread_type_lock mutex)
{
    queues->mutex = mutex;
    queues->head = NULL;
    queues->count = 0;
    queues->next_id = 1;
}

static void
_queues_fini(_queues *queues)
{
    assert(queues->count == 0);
    assert(queues->head == NULL);
    if (queues->mutex != NULL) {
        PyThread_free_lock(queues->mutex);
        queues->mutex = NULL;
    }
}

static int64_t
_queues_next_id(_queues *queues)  // needs lock
{
    int64_t qid = queues->next_id;
    if (qid < 0) {
        /* overflow */
        return -1;
    }
    queues->next_id += 1;
    return qid;
}

static int
_queues_lookup(_queues *queues, int64_t qid, PyThread_type_lock *pmutex,
               _queue_state **res)
{
    int err = -1;
    _queue_state *queue = NULL;
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);
    if (pmutex != NULL) {
        *pmutex = NULL;
    }

    _queueref *ref = _queuerefs_find(queues->head, qid, NULL);
    if (ref == NULL) {
        err = ERR_QUEUE_NOT_FOUND;
        goto done;
    }
    assert(ref->queue != NULL);

    if (pmutex != NULL) {
        // The mutex will be closed by the caller.
        *pmutex = queues->mutex;
    }

    queue = ref->queue;
    err = 0;

done:
    if (pmutex == NULL || *pmutex == NULL) {
        PyThread_release_lock(queues->mutex);
    }
    *res = queue;
    return err;
}

static int64_t
_queues_add(_queues *queues, _queue_state *queue)
{
    int64_t qid = -1;
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    // Create a new ref.
    int64_t _qid = _queues_next_id(queues);
    if (_qid < 0) {
        qid = ERR_NO_NEXT_QUEUE_ID;
        goto done;
    }
    _queueref *ref = _queueref_new(_qid, queue);
    if (ref == NULL) {
        goto done;
    }

    // Add it to the list.
    // We assume that the channel is a new one (not already in the list).
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
                   _queue_state **p_queue)
{
    if (ref == queues->head) {
        queues->head = ref->next;
    }
    else {
        prev->next = ref->next;
    }
    ref->next = NULL;
    queues->count -= 1;

    if (p_queue != NULL) {
        *p_queue = ref->queue;
    }
    _queueref_free(ref);
}

static int
_queues_remove(_queues *queues, int64_t qid, _queue_state **p_queue)
{
    int res = -1;
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    if (p_queue != NULL) {
        *p_queue = NULL;
    }

    _queueref *prev = NULL;
    _queueref *ref = _queuerefs_find(queues->head, qid, &prev);
    if (ref == NULL) {
        res = ERR_QUEUE_NOT_FOUND;
        goto done;
    }

    _queues_remove_ref(queues, ref, prev, p_queue);

    res = 0;
done:
    PyThread_release_lock(queues->mutex);
    return res;
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

static void
_queues_decref(_queues *queues, int64_t qid)
{
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    _queueref *prev = NULL;
    _queueref *ref = _queuerefs_find(queues->head, qid, &prev);
    if (ref == NULL) {
        assert(!PyErr_Occurred());
        // Already destroyed.
        // XXX Warn?
        goto done;
    }
    assert(ref->refcount > 0);
    ref->refcount -= 1;

    // Destroy if no longer used.
    if (ref->refcount == 0) {
        _queue_state *queue = NULL;
        _queues_remove_ref(queues, ref, prev, &queue);
        if (queue != NULL) {
            _queue_free(queue);
        }
    }

done:
    PyThread_release_lock(queues->mutex);
}

static int64_t *
_queues_list_all(_queues *queues, int64_t *count)
{
    int64_t *qids = NULL;
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);
    int64_t *ids = PyMem_NEW(int64_t, (Py_ssize_t)(queues->count));
    if (ids == NULL) {
        goto done;
    }
    _queueref *ref = queues->head;
    for (int64_t i=0; ref != NULL; ref = ref->next, i++) {
        ids[i] = ref->qid;
    }
    *count = queues->count;

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


/* "high"-level channel-related functions */

// Create a new channel.
static int64_t
queue_create(_queues *queues)
{
    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        return ERR_QUEUE_MUTEX_INIT;
    }
    _queue_state *queue = _queue_new(mutex);
    if (queue == NULL) {
        PyThread_free_lock(mutex);
        return -1;
    }
    int64_t qid = _queues_add(queues, queue);
    if (qid < 0) {
        _queue_free(queue);
    }
    return qid;
}

// Completely destroy the channel.
static int
queue_destroy(_queues *queues, int64_t qid)
{
    _queue_state *queue = NULL;
    int err = _queues_remove(queues, qid, &queue);
    if (err != 0) {
        return err;
    }
    if (queue != NULL) {
        _queue_free(queue);
    }
    return 0;
}

// Push an object onto the queue.
static int
queue_put(_queues *queues, int64_t qid, PyObject *obj)
{
    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _queue_state *queue = NULL;
    int err = _queues_lookup(queues, qid, &mutex, &queue);
    if (err != 0) {
        return err;
    }
    assert(queue != NULL);
    // Past this point we are responsible for releasing the mutex.

    // Convert the object to cross-interpreter data.
    _PyCrossInterpreterData *data = GLOBAL_MALLOC(_PyCrossInterpreterData);
    if (data == NULL) {
        PyThread_release_lock(mutex);
        return -1;
    }
    if (_PyObject_GetCrossInterpreterData(obj, data) != 0) {
        PyThread_release_lock(mutex);
        GLOBAL_FREE(data);
        return -1;
    }

    // Add the data to the channel.
    int res = _queue_add(queue, data);
    PyThread_release_lock(mutex);
    if (res != 0) {
        // We may chain an exception here:
        (void)_release_xid_data(data, 0);
        GLOBAL_FREE(data);
        return res;
    }

    return 0;
}

// Pop the next object off the channel.  Fail if empty.
// The current interpreter gets associated with the recv end of the channel.
// XXX Support a "wait" mutex?
static int
queue_get(_queues *queues, int64_t qid, PyObject **res)
{
    int err;
    *res = NULL;

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _queue_state *queue = NULL;
    err = _queues_lookup(queues, qid, &mutex, &queue);
    if (err != 0) {
        return err;
    }
    assert(queue != NULL);
    // Past this point we are responsible for releasing the mutex.

    // Pop off the next item from the channel.
    _PyCrossInterpreterData *data = NULL;
    err = _queue_next(queue, &data);
    PyThread_release_lock(mutex);
    if (err != 0) {
        return err;
    }
    else if (data == NULL) {
        assert(!PyErr_Occurred());
        return 0;
    }

    // Convert the data back to an object.
    PyObject *obj = _PyCrossInterpreterData_NewObject(data);
    if (obj == NULL) {
        assert(PyErr_Occurred());
        // It was allocated in channel_send(), so we free it.
        (void)_release_xid_data(data, XID_IGNORE_EXC | XID_FREE);
        return -1;
    }
    // It was allocated in channel_send(), so we free it.
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


/* channel info */

static int
queue_get_count(_queues *queues, int64_t qid, Py_ssize_t *p_count)
{
    int err = 0;

    // Hold the global lock until we're done.
    PyThread_acquire_lock(queues->mutex, WAIT_LOCK);

    // Find the channel.
    _queueref *ref = _queuerefs_find(queues->head, qid, NULL);
    if (ref == NULL) {
        err = ERR_QUEUE_NOT_FOUND;
        goto finally;
    }
    _queue_state *queue = ref->queue;
    assert(queue != NULL);

    // Get the number of queued objects.
    assert(queue->items->count <= PY_SSIZE_T_MAX);
    *p_count = (Py_ssize_t)queue->items->count;

finally:
    PyThread_release_lock(queues->mutex);
    return err;
}


/* module level code ********************************************************/

/* globals is the process-global state for the module.  It holds all
   the data that we need to share between interpreters, so it cannot
   hold PyObject values. */
static struct globals {
    int module_count;
    _queues queues;
} _globals = {0};

static int
_globals_init(void)
{
    // XXX This isn't thread-safe.
    _globals.module_count++;
    if (_globals.module_count > 1) {
        // Already initialized.
        return 0;
    }

    assert(_globals.queues.mutex == NULL);
    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        return ERR_QUEUES_MUTEX_INIT;
    }
    _queues_init(&_globals.queues, mutex);
    return 0;
}

static void
_globals_fini(void)
{
    // XXX This isn't thread-safe.
    _globals.module_count--;
    if (_globals.module_count > 0) {
        return;
    }

    _queues_fini(&_globals.queues);
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
queuesmod_create(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int64_t qid = queue_create(&_globals.queues);
    if (qid < 0) {
        (void)handle_queue_error(-1, self, qid);
        return NULL;
    }
    module_state *state = get_module_state(self);
    if (state == NULL) {
        return NULL;
    }
    PyObject *qidobj = PyLong_FromLongLong(qid);
    if (qidobj == NULL) {
        int err = queue_destroy(&_globals.queues, qid);
        if (handle_queue_error(err, self, qid)) {
            // XXX issue a warning?
        }
        return NULL;
    }
    return qidobj;
}

PyDoc_STRVAR(queuesmod_create_doc,
"create() -> qid\n\
\n\
Create a new cross-interpreter queue and return its unique generated ID.\n\
It is a new reference as though bind() had been called on the queue.");

static PyObject *
queuesmod_destroy(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg;
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
    int64_t *qids = _queues_list_all(&_globals.queues, &count);
    if (qids == NULL) {
        if (count == 0) {
            return PyList_New(0);
        }
        return NULL;
    }
    PyObject *ids = PyList_New((Py_ssize_t)count);
    if (ids == NULL) {
        goto finally;
    }
    module_state *state = get_module_state(self);
    if (state == NULL) {
        Py_DECREF(ids);
        ids = NULL;
        goto finally;
    }
    int64_t *cur = qids;
    for (int64_t i=0; i < count; cur++, i++) {
        PyObject *qidobj = PyLong_FromLongLong(*cur);
        if (qidobj == NULL) {
            Py_SETREF(ids, NULL);
            break;
        }
        PyList_SET_ITEM(ids, (Py_ssize_t)i, qidobj);
    }

finally:
    PyMem_Free(qids);
    return ids;
}

PyDoc_STRVAR(queuesmod_list_all_doc,
"list_all() -> [qid]\n\
\n\
Return the list of IDs for all queues.");

static PyObject *
queuesmod_put(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", "obj", NULL};
    qidarg_converter_data qidarg;
    PyObject *obj;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O:put", kwlist,
                                     qidarg_converter, &qidarg, &obj)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    /* Queue up the object. */
    int err = queue_put(&_globals.queues, qid, obj);
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
    static char *kwlist[] = {"qid", "default", NULL};
    qidarg_converter_data qidarg;
    PyObject *dflt = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&|O:get", kwlist,
                                     qidarg_converter, &qidarg, &dflt)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    PyObject *obj = NULL;
    int err = queue_get(&_globals.queues, qid, &obj);
    if (handle_queue_error(err, self, qid)) {
        return NULL;
    }
    Py_XINCREF(dflt);
    if (obj == NULL) {
        // Use the default.
        if (dflt == NULL) {
            (void)handle_queue_error(ERR_QUEUE_EMPTY, self, qid);
            return NULL;
        }
        obj = Py_NewRef(dflt);
    }
    Py_XDECREF(dflt);
    return obj;
}

PyDoc_STRVAR(queuesmod_get_doc,
"get(qid, [default]) -> obj\n\
\n\
Return a new object from the data at the front of the queue.\n\
\n\
If there is nothing to receive then raise QueueEmpty, unless\n\
a default value is provided.  In that case return it.");

static PyObject *
queuesmod_bind(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg;
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
    qidarg_converter_data qidarg;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&:release", kwlist,
                                     qidarg_converter, &qidarg)) {
        return NULL;
    }
    int64_t qid = qidarg.id;

    // XXX Check module state if bound already.
    // XXX Update module state.

    _queues_decref(&_globals.queues, qid);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(queuesmod_release_doc,
"release(qid)\n\
\n\
Release a reference to the queue.\n\
The queue is destroyed once there are no references left.");

static PyObject *
queuesmod_get_count(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"qid", NULL};
    qidarg_converter_data qidarg;
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

static PyMethodDef module_functions[] = {
    {"create",                     queuesmod_create,
     METH_NOARGS,                  queuesmod_create_doc},
    {"destroy",                    _PyCFunction_CAST(queuesmod_destroy),
     METH_VARARGS | METH_KEYWORDS, queuesmod_destroy_doc},
    {"list_all",                   queuesmod_list_all,
     METH_NOARGS,                  queuesmod_list_all_doc},
    {"put",                       _PyCFunction_CAST(queuesmod_put),
     METH_VARARGS | METH_KEYWORDS, queuesmod_put_doc},
    {"get",                       _PyCFunction_CAST(queuesmod_get),
     METH_VARARGS | METH_KEYWORDS, queuesmod_get_doc},
    {"bind",                      _PyCFunction_CAST(queuesmod_bind),
     METH_VARARGS | METH_KEYWORDS, queuesmod_bind_doc},
    {"release",                    _PyCFunction_CAST(queuesmod_release),
     METH_VARARGS | METH_KEYWORDS, queuesmod_release_doc},
    {"get_count",                  _PyCFunction_CAST(queuesmod_get_count),
     METH_VARARGS | METH_KEYWORDS, queuesmod_get_count_doc},

    {NULL,                        NULL}           /* sentinel */
};


/* initialization function */

PyDoc_STRVAR(module_doc,
"This module provides primitive operations to manage Python interpreters.\n\
The 'interpreters' module provides a more convenient interface.");

static int
module_exec(PyObject *mod)
{
    if (_globals_init() != 0) {
        return -1;
    }

    module_state *state = get_module_state(mod);
    if (state == NULL) {
        goto error;
    }

    /* Add exception types */
    if (exceptions_init(mod) != 0) {
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
    {0, NULL},
};

static int
module_traverse(PyObject *mod, visitproc visit, void *arg)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);
    traverse_module_state(state, visit, arg);
    return 0;
}

static int
module_clear(PyObject *mod)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);

    // Now we clear the module state.
    clear_module_state(state);
    return 0;
}

static void
module_free(void *mod)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);

    // Now we clear the module state.
    clear_module_state(state);

    _globals_fini();
}

static struct PyModuleDef moduledef = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME,
    .m_doc = module_doc,
    .m_size = sizeof(module_state),
    .m_methods = module_functions,
    .m_slots = module_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = (freefunc)module_free,
};

PyMODINIT_FUNC
PyInit__xxinterpqueues(void)
{
    return PyModuleDef_Init(&moduledef);
}
