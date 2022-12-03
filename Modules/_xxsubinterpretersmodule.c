
/* interpreters module */
/* low-level access to interpreter primitives */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
// XXX This module should not rely on internal API.
#include "pycore_frame.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_interpreteridobject.h"


#define MODULE_NAME "_xxsubinterpreters"


static char *
_copy_raw_string(PyObject *strobj)
{
    const char *str = PyUnicode_AsUTF8(strobj);
    if (str == NULL) {
        return NULL;
    }
    char *copied = PyMem_Malloc(strlen(str)+1);
    if (copied == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    strcpy(copied, str);
    return copied;
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
    // We ensured it was imported in _run_script().
    PyObject *name = PyUnicode_FromString(MODULE_NAME);
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

static PyObject *
get_module_from_owned_type(PyTypeObject *cls)
{
    assert(cls != NULL);
    return _get_current_module();
    // XXX Use the more efficient API now that we use heap types:
    //return PyType_GetModule(cls);
}

static struct PyModuleDef moduledef;

static PyObject *
get_module_from_type(PyTypeObject *cls)
{
    assert(cls != NULL);
    return _get_current_module();
    // XXX Use the more efficient API now that we use heap types:
    //return PyType_GetModuleByDef(cls, &moduledef);
}

static PyObject *
add_new_exception(PyObject *mod, const char *name, PyObject *base)
{
    assert(!PyObject_HasAttrString(mod, name));
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

static PyTypeObject *
add_new_type(PyObject *mod, PyTypeObject *cls, crossinterpdatafunc shared)
{
    if (PyType_Ready(cls) != 0) {
        return NULL;
    }
    if (PyModule_AddType(mod, cls) != 0) {
        // XXX When this becomes a heap type, we need to decref here.
        return NULL;
    }
    if (shared != NULL) {
        if (_PyCrossInterpreterData_RegisterClass(cls, shared)) {
            // XXX When this becomes a heap type, we need to decref here.
            return NULL;
        }
    }
    return cls;
}

static int
_release_xid_data(_PyCrossInterpreterData *data, int ignoreexc)
{
    PyObject *exctype, *excval, *exctb;
    if (ignoreexc) {
        PyErr_Fetch(&exctype, &excval, &exctb);
    }
    int res = _PyCrossInterpreterData_Release(data);
    if (res < 0) {
        // XXX Fix this!
        /* The owning interpreter is already destroyed.
         * Ideally, this shouldn't ever happen.  When an interpreter is
         * about to be destroyed, we should clear out all of its objects
         * from every channel associated with that interpreter.
         * For now we hack around that to resolve refleaks, by decref'ing
         * the released object here, even if its the wrong interpreter.
         * The owning interpreter has already been destroyed
         * so we should be okay, especially since the currently
         * shareable types are all very basic, with no GC.
         * That said, it becomes much messier once interpreters
         * no longer share a GIL, so this needs to be fixed before then. */
        // We do what _release_xidata() does in pystate.c.
        if (data->free != NULL) {
            data->free(data->data);
            data->data = NULL;
        }
        Py_CLEAR(data->obj);
        if (ignoreexc) {
            // XXX Emit a warning?
            PyErr_Clear();
        }
    }
    if (ignoreexc) {
        PyErr_Restore(exctype, excval, exctb);
    }
    return res;
}


/* data-sharing-specific code ***********************************************/

struct _sharednsitem {
    char *name;
    _PyCrossInterpreterData data;
};

static void _sharednsitem_clear(struct _sharednsitem *);  // forward

static int
_sharednsitem_init(struct _sharednsitem *item, PyObject *key, PyObject *value)
{
    item->name = _copy_raw_string(key);
    if (item->name == NULL) {
        return -1;
    }
    if (_PyObject_GetCrossInterpreterData(value, &item->data) != 0) {
        _sharednsitem_clear(item);
        return -1;
    }
    return 0;
}

static void
_sharednsitem_clear(struct _sharednsitem *item)
{
    if (item->name != NULL) {
        PyMem_Free(item->name);
        item->name = NULL;
    }
    (void)_release_xid_data(&item->data, 1);
}

static int
_sharednsitem_apply(struct _sharednsitem *item, PyObject *ns)
{
    PyObject *name = PyUnicode_FromString(item->name);
    if (name == NULL) {
        return -1;
    }
    PyObject *value = _PyCrossInterpreterData_NewObject(&item->data);
    if (value == NULL) {
        Py_DECREF(name);
        return -1;
    }
    int res = PyDict_SetItem(ns, name, value);
    Py_DECREF(name);
    Py_DECREF(value);
    return res;
}

typedef struct _sharedns {
    Py_ssize_t len;
    struct _sharednsitem* items;
} _sharedns;

static _sharedns *
_sharedns_new(Py_ssize_t len)
{
    _sharedns *shared = PyMem_NEW(_sharedns, 1);
    if (shared == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    shared->len = len;
    shared->items = PyMem_NEW(struct _sharednsitem, len);
    if (shared->items == NULL) {
        PyErr_NoMemory();
        PyMem_Free(shared);
        return NULL;
    }
    return shared;
}

static void
_sharedns_free(_sharedns *shared)
{
    for (Py_ssize_t i=0; i < shared->len; i++) {
        _sharednsitem_clear(&shared->items[i]);
    }
    PyMem_Free(shared->items);
    PyMem_Free(shared);
}

static _sharedns *
_get_shared_ns(PyObject *shareable, PyTypeObject *channelidtype,
               int *needs_import)
{
    *needs_import = 0;
    if (shareable == NULL || shareable == Py_None) {
        return NULL;
    }
    Py_ssize_t len = PyDict_Size(shareable);
    if (len == 0) {
        return NULL;
    }

    _sharedns *shared = _sharedns_new(len);
    if (shared == NULL) {
        return NULL;
    }
    Py_ssize_t pos = 0;
    for (Py_ssize_t i=0; i < len; i++) {
        PyObject *key, *value;
        if (PyDict_Next(shareable, &pos, &key, &value) == 0) {
            break;
        }
        if (_sharednsitem_init(&shared->items[i], key, value) != 0) {
            break;
        }
        if (Py_TYPE(value) == channelidtype) {
            *needs_import = 1;
        }
    }
    if (PyErr_Occurred()) {
        _sharedns_free(shared);
        return NULL;
    }
    return shared;
}

static int
_sharedns_apply(_sharedns *shared, PyObject *ns)
{
    for (Py_ssize_t i=0; i < shared->len; i++) {
        if (_sharednsitem_apply(&shared->items[i], ns) != 0) {
            return -1;
        }
    }
    return 0;
}

// Ultimately we'd like to preserve enough information about the
// exception and traceback that we could re-constitute (or at least
// simulate, a la traceback.TracebackException), and even chain, a copy
// of the exception in the calling interpreter.

typedef struct _sharedexception {
    char *name;
    char *msg;
} _sharedexception;

static _sharedexception *
_sharedexception_new(void)
{
    _sharedexception *err = PyMem_NEW(_sharedexception, 1);
    if (err == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    err->name = NULL;
    err->msg = NULL;
    return err;
}

static void
_sharedexception_clear(_sharedexception *exc)
{
    if (exc->name != NULL) {
        PyMem_Free(exc->name);
    }
    if (exc->msg != NULL) {
        PyMem_Free(exc->msg);
    }
}

static void
_sharedexception_free(_sharedexception *exc)
{
    _sharedexception_clear(exc);
    PyMem_Free(exc);
}

static _sharedexception *
_sharedexception_bind(PyObject *exctype, PyObject *exc, PyObject *tb)
{
    assert(exctype != NULL);
    char *failure = NULL;

    _sharedexception *err = _sharedexception_new();
    if (err == NULL) {
        goto finally;
    }

    PyObject *name = PyUnicode_FromFormat("%S", exctype);
    if (name == NULL) {
        failure = "unable to format exception type name";
        goto finally;
    }
    err->name = _copy_raw_string(name);
    Py_DECREF(name);
    if (err->name == NULL) {
        if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
            failure = "out of memory copying exception type name";
        } else {
            failure = "unable to encode and copy exception type name";
        }
        goto finally;
    }

    if (exc != NULL) {
        PyObject *msg = PyUnicode_FromFormat("%S", exc);
        if (msg == NULL) {
            failure = "unable to format exception message";
            goto finally;
        }
        err->msg = _copy_raw_string(msg);
        Py_DECREF(msg);
        if (err->msg == NULL) {
            if (PyErr_ExceptionMatches(PyExc_MemoryError)) {
                failure = "out of memory copying exception message";
            } else {
                failure = "unable to encode and copy exception message";
            }
            goto finally;
        }
    }

finally:
    if (failure != NULL) {
        PyErr_Clear();
        if (err->name != NULL) {
            PyMem_Free(err->name);
            err->name = NULL;
        }
        err->msg = failure;
    }
    return err;
}

static void
_sharedexception_apply(_sharedexception *exc, PyObject *wrapperclass)
{
    if (exc->name != NULL) {
        if (exc->msg != NULL) {
            PyErr_Format(wrapperclass, "%s: %s",  exc->name, exc->msg);
        }
        else {
            PyErr_SetString(wrapperclass, exc->name);
        }
    }
    else if (exc->msg != NULL) {
        PyErr_SetString(wrapperclass, exc->msg);
    }
    else {
        PyErr_SetNone(wrapperclass);
    }
}


/* channel-specific code ****************************************************/

#define CHANNEL_SEND 1
#define CHANNEL_BOTH 0
#define CHANNEL_RECV -1

/* channel errors */

#define ERR_CHANNEL_NOT_FOUND -2
#define ERR_CHANNEL_CLOSED -3
#define ERR_CHANNEL_INTERP_CLOSED -4
#define ERR_CHANNEL_EMPTY -5
#define ERR_CHANNEL_NOT_EMPTY -6
#define ERR_CHANNEL_MUTEX_INIT -7
#define ERR_CHANNELS_MUTEX_INIT -8
#define ERR_NO_NEXT_CHANNEL_ID -9

static PyObject *ChannelError;
static PyObject *ChannelNotFoundError;
static PyObject *ChannelClosedError;
static PyObject *ChannelEmptyError;
static PyObject *ChannelNotEmptyError;

static int
channel_exceptions_init(PyObject *mod)
{
    // XXX Move the exceptions into per-module memory?

#define ADD(NAME, BASE) \
    do { \
        if (NAME == NULL) { \
            NAME = ADD_NEW_EXCEPTION(mod, NAME, BASE); \
            if (NAME == NULL) { \
                return -1; \
            } \
        } \
    } while (0)

    // A channel-related operation failed.
    ADD(ChannelError, PyExc_RuntimeError);
    // An operation tried to use a channel that doesn't exist.
    ADD(ChannelNotFoundError, ChannelError);
    // An operation tried to use a closed channel.
    ADD(ChannelClosedError, ChannelError);
    // An operation tried to pop from an empty channel.
    ADD(ChannelEmptyError, ChannelError);
    // An operation tried to close a non-empty channel.
    ADD(ChannelNotEmptyError, ChannelError);
#undef ADD

    return 0;
}

static int
handle_channel_error(int err, PyObject *Py_UNUSED(mod), int64_t cid)
{
    if (err == 0) {
        assert(!PyErr_Occurred());
        return 0;
    }
    assert(err < 0);
    if (err == ERR_CHANNEL_NOT_FOUND) {
        PyErr_Format(ChannelNotFoundError,
                     "channel %" PRId64 " not found", cid);
    }
    else if (err == ERR_CHANNEL_CLOSED) {
        PyErr_Format(ChannelClosedError,
                     "channel %" PRId64 " is closed", cid);
    }
    else if (err == ERR_CHANNEL_INTERP_CLOSED) {
        PyErr_Format(ChannelClosedError,
                     "channel %" PRId64 " is already closed", cid);
    }
    else if (err == ERR_CHANNEL_EMPTY) {
        PyErr_Format(ChannelEmptyError,
                     "channel %" PRId64 " is empty", cid);
    }
    else if (err == ERR_CHANNEL_NOT_EMPTY) {
        PyErr_Format(ChannelNotEmptyError,
                     "channel %" PRId64 " may not be closed "
                     "if not empty (try force=True)",
                     cid);
    }
    else if (err == ERR_CHANNEL_MUTEX_INIT) {
        PyErr_SetString(ChannelError,
                        "can't initialize mutex for new channel");
    }
    else if (err == ERR_CHANNELS_MUTEX_INIT) {
        PyErr_SetString(ChannelError,
                        "can't initialize mutex for channel management");
    }
    else if (err == ERR_NO_NEXT_CHANNEL_ID) {
        PyErr_SetString(ChannelError,
                        "failed to get a channel ID");
    }
    else {
        assert(PyErr_Occurred());
    }
    return 1;
}

/* the channel queue */

struct _channelitem;

typedef struct _channelitem {
    _PyCrossInterpreterData *data;
    struct _channelitem *next;
} _channelitem;

static _channelitem *
_channelitem_new(void)
{
    _channelitem *item = PyMem_NEW(_channelitem, 1);
    if (item == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    item->data = NULL;
    item->next = NULL;
    return item;
}

static void
_channelitem_clear(_channelitem *item)
{
    if (item->data != NULL) {
        (void)_release_xid_data(item->data, 1);
        PyMem_Free(item->data);
        item->data = NULL;
    }
    item->next = NULL;
}

static void
_channelitem_free(_channelitem *item)
{
    _channelitem_clear(item);
    PyMem_Free(item);
}

static void
_channelitem_free_all(_channelitem *item)
{
    while (item != NULL) {
        _channelitem *last = item;
        item = item->next;
        _channelitem_free(last);
    }
}

static _PyCrossInterpreterData *
_channelitem_popped(_channelitem *item)
{
    _PyCrossInterpreterData *data = item->data;
    item->data = NULL;
    _channelitem_free(item);
    return data;
}

typedef struct _channelqueue {
    int64_t count;
    _channelitem *first;
    _channelitem *last;
} _channelqueue;

static _channelqueue *
_channelqueue_new(void)
{
    _channelqueue *queue = PyMem_NEW(_channelqueue, 1);
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
_channelqueue_clear(_channelqueue *queue)
{
    _channelitem_free_all(queue->first);
    queue->count = 0;
    queue->first = NULL;
    queue->last = NULL;
}

static void
_channelqueue_free(_channelqueue *queue)
{
    _channelqueue_clear(queue);
    PyMem_Free(queue);
}

static int
_channelqueue_put(_channelqueue *queue, _PyCrossInterpreterData *data)
{
    _channelitem *item = _channelitem_new();
    if (item == NULL) {
        return -1;
    }
    item->data = data;

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

static _PyCrossInterpreterData *
_channelqueue_get(_channelqueue *queue)
{
    _channelitem *item = queue->first;
    if (item == NULL) {
        return NULL;
    }
    queue->first = item->next;
    if (queue->last == item) {
        queue->last = NULL;
    }
    queue->count -= 1;

    return _channelitem_popped(item);
}

/* channel-interpreter associations */

struct _channelend;

typedef struct _channelend {
    struct _channelend *next;
    int64_t interp;
    int open;
} _channelend;

static _channelend *
_channelend_new(int64_t interp)
{
    _channelend *end = PyMem_NEW(_channelend, 1);
    if (end == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    end->next = NULL;
    end->interp = interp;
    end->open = 1;
    return end;
}

static void
_channelend_free(_channelend *end)
{
    PyMem_Free(end);
}

static void
_channelend_free_all(_channelend *end)
{
    while (end != NULL) {
        _channelend *last = end;
        end = end->next;
        _channelend_free(last);
    }
}

static _channelend *
_channelend_find(_channelend *first, int64_t interp, _channelend **pprev)
{
    _channelend *prev = NULL;
    _channelend *end = first;
    while (end != NULL) {
        if (end->interp == interp) {
            break;
        }
        prev = end;
        end = end->next;
    }
    if (pprev != NULL) {
        *pprev = prev;
    }
    return end;
}

typedef struct _channelassociations {
    // Note that the list entries are never removed for interpreter
    // for which the channel is closed.  This should not be a problem in
    // practice.  Also, a channel isn't automatically closed when an
    // interpreter is destroyed.
    int64_t numsendopen;
    int64_t numrecvopen;
    _channelend *send;
    _channelend *recv;
} _channelends;

static _channelends *
_channelends_new(void)
{
    _channelends *ends = PyMem_NEW(_channelends, 1);
    if (ends== NULL) {
        return NULL;
    }
    ends->numsendopen = 0;
    ends->numrecvopen = 0;
    ends->send = NULL;
    ends->recv = NULL;
    return ends;
}

static void
_channelends_clear(_channelends *ends)
{
    _channelend_free_all(ends->send);
    ends->send = NULL;
    ends->numsendopen = 0;

    _channelend_free_all(ends->recv);
    ends->recv = NULL;
    ends->numrecvopen = 0;
}

static void
_channelends_free(_channelends *ends)
{
    _channelends_clear(ends);
    PyMem_Free(ends);
}

static _channelend *
_channelends_add(_channelends *ends, _channelend *prev, int64_t interp,
                 int send)
{
    _channelend *end = _channelend_new(interp);
    if (end == NULL) {
        return NULL;
    }

    if (prev == NULL) {
        if (send) {
            ends->send = end;
        }
        else {
            ends->recv = end;
        }
    }
    else {
        prev->next = end;
    }
    if (send) {
        ends->numsendopen += 1;
    }
    else {
        ends->numrecvopen += 1;
    }
    return end;
}

static int
_channelends_associate(_channelends *ends, int64_t interp, int send)
{
    _channelend *prev;
    _channelend *end = _channelend_find(send ? ends->send : ends->recv,
                                        interp, &prev);
    if (end != NULL) {
        if (!end->open) {
            return ERR_CHANNEL_CLOSED;
        }
        // already associated
        return 0;
    }
    if (_channelends_add(ends, prev, interp, send) == NULL) {
        return -1;
    }
    return 0;
}

static int
_channelends_is_open(_channelends *ends)
{
    if (ends->numsendopen != 0 || ends->numrecvopen != 0) {
        return 1;
    }
    if (ends->send == NULL && ends->recv == NULL) {
        return 1;
    }
    return 0;
}

static void
_channelends_close_end(_channelends *ends, _channelend *end, int send)
{
    end->open = 0;
    if (send) {
        ends->numsendopen -= 1;
    }
    else {
        ends->numrecvopen -= 1;
    }
}

static int
_channelends_close_interpreter(_channelends *ends, int64_t interp, int which)
{
    _channelend *prev;
    _channelend *end;
    if (which >= 0) {  // send/both
        end = _channelend_find(ends->send, interp, &prev);
        if (end == NULL) {
            // never associated so add it
            end = _channelends_add(ends, prev, interp, 1);
            if (end == NULL) {
                return -1;
            }
        }
        _channelends_close_end(ends, end, 1);
    }
    if (which <= 0) {  // recv/both
        end = _channelend_find(ends->recv, interp, &prev);
        if (end == NULL) {
            // never associated so add it
            end = _channelends_add(ends, prev, interp, 0);
            if (end == NULL) {
                return -1;
            }
        }
        _channelends_close_end(ends, end, 0);
    }
    return 0;
}

static void
_channelends_close_all(_channelends *ends, int which, int force)
{
    // XXX Handle the ends.
    // XXX Handle force is True.

    // Ensure all the "send"-associated interpreters are closed.
    _channelend *end;
    for (end = ends->send; end != NULL; end = end->next) {
        _channelends_close_end(ends, end, 1);
    }

    // Ensure all the "recv"-associated interpreters are closed.
    for (end = ends->recv; end != NULL; end = end->next) {
        _channelends_close_end(ends, end, 0);
    }
}

/* channels */

struct _channel;
struct _channel_closing;
static void _channel_clear_closing(struct _channel *);
static void _channel_finish_closing(struct _channel *);

typedef struct _channel {
    PyThread_type_lock mutex;
    _channelqueue *queue;
    _channelends *ends;
    int open;
    struct _channel_closing *closing;
} _PyChannelState;

static _PyChannelState *
_channel_new(PyThread_type_lock mutex)
{
    _PyChannelState *chan = PyMem_NEW(_PyChannelState, 1);
    if (chan == NULL) {
        return NULL;
    }
    chan->mutex = mutex;
    chan->queue = _channelqueue_new();
    if (chan->queue == NULL) {
        PyMem_Free(chan);
        return NULL;
    }
    chan->ends = _channelends_new();
    if (chan->ends == NULL) {
        _channelqueue_free(chan->queue);
        PyMem_Free(chan);
        return NULL;
    }
    chan->open = 1;
    chan->closing = NULL;
    return chan;
}

static void
_channel_free(_PyChannelState *chan)
{
    _channel_clear_closing(chan);
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    _channelqueue_free(chan->queue);
    _channelends_free(chan->ends);
    PyThread_release_lock(chan->mutex);

    PyThread_free_lock(chan->mutex);
    PyMem_Free(chan);
}

static int
_channel_add(_PyChannelState *chan, int64_t interp,
             _PyCrossInterpreterData *data)
{
    int res = -1;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    if (!chan->open) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }
    if (_channelends_associate(chan->ends, interp, 1) != 0) {
        res = ERR_CHANNEL_INTERP_CLOSED;
        goto done;
    }

    if (_channelqueue_put(chan->queue, data) != 0) {
        goto done;
    }

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

static int
_channel_next(_PyChannelState *chan, int64_t interp,
              _PyCrossInterpreterData **res)
{
    int err = 0;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    if (!chan->open) {
        err = ERR_CHANNEL_CLOSED;
        goto done;
    }
    if (_channelends_associate(chan->ends, interp, 0) != 0) {
        err = ERR_CHANNEL_INTERP_CLOSED;
        goto done;
    }

    _PyCrossInterpreterData *data = _channelqueue_get(chan->queue);
    if (data == NULL && !PyErr_Occurred() && chan->closing != NULL) {
        chan->open = 0;
    }
    *res = data;

done:
    PyThread_release_lock(chan->mutex);
    if (chan->queue->count == 0) {
        _channel_finish_closing(chan);
    }
    return err;
}

static int
_channel_close_interpreter(_PyChannelState *chan, int64_t interp, int end)
{
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    int res = -1;
    if (!chan->open) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }

    if (_channelends_close_interpreter(chan->ends, interp, end) != 0) {
        goto done;
    }
    chan->open = _channelends_is_open(chan->ends);

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

static int
_channel_close_all(_PyChannelState *chan, int end, int force)
{
    int res = -1;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    if (!chan->open) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }

    if (!force && chan->queue->count > 0) {
        res = ERR_CHANNEL_NOT_EMPTY;
        goto done;
    }

    chan->open = 0;

    // We *could* also just leave these in place, since we've marked
    // the channel as closed already.
    _channelends_close_all(chan->ends, end, force);

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

/* the set of channels */

struct _channelref;

typedef struct _channelref {
    int64_t id;
    _PyChannelState *chan;
    struct _channelref *next;
    Py_ssize_t objcount;
} _channelref;

static _channelref *
_channelref_new(int64_t id, _PyChannelState *chan)
{
    _channelref *ref = PyMem_NEW(_channelref, 1);
    if (ref == NULL) {
        return NULL;
    }
    ref->id = id;
    ref->chan = chan;
    ref->next = NULL;
    ref->objcount = 0;
    return ref;
}

//static void
//_channelref_clear(_channelref *ref)
//{
//    ref->id = -1;
//    ref->chan = NULL;
//    ref->next = NULL;
//    ref->objcount = 0;
//}

static void
_channelref_free(_channelref *ref)
{
    if (ref->chan != NULL) {
        _channel_clear_closing(ref->chan);
    }
    //_channelref_clear(ref);
    PyMem_Free(ref);
}

static _channelref *
_channelref_find(_channelref *first, int64_t id, _channelref **pprev)
{
    _channelref *prev = NULL;
    _channelref *ref = first;
    while (ref != NULL) {
        if (ref->id == id) {
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

typedef struct _channels {
    PyThread_type_lock mutex;
    _channelref *head;
    int64_t numopen;
    int64_t next_id;
} _channels;

static void
_channels_init(_channels *channels, PyThread_type_lock mutex)
{
    channels->mutex = mutex;
    channels->head = NULL;
    channels->numopen = 0;
    channels->next_id = 0;
}

static void
_channels_fini(_channels *channels)
{
    assert(channels->numopen == 0);
    assert(channels->head == NULL);
    if (channels->mutex != NULL) {
        PyThread_free_lock(channels->mutex);
        channels->mutex = NULL;
    }
}

static int64_t
_channels_next_id(_channels *channels)  // needs lock
{
    int64_t id = channels->next_id;
    if (id < 0) {
        /* overflow */
        return -1;
    }
    channels->next_id += 1;
    return id;
}

static int
_channels_lookup(_channels *channels, int64_t id, PyThread_type_lock *pmutex,
                 _PyChannelState **res)
{
    int err = -1;
    _PyChannelState *chan = NULL;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    if (pmutex != NULL) {
        *pmutex = NULL;
    }

    _channelref *ref = _channelref_find(channels->head, id, NULL);
    if (ref == NULL) {
        err = ERR_CHANNEL_NOT_FOUND;
        goto done;
    }
    if (ref->chan == NULL || !ref->chan->open) {
        err = ERR_CHANNEL_CLOSED;
        goto done;
    }

    if (pmutex != NULL) {
        // The mutex will be closed by the caller.
        *pmutex = channels->mutex;
    }

    chan = ref->chan;
    err = 0;

done:
    if (pmutex == NULL || *pmutex == NULL) {
        PyThread_release_lock(channels->mutex);
    }
    *res = chan;
    return err;
}

static int64_t
_channels_add(_channels *channels, _PyChannelState *chan)
{
    int64_t cid = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    // Create a new ref.
    int64_t id = _channels_next_id(channels);
    if (id < 0) {
        cid = ERR_NO_NEXT_CHANNEL_ID;
        goto done;
    }
    _channelref *ref = _channelref_new(id, chan);
    if (ref == NULL) {
        goto done;
    }

    // Add it to the list.
    // We assume that the channel is a new one (not already in the list).
    ref->next = channels->head;
    channels->head = ref;
    channels->numopen += 1;

    cid = id;
done:
    PyThread_release_lock(channels->mutex);
    return cid;
}

/* forward */
static int _channel_set_closing(struct _channelref *, PyThread_type_lock);

static int
_channels_close(_channels *channels, int64_t cid, _PyChannelState **pchan,
                int end, int force)
{
    int res = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    if (pchan != NULL) {
        *pchan = NULL;
    }

    _channelref *ref = _channelref_find(channels->head, cid, NULL);
    if (ref == NULL) {
        res = ERR_CHANNEL_NOT_FOUND;
        goto done;
    }

    if (ref->chan == NULL) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }
    else if (!force && end == CHANNEL_SEND && ref->chan->closing != NULL) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }
    else {
        int err = _channel_close_all(ref->chan, end, force);
        if (err != 0) {
            if (end == CHANNEL_SEND && err == ERR_CHANNEL_NOT_EMPTY) {
                if (ref->chan->closing != NULL) {
                    res = ERR_CHANNEL_CLOSED;
                    goto done;
                }
                // Mark the channel as closing and return.  The channel
                // will be cleaned up in _channel_next().
                PyErr_Clear();
                int err = _channel_set_closing(ref, channels->mutex);
                if (err != 0) {
                    res = err;
                    goto done;
                }
                if (pchan != NULL) {
                    *pchan = ref->chan;
                }
                res = 0;
            }
            else {
                res = err;
            }
            goto done;
        }
        if (pchan != NULL) {
            *pchan = ref->chan;
        }
        else  {
            _channel_free(ref->chan);
        }
        ref->chan = NULL;
    }

    res = 0;
done:
    PyThread_release_lock(channels->mutex);
    return res;
}

static void
_channels_remove_ref(_channels *channels, _channelref *ref, _channelref *prev,
                     _PyChannelState **pchan)
{
    if (ref == channels->head) {
        channels->head = ref->next;
    }
    else {
        prev->next = ref->next;
    }
    channels->numopen -= 1;

    if (pchan != NULL) {
        *pchan = ref->chan;
    }
    _channelref_free(ref);
}

static int
_channels_remove(_channels *channels, int64_t id, _PyChannelState **pchan)
{
    int res = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    if (pchan != NULL) {
        *pchan = NULL;
    }

    _channelref *prev = NULL;
    _channelref *ref = _channelref_find(channels->head, id, &prev);
    if (ref == NULL) {
        res = ERR_CHANNEL_NOT_FOUND;
        goto done;
    }

    _channels_remove_ref(channels, ref, prev, pchan);

    res = 0;
done:
    PyThread_release_lock(channels->mutex);
    return res;
}

static int
_channels_add_id_object(_channels *channels, int64_t id)
{
    int res = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    _channelref *ref = _channelref_find(channels->head, id, NULL);
    if (ref == NULL) {
        res = ERR_CHANNEL_NOT_FOUND;
        goto done;
    }
    ref->objcount += 1;

    res = 0;
done:
    PyThread_release_lock(channels->mutex);
    return res;
}

static void
_channels_drop_id_object(_channels *channels, int64_t id)
{
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    _channelref *prev = NULL;
    _channelref *ref = _channelref_find(channels->head, id, &prev);
    if (ref == NULL) {
        // Already destroyed.
        goto done;
    }
    ref->objcount -= 1;

    // Destroy if no longer used.
    if (ref->objcount == 0) {
        _PyChannelState *chan = NULL;
        _channels_remove_ref(channels, ref, prev, &chan);
        if (chan != NULL) {
            _channel_free(chan);
        }
    }

done:
    PyThread_release_lock(channels->mutex);
}

static int64_t *
_channels_list_all(_channels *channels, int64_t *count)
{
    int64_t *cids = NULL;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    int64_t *ids = PyMem_NEW(int64_t, (Py_ssize_t)(channels->numopen));
    if (ids == NULL) {
        goto done;
    }
    _channelref *ref = channels->head;
    for (int64_t i=0; ref != NULL; ref = ref->next, i++) {
        ids[i] = ref->id;
    }
    *count = channels->numopen;

    cids = ids;
done:
    PyThread_release_lock(channels->mutex);
    return cids;
}

/* support for closing non-empty channels */

struct _channel_closing {
    struct _channelref *ref;
};

static int
_channel_set_closing(struct _channelref *ref, PyThread_type_lock mutex) {
    struct _channel *chan = ref->chan;
    if (chan == NULL) {
        // already closed
        return 0;
    }
    int res = -1;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    if (chan->closing != NULL) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }
    chan->closing = PyMem_NEW(struct _channel_closing, 1);
    if (chan->closing == NULL) {
        goto done;
    }
    chan->closing->ref = ref;

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

static void
_channel_clear_closing(struct _channel *chan) {
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    if (chan->closing != NULL) {
        PyMem_Free(chan->closing);
        chan->closing = NULL;
    }
    PyThread_release_lock(chan->mutex);
}

static void
_channel_finish_closing(struct _channel *chan) {
    struct _channel_closing *closing = chan->closing;
    if (closing == NULL) {
        return;
    }
    _channelref *ref = closing->ref;
    _channel_clear_closing(chan);
    // Do the things that would have been done in _channels_close().
    ref->chan = NULL;
    _channel_free(chan);
}

/* "high"-level channel-related functions */

static int64_t
_channel_create(_channels *channels)
{
    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        return ERR_CHANNEL_MUTEX_INIT;
    }
    _PyChannelState *chan = _channel_new(mutex);
    if (chan == NULL) {
        PyThread_free_lock(mutex);
        return -1;
    }
    int64_t id = _channels_add(channels, chan);
    if (id < 0) {
        _channel_free(chan);
    }
    return id;
}

static int
_channel_destroy(_channels *channels, int64_t id)
{
    _PyChannelState *chan = NULL;
    int err = _channels_remove(channels, id, &chan);
    if (err != 0) {
        return err;
    }
    if (chan != NULL) {
        _channel_free(chan);
    }
    return 0;
}

static int
_channel_send(_channels *channels, int64_t id, PyObject *obj)
{
    PyInterpreterState *interp = _get_current_interp();
    if (interp == NULL) {
        return -1;
    }

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = NULL;
    int err = _channels_lookup(channels, id, &mutex, &chan);
    if (err != 0) {
        return err;
    }
    assert(chan != NULL);
    // Past this point we are responsible for releasing the mutex.

    if (chan->closing != NULL) {
        PyThread_release_lock(mutex);
        return ERR_CHANNEL_CLOSED;
    }

    // Convert the object to cross-interpreter data.
    _PyCrossInterpreterData *data = PyMem_NEW(_PyCrossInterpreterData, 1);
    if (data == NULL) {
        PyThread_release_lock(mutex);
        return -1;
    }
    if (_PyObject_GetCrossInterpreterData(obj, data) != 0) {
        PyThread_release_lock(mutex);
        PyMem_Free(data);
        return -1;
    }

    // Add the data to the channel.
    int res = _channel_add(chan, PyInterpreterState_GetID(interp), data);
    PyThread_release_lock(mutex);
    if (res != 0) {
        // We may chain an exception here:
        (void)_release_xid_data(data, 0);
        PyMem_Free(data);
        return res;
    }

    return 0;
}

static int
_channel_recv(_channels *channels, int64_t id, PyObject **res)
{
    int err;
    *res = NULL;

    PyInterpreterState *interp = _get_current_interp();
    if (interp == NULL) {
        // XXX Is this always an error?
        if (PyErr_Occurred()) {
            return -1;
        }
        return 0;
    }

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = NULL;
    err = _channels_lookup(channels, id, &mutex, &chan);
    if (err != 0) {
        return err;
    }
    assert(chan != NULL);
    // Past this point we are responsible for releasing the mutex.

    // Pop off the next item from the channel.
    _PyCrossInterpreterData *data = NULL;
    err = _channel_next(chan, PyInterpreterState_GetID(interp), &data);
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
        (void)_release_xid_data(data, 1);
        PyMem_Free(data);
        return -1;
    }
    int release_res = _release_xid_data(data, 0);
    PyMem_Free(data);
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
_channel_drop(_channels *channels, int64_t id, int send, int recv)
{
    PyInterpreterState *interp = _get_current_interp();
    if (interp == NULL) {
        return -1;
    }

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = NULL;
    int err = _channels_lookup(channels, id, &mutex, &chan);
    if (err != 0) {
        return err;
    }
    // Past this point we are responsible for releasing the mutex.

    // Close one or both of the two ends.
    int res = _channel_close_interpreter(chan, PyInterpreterState_GetID(interp), send-recv);
    PyThread_release_lock(mutex);
    return res;
}

static int
_channel_close(_channels *channels, int64_t id, int end, int force)
{
    return _channels_close(channels, id, NULL, end, force);
}

static int
_channel_is_associated(_channels *channels, int64_t cid, int64_t interp,
                       int send)
{
    _PyChannelState *chan = NULL;
    int err = _channels_lookup(channels, cid, NULL, &chan);
    if (err != 0) {
        return err;
    }
    else if (send && chan->closing != NULL) {
        return ERR_CHANNEL_CLOSED;
    }

    _channelend *end = _channelend_find(send ? chan->ends->send : chan->ends->recv,
                                        interp, NULL);

    return (end != NULL && end->open);
}

/* ChannelID class */

static PyTypeObject ChannelIDType;

typedef struct channelid {
    PyObject_HEAD
    int64_t id;
    int end;
    int resolve;
    _channels *channels;
} channelid;

struct channel_id_converter_data {
    PyObject *module;
    int64_t cid;
};

static int
channel_id_converter(PyObject *arg, void *ptr)
{
    int64_t cid;
    struct channel_id_converter_data *data = ptr;
    if (PyObject_TypeCheck(arg, &ChannelIDType)) {
        cid = ((channelid *)arg)->id;
    }
    else if (PyIndex_Check(arg)) {
        cid = PyLong_AsLongLong(arg);
        if (cid == -1 && PyErr_Occurred()) {
            return 0;
        }
        if (cid < 0) {
            PyErr_Format(PyExc_ValueError,
                        "channel ID must be a non-negative int, got %R", arg);
            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "channel ID must be an int, got %.100s",
                     Py_TYPE(arg)->tp_name);
        return 0;
    }
    data->cid = cid;
    return 1;
}

static int
newchannelid(PyTypeObject *cls, int64_t cid, int end, _channels *channels,
             int force, int resolve, channelid **res)
{
    *res = NULL;

    channelid *self = PyObject_New(channelid, cls);
    if (self == NULL) {
        return -1;
    }
    self->id = cid;
    self->end = end;
    self->resolve = resolve;
    self->channels = channels;

    int err = _channels_add_id_object(channels, cid);
    if (err != 0) {
        if (force && err == ERR_CHANNEL_NOT_FOUND) {
            assert(!PyErr_Occurred());
        }
        else {
            Py_DECREF((PyObject *)self);
            return err;
        }
    }

    *res = self;
    return 0;
}

static _channels * _global_channels(void);

static PyObject *
_channelid_new(PyObject *mod, PyTypeObject *cls,
               PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "send", "recv", "force", "_resolve", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = mod,
    };
    int send = -1;
    int recv = -1;
    int force = 0;
    int resolve = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&|$pppp:ChannelID.__new__", kwlist,
                                     channel_id_converter, &cid_data,
                                     &send, &recv, &force, &resolve)) {
        return NULL;
    }
    cid = cid_data.cid;

    // Handle "send" and "recv".
    if (send == 0 && recv == 0) {
        PyErr_SetString(PyExc_ValueError,
                        "'send' and 'recv' cannot both be False");
        return NULL;
    }

    int end = 0;
    if (send == 1) {
        if (recv == 0 || recv == -1) {
            end = CHANNEL_SEND;
        }
    }
    else if (recv == 1) {
        end = CHANNEL_RECV;
    }

    PyObject *id = NULL;
    int err = newchannelid(cls, cid, end, _global_channels(),
                           force, resolve,
                           (channelid **)&id);
    if (handle_channel_error(err, mod, cid)) {
        assert(id == NULL);
        return NULL;
    }
    assert(id != NULL);
    return id;
}

static void
channelid_dealloc(PyObject *v)
{
    int64_t cid = ((channelid *)v)->id;
    _channels *channels = ((channelid *)v)->channels;
    Py_TYPE(v)->tp_free(v);

    _channels_drop_id_object(channels, cid);
}

static PyObject *
channelid_repr(PyObject *self)
{
    PyTypeObject *type = Py_TYPE(self);
    const char *name = _PyType_Name(type);

    channelid *cid = (channelid *)self;
    const char *fmt;
    if (cid->end == CHANNEL_SEND) {
        fmt = "%s(%" PRId64 ", send=True)";
    }
    else if (cid->end == CHANNEL_RECV) {
        fmt = "%s(%" PRId64 ", recv=True)";
    }
    else {
        fmt = "%s(%" PRId64 ")";
    }
    return PyUnicode_FromFormat(fmt, name, cid->id);
}

static PyObject *
channelid_str(PyObject *self)
{
    channelid *cid = (channelid *)self;
    return PyUnicode_FromFormat("%" PRId64 "", cid->id);
}

static PyObject *
channelid_int(PyObject *self)
{
    channelid *cid = (channelid *)self;
    return PyLong_FromLongLong(cid->id);
}

static PyNumberMethods channelid_as_number = {
    .nb_int = (unaryfunc)channelid_int, /* nb_int */
    .nb_index = (unaryfunc)channelid_int, /* nb_index */
};

static Py_hash_t
channelid_hash(PyObject *self)
{
    channelid *cid = (channelid *)self;
    PyObject *id = PyLong_FromLongLong(cid->id);
    if (id == NULL) {
        return -1;
    }
    Py_hash_t hash = PyObject_Hash(id);
    Py_DECREF(id);
    return hash;
}

static PyObject *
channelid_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *res = NULL;
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    PyObject *mod = get_module_from_type(Py_TYPE(self));
    if (mod == NULL) {
        return NULL;
    }

    if (!PyObject_TypeCheck(self, &ChannelIDType)) {
        res = Py_NewRef(Py_NotImplemented);
        goto done;
    }

    channelid *cid = (channelid *)self;
    int equal;
    if (PyObject_TypeCheck(other, &ChannelIDType)) {
        channelid *othercid = (channelid *)other;
        equal = (cid->end == othercid->end) && (cid->id == othercid->id);
    }
    else if (PyLong_Check(other)) {
        /* Fast path */
        int overflow;
        long long othercid = PyLong_AsLongLongAndOverflow(other, &overflow);
        if (othercid == -1 && PyErr_Occurred()) {
            goto done;
        }
        equal = !overflow && (othercid >= 0) && (cid->id == othercid);
    }
    else if (PyNumber_Check(other)) {
        PyObject *pyid = PyLong_FromLongLong(cid->id);
        if (pyid == NULL) {
            goto done;
        }
        res = PyObject_RichCompare(pyid, other, op);
        Py_DECREF(pyid);
        goto done;
    }
    else {
        res = Py_NewRef(Py_NotImplemented);
        goto done;
    }

    if ((op == Py_EQ && equal) || (op == Py_NE && !equal)) {
        res = Py_NewRef(Py_True);
    }
    else {
        res = Py_NewRef(Py_False);
    }

done:
    Py_DECREF(mod);
    return res;
}

static PyObject *
_channel_from_cid(PyObject *cid, int end)
{
    PyObject *highlevel = PyImport_ImportModule("interpreters");
    if (highlevel == NULL) {
        PyErr_Clear();
        highlevel = PyImport_ImportModule("test.support.interpreters");
        if (highlevel == NULL) {
            return NULL;
        }
    }
    const char *clsname = (end == CHANNEL_RECV) ? "RecvChannel" :
                                                  "SendChannel";
    PyObject *cls = PyObject_GetAttrString(highlevel, clsname);
    Py_DECREF(highlevel);
    if (cls == NULL) {
        return NULL;
    }
    PyObject *chan = PyObject_CallFunctionObjArgs(cls, cid, NULL);
    Py_DECREF(cls);
    if (chan == NULL) {
        return NULL;
    }
    return chan;
}

struct _channelid_xid {
    int64_t id;
    int end;
    int resolve;
};

static PyObject *
_channelid_from_xid(_PyCrossInterpreterData *data)
{
    struct _channelid_xid *xid = (struct _channelid_xid *)data->data;

    PyObject *mod = _get_current_module();
    if (mod == NULL) {
        return NULL;
    }

    // Note that we do not preserve the "resolve" flag.
    PyObject *cid = NULL;
    int err = newchannelid(&ChannelIDType, xid->id, xid->end,
                           _global_channels(), 0, 0,
                           (channelid **)&cid);
    if (err != 0) {
        assert(cid == NULL);
        (void)handle_channel_error(err, mod, xid->id);
        goto done;
    }
    assert(cid != NULL);
    if (xid->end == 0) {
        goto done;
    }
    if (!xid->resolve) {
        goto done;
    }

    /* Try returning a high-level channel end but fall back to the ID. */
    PyObject *chan = _channel_from_cid(cid, xid->end);
    if (chan == NULL) {
        PyErr_Clear();
        goto done;
    }
    Py_DECREF(cid);
    cid = chan;

done:
    Py_DECREF(mod);
    return cid;
}

static int
_channelid_shared(PyObject *obj, _PyCrossInterpreterData *data)
{
    struct _channelid_xid *xid = PyMem_NEW(struct _channelid_xid, 1);
    if (xid == NULL) {
        return -1;
    }
    xid->id = ((channelid *)obj)->id;
    xid->end = ((channelid *)obj)->end;
    xid->resolve = ((channelid *)obj)->resolve;

    data->data = xid;
    data->obj = Py_NewRef(obj);
    data->new_object = _channelid_from_xid;
    data->free = PyMem_Free;
    return 0;
}

static PyObject *
channelid_end(PyObject *self, void *end)
{
    int force = 1;
    channelid *cid = (channelid *)self;
    if (end != NULL) {
        PyObject *id = NULL;
        int err = newchannelid(Py_TYPE(self), cid->id, *(int *)end,
                               cid->channels, force, cid->resolve,
                               (channelid **)&id);
        if (err != 0) {
            assert(id == NULL);
            PyObject *mod = get_module_from_type(Py_TYPE(self));
            if (mod == NULL) {
                return NULL;
            }
            (void)handle_channel_error(err, mod, cid->id);
            Py_DECREF(mod);
            return NULL;
        }
        assert(id != NULL);
        return id;
    }

    if (cid->end == CHANNEL_SEND) {
        return PyUnicode_InternFromString("send");
    }
    if (cid->end == CHANNEL_RECV) {
        return PyUnicode_InternFromString("recv");
    }
    return PyUnicode_InternFromString("both");
}

static int _channelid_end_send = CHANNEL_SEND;
static int _channelid_end_recv = CHANNEL_RECV;

static PyGetSetDef channelid_getsets[] = {
    {"end", (getter)channelid_end, NULL,
     PyDoc_STR("'send', 'recv', or 'both'")},
    {"send", (getter)channelid_end, NULL,
     PyDoc_STR("the 'send' end of the channel"), &_channelid_end_send},
    {"recv", (getter)channelid_end, NULL,
     PyDoc_STR("the 'recv' end of the channel"), &_channelid_end_recv},
    {NULL}
};

PyDoc_STRVAR(channelid_doc,
"A channel ID identifies a channel and may be used as an int.");

static PyTypeObject ChannelIDType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "_xxsubinterpreters.ChannelID", /* tp_name */
    sizeof(channelid),              /* tp_basicsize */
    0,                              /* tp_itemsize */
    (destructor)channelid_dealloc,  /* tp_dealloc */
    0,                              /* tp_vectorcall_offset */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_as_async */
    (reprfunc)channelid_repr,       /* tp_repr */
    &channelid_as_number,           /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    channelid_hash,                 /* tp_hash */
    0,                              /* tp_call */
    (reprfunc)channelid_str,        /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    // Use Py_TPFLAGS_DISALLOW_INSTANTIATION so the type cannot be instantiated
    // from Python code.  We do this because there is a strong relationship
    // between channel IDs and the channel lifecycle, so this limitation avoids
    // related complications. Use the _channel_id() function instead.
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_DISALLOW_INSTANTIATION, /* tp_flags */
    channelid_doc,                  /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    channelid_richcompare,          /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    0,                              /* tp_methods */
    0,                              /* tp_members */
    channelid_getsets,              /* tp_getset */
};


/* interpreter-specific code ************************************************/

static PyObject * RunFailedError = NULL;

static int
interp_exceptions_init(PyObject *mod)
{
    // XXX Move the exceptions into per-module memory?

#define ADD(NAME, BASE) \
    do { \
        if (NAME == NULL) { \
            NAME = ADD_NEW_EXCEPTION(mod, NAME, BASE); \
            if (NAME == NULL) { \
                return -1; \
            } \
        } \
    } while (0)

    // An uncaught exception came out of interp_run_string().
    ADD(RunFailedError, PyExc_RuntimeError);
#undef ADD

    return 0;
}

static int
_is_running(PyInterpreterState *interp)
{
    PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
    if (PyThreadState_Next(tstate) != NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "interpreter has more than one thread");
        return -1;
    }

    assert(!PyErr_Occurred());
    _PyInterpreterFrame *frame = tstate->cframe->current_frame;
    if (frame == NULL) {
        return 0;
    }
    return 1;
}

static int
_ensure_not_running(PyInterpreterState *interp)
{
    int is_running = _is_running(interp);
    if (is_running < 0) {
        return -1;
    }
    if (is_running) {
        PyErr_Format(PyExc_RuntimeError, "interpreter already running");
        return -1;
    }
    return 0;
}

static int
_run_script(PyInterpreterState *interp, const char *codestr,
            _sharedns *shared, int needs_import,
            _sharedexception **exc)
{
    PyObject *exctype = NULL;
    PyObject *excval = NULL;
    PyObject *tb = NULL;

    if (needs_import) {
        // It might not have been imported yet in the current interpreter.
        // However, it will (almost) always have been imported already
        // in the main interpreter.
        PyObject *mod = PyImport_ImportModule(MODULE_NAME);
        if (mod == NULL) {
            goto error;
        }
        Py_DECREF(mod);
    }

    PyObject *main_mod = _PyInterpreterState_GetMainModule(interp);
    if (main_mod == NULL) {
        goto error;
    }
    PyObject *ns = PyModule_GetDict(main_mod);  // borrowed
    Py_DECREF(main_mod);
    if (ns == NULL) {
        goto error;
    }
    Py_INCREF(ns);

    // Apply the cross-interpreter data.
    if (shared != NULL) {
        if (_sharedns_apply(shared, ns) != 0) {
            Py_DECREF(ns);
            goto error;
        }
    }

    // Run the string (see PyRun_SimpleStringFlags).
    PyObject *result = PyRun_StringFlags(codestr, Py_file_input, ns, ns, NULL);
    Py_DECREF(ns);
    if (result == NULL) {
        goto error;
    }
    else {
        Py_DECREF(result);  // We throw away the result.
    }

    *exc = NULL;
    return 0;

error:
    PyErr_Fetch(&exctype, &excval, &tb);

    _sharedexception *sharedexc = _sharedexception_bind(exctype, excval, tb);
    Py_XDECREF(exctype);
    Py_XDECREF(excval);
    Py_XDECREF(tb);
    if (sharedexc == NULL) {
        fprintf(stderr, "RunFailedError: script raised an uncaught exception");
        PyErr_Clear();
        sharedexc = NULL;
    }
    else {
        assert(!PyErr_Occurred());
    }
    *exc = sharedexc;
    return -1;
}

static int
_run_script_in_interpreter(PyObject *mod, PyInterpreterState *interp,
                           const char *codestr, PyObject *shareables)
{
    if (_ensure_not_running(interp) < 0) {
        return -1;
    }

    int needs_import = 0;
    _sharedns *shared = _get_shared_ns(shareables, &ChannelIDType,
                                       &needs_import);
    if (shared == NULL && PyErr_Occurred()) {
        return -1;
    }

    // Switch to interpreter.
    PyThreadState *save_tstate = NULL;
    if (interp != PyInterpreterState_Get()) {
        // XXX Using the "head" thread isn't strictly correct.
        PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
        // XXX Possible GILState issues?
        save_tstate = PyThreadState_Swap(tstate);
    }

    // Run the script.
    _sharedexception *exc = NULL;
    int result = _run_script(interp, codestr, shared, needs_import, &exc);

    // Switch back.
    if (save_tstate != NULL) {
        PyThreadState_Swap(save_tstate);
    }

    // Propagate any exception out to the caller.
    if (exc != NULL) {
        _sharedexception_apply(exc, RunFailedError);
        _sharedexception_free(exc);
    }
    else if (result != 0) {
        // We were unable to allocate a shared exception.
        PyErr_NoMemory();
    }

    if (shared != NULL) {
        _sharedns_free(shared);
    }

    return result;
}


/* module level code ********************************************************/

/* globals is the process-global state for the module.  It holds all
   the data that we need to share between interpreters, so it cannot
   hold PyObject values. */
static struct globals {
    int module_count;
    _channels channels;
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

    assert(_globals.channels.mutex == NULL);
    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        return ERR_CHANNELS_MUTEX_INIT;
    }
    _channels_init(&_globals.channels, mutex);
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

    _channels_fini(&_globals.channels);
}

static _channels *
_global_channels(void) {
    return &_globals.channels;
}

static PyObject *
interp_create(PyObject *self, PyObject *args, PyObject *kwds)
{

    static char *kwlist[] = {"isolated", NULL};
    int isolated = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|$i:create", kwlist,
                                     &isolated)) {
        return NULL;
    }

    // Create and initialize the new interpreter.
    PyThreadState *save_tstate = _PyThreadState_GET();
    const _PyInterpreterConfig config = isolated
        ? (_PyInterpreterConfig)_PyInterpreterConfig_INIT
        : (_PyInterpreterConfig)_PyInterpreterConfig_LEGACY_INIT;
    // XXX Possible GILState issues?
    PyThreadState *tstate = _Py_NewInterpreterFromConfig(&config);
    PyThreadState_Swap(save_tstate);
    if (tstate == NULL) {
        /* Since no new thread state was created, there is no exception to
           propagate; raise a fresh one after swapping in the old thread
           state. */
        PyErr_SetString(PyExc_RuntimeError, "interpreter creation failed");
        return NULL;
    }
    PyInterpreterState *interp = PyThreadState_GetInterpreter(tstate);
    PyObject *idobj = _PyInterpreterState_GetIDObject(interp);
    if (idobj == NULL) {
        // XXX Possible GILState issues?
        save_tstate = PyThreadState_Swap(tstate);
        Py_EndInterpreter(tstate);
        PyThreadState_Swap(save_tstate);
        return NULL;
    }
    _PyInterpreterState_RequireIDRef(interp, 1);
    return idobj;
}

PyDoc_STRVAR(create_doc,
"create() -> ID\n\
\n\
Create a new interpreter and return a unique generated ID.");


static PyObject *
interp_destroy(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", NULL};
    PyObject *id;
    // XXX Use "L" for id?
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:destroy", kwlist, &id)) {
        return NULL;
    }

    // Look up the interpreter.
    PyInterpreterState *interp = _PyInterpreterID_LookUp(id);
    if (interp == NULL) {
        return NULL;
    }

    // Ensure we don't try to destroy the current interpreter.
    PyInterpreterState *current = _get_current_interp();
    if (current == NULL) {
        return NULL;
    }
    if (interp == current) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot destroy the current interpreter");
        return NULL;
    }

    // Ensure the interpreter isn't running.
    /* XXX We *could* support destroying a running interpreter but
       aren't going to worry about it for now. */
    if (_ensure_not_running(interp) < 0) {
        return NULL;
    }

    // Destroy the interpreter.
    PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
    // XXX Possible GILState issues?
    PyThreadState *save_tstate = PyThreadState_Swap(tstate);
    Py_EndInterpreter(tstate);
    PyThreadState_Swap(save_tstate);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(destroy_doc,
"destroy(id)\n\
\n\
Destroy the identified interpreter.\n\
\n\
Attempting to destroy the current interpreter results in a RuntimeError.\n\
So does an unrecognized ID.");


static PyObject *
interp_list_all(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *ids, *id;
    PyInterpreterState *interp;

    ids = PyList_New(0);
    if (ids == NULL) {
        return NULL;
    }

    interp = PyInterpreterState_Head();
    while (interp != NULL) {
        id = _PyInterpreterState_GetIDObject(interp);
        if (id == NULL) {
            Py_DECREF(ids);
            return NULL;
        }
        // insert at front of list
        int res = PyList_Insert(ids, 0, id);
        Py_DECREF(id);
        if (res < 0) {
            Py_DECREF(ids);
            return NULL;
        }

        interp = PyInterpreterState_Next(interp);
    }

    return ids;
}

PyDoc_STRVAR(list_all_doc,
"list_all() -> [ID]\n\
\n\
Return a list containing the ID of every existing interpreter.");


static PyObject *
interp_get_current(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyInterpreterState *interp =_get_current_interp();
    if (interp == NULL) {
        return NULL;
    }
    return _PyInterpreterState_GetIDObject(interp);
}

PyDoc_STRVAR(get_current_doc,
"get_current() -> ID\n\
\n\
Return the ID of current interpreter.");


static PyObject *
interp_get_main(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // Currently, 0 is always the main interpreter.
    int64_t id = 0;
    return _PyInterpreterID_New(id);
}

PyDoc_STRVAR(get_main_doc,
"get_main() -> ID\n\
\n\
Return the ID of main interpreter.");


static PyObject *
interp_run_string(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "script", "shared", NULL};
    PyObject *id, *code;
    PyObject *shared = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OU|O:run_string", kwlist,
                                     &id, &code, &shared)) {
        return NULL;
    }

    // Look up the interpreter.
    PyInterpreterState *interp = _PyInterpreterID_LookUp(id);
    if (interp == NULL) {
        return NULL;
    }

    // Extract code.
    Py_ssize_t size;
    const char *codestr = PyUnicode_AsUTF8AndSize(code, &size);
    if (codestr == NULL) {
        return NULL;
    }
    if (strlen(codestr) != (size_t)size) {
        PyErr_SetString(PyExc_ValueError,
                        "source code string cannot contain null bytes");
        return NULL;
    }

    // Run the code in the interpreter.
    if (_run_script_in_interpreter(self, interp, codestr, shared) != 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(run_string_doc,
"run_string(id, script, shared)\n\
\n\
Execute the provided string in the identified interpreter.\n\
\n\
See PyRun_SimpleStrings.");


static PyObject *
object_is_shareable(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"obj", NULL};
    PyObject *obj;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:is_shareable", kwlist, &obj)) {
        return NULL;
    }

    if (_PyObject_CheckCrossInterpreterData(obj) == 0) {
        Py_RETURN_TRUE;
    }
    PyErr_Clear();
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(is_shareable_doc,
"is_shareable(obj) -> bool\n\
\n\
Return True if the object's data may be shared between interpreters and\n\
False otherwise.");


static PyObject *
interp_is_running(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", NULL};
    PyObject *id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O:is_running", kwlist, &id)) {
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterID_LookUp(id);
    if (interp == NULL) {
        return NULL;
    }
    int is_running = _is_running(interp);
    if (is_running < 0) {
        return NULL;
    }
    if (is_running) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(is_running_doc,
"is_running(id) -> bool\n\
\n\
Return whether or not the identified interpreter is running.");

static PyObject *
channel_create(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int64_t cid = _channel_create(&_globals.channels);
    if (cid < 0) {
        (void)handle_channel_error(cid, self, -1);
        return NULL;
    }
    PyObject *id = NULL;
    int err = newchannelid(&ChannelIDType, cid, 0,
                           &_globals.channels, 0, 0,
                           (channelid **)&id);
    if (handle_channel_error(err, self, cid)) {
        assert(id == NULL);
        err = _channel_destroy(&_globals.channels, cid);
        if (handle_channel_error(err, self, cid)) {
            // XXX issue a warning?
        }
        return NULL;
    }
    assert(id != NULL);
    assert(((channelid *)id)->channels != NULL);
    return id;
}

PyDoc_STRVAR(channel_create_doc,
"channel_create() -> cid\n\
\n\
Create a new cross-interpreter channel and return a unique generated ID.");

static PyObject *
channel_destroy(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:channel_destroy", kwlist,
                                     channel_id_converter, &cid_data)) {
        return NULL;
    }
    cid = cid_data.cid;

    int err = _channel_destroy(&_globals.channels, cid);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_destroy_doc,
"channel_destroy(cid)\n\
\n\
Close and finalize the channel.  Afterward attempts to use the channel\n\
will behave as though it never existed.");

static PyObject *
channel_list_all(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int64_t count = 0;
    int64_t *cids = _channels_list_all(&_globals.channels, &count);
    if (cids == NULL) {
        if (count == 0) {
            return PyList_New(0);
        }
        return NULL;
    }
    PyObject *ids = PyList_New((Py_ssize_t)count);
    if (ids == NULL) {
        goto finally;
    }
    int64_t *cur = cids;
    for (int64_t i=0; i < count; cur++, i++) {
        PyObject *id = NULL;
        int err = newchannelid(&ChannelIDType, *cur, 0,
                               &_globals.channels, 0, 0,
                               (channelid **)&id);
        if (handle_channel_error(err, self, *cur)) {
            assert(id == NULL);
            Py_SETREF(ids, NULL);
            break;
        }
        assert(id != NULL);
        PyList_SET_ITEM(ids, (Py_ssize_t)i, id);
    }

finally:
    PyMem_Free(cids);
    return ids;
}

PyDoc_STRVAR(channel_list_all_doc,
"channel_list_all() -> [cid]\n\
\n\
Return the list of all IDs for active channels.");

static PyObject *
channel_list_interpreters(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", "send", NULL};
    int64_t cid;            /* Channel ID */
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    int send = 0;           /* Send or receive end? */
    int64_t id;
    PyObject *ids, *id_obj;
    PyInterpreterState *interp;

    if (!PyArg_ParseTupleAndKeywords(
            args, kwds, "O&$p:channel_list_interpreters",
            kwlist, channel_id_converter, &cid_data, &send)) {
        return NULL;
    }
    cid = cid_data.cid;

    ids = PyList_New(0);
    if (ids == NULL) {
        goto except;
    }

    interp = PyInterpreterState_Head();
    while (interp != NULL) {
        id = PyInterpreterState_GetID(interp);
        assert(id >= 0);
        int res = _channel_is_associated(&_globals.channels, cid, id, send);
        if (res < 0) {
            (void)handle_channel_error(res, self, cid);
            goto except;
        }
        if (res) {
            id_obj = _PyInterpreterState_GetIDObject(interp);
            if (id_obj == NULL) {
                goto except;
            }
            res = PyList_Insert(ids, 0, id_obj);
            Py_DECREF(id_obj);
            if (res < 0) {
                goto except;
            }
        }
        interp = PyInterpreterState_Next(interp);
    }

    goto finally;

except:
    Py_CLEAR(ids);

finally:
    return ids;
}

PyDoc_STRVAR(channel_list_interpreters_doc,
"channel_list_interpreters(cid, *, send) -> [id]\n\
\n\
Return the list of all interpreter IDs associated with an end of the channel.\n\
\n\
The 'send' argument should be a boolean indicating whether to use the send or\n\
receive end.");


static PyObject *
channel_send(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", "obj", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    PyObject *obj;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O:channel_send", kwlist,
                                     channel_id_converter, &cid_data, &obj)) {
        return NULL;
    }
    cid = cid_data.cid;

    int err = _channel_send(&_globals.channels, cid, obj);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_send_doc,
"channel_send(cid, obj)\n\
\n\
Add the object's data to the channel's queue.");

static PyObject *
channel_recv(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", "default", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    PyObject *dflt = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&|O:channel_recv", kwlist,
                                     channel_id_converter, &cid_data, &dflt)) {
        return NULL;
    }
    cid = cid_data.cid;

    PyObject *obj = NULL;
    int err = _channel_recv(&_globals.channels, cid, &obj);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }
    Py_XINCREF(dflt);
    if (obj == NULL) {
        // Use the default.
        if (dflt == NULL) {
            (void)handle_channel_error(ERR_CHANNEL_EMPTY, self, cid);
            return NULL;
        }
        obj = Py_NewRef(dflt);
    }
    Py_XDECREF(dflt);
    return obj;
}

PyDoc_STRVAR(channel_recv_doc,
"channel_recv(cid, [default]) -> obj\n\
\n\
Return a new object from the data at the front of the channel's queue.\n\
\n\
If there is nothing to receive then raise ChannelEmptyError, unless\n\
a default value is provided.  In that case return it.");

static PyObject *
channel_close(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", "send", "recv", "force", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    int send = 0;
    int recv = 0;
    int force = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&|$ppp:channel_close", kwlist,
                                     channel_id_converter, &cid_data,
                                     &send, &recv, &force)) {
        return NULL;
    }
    cid = cid_data.cid;

    int err = _channel_close(&_globals.channels, cid, send-recv, force);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_close_doc,
"channel_close(cid, *, send=None, recv=None, force=False)\n\
\n\
Close the channel for all interpreters.\n\
\n\
If the channel is empty then the keyword args are ignored and both\n\
ends are immediately closed.  Otherwise, if 'force' is True then\n\
all queued items are released and both ends are immediately\n\
closed.\n\
\n\
If the channel is not empty *and* 'force' is False then following\n\
happens:\n\
\n\
 * recv is True (regardless of send):\n\
   - raise ChannelNotEmptyError\n\
 * recv is None and send is None:\n\
   - raise ChannelNotEmptyError\n\
 * send is True and recv is not True:\n\
   - fully close the 'send' end\n\
   - close the 'recv' end to interpreters not already receiving\n\
   - fully close it once empty\n\
\n\
Closing an already closed channel results in a ChannelClosedError.\n\
\n\
Once the channel's ID has no more ref counts in any interpreter\n\
the channel will be destroyed.");

static PyObject *
channel_release(PyObject *self, PyObject *args, PyObject *kwds)
{
    // Note that only the current interpreter is affected.
    static char *kwlist[] = {"cid", "send", "recv", "force", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    int send = 0;
    int recv = 0;
    int force = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&|$ppp:channel_release", kwlist,
                                     channel_id_converter, &cid_data,
                                     &send, &recv, &force)) {
        return NULL;
    }
    cid = cid_data.cid;
    if (send == 0 && recv == 0) {
        send = 1;
        recv = 1;
    }

    // XXX Handle force is True.
    // XXX Fix implicit release.

    int err = _channel_drop(&_globals.channels, cid, send, recv);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_release_doc,
"channel_release(cid, *, send=None, recv=None, force=True)\n\
\n\
Close the channel for the current interpreter.  'send' and 'recv'\n\
(bool) may be used to indicate the ends to close.  By default both\n\
ends are closed.  Closing an already closed end is a noop.");

static PyObject *
channel__channel_id(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyTypeObject *cls = &ChannelIDType;
    PyObject *mod = get_module_from_owned_type(cls);
    if (mod == NULL) {
        return NULL;
    }
    PyObject *cid = _channelid_new(mod, cls, args, kwds);
    Py_DECREF(mod);
    return cid;
}

static PyMethodDef module_functions[] = {
    {"create",                    _PyCFunction_CAST(interp_create),
     METH_VARARGS | METH_KEYWORDS, create_doc},
    {"destroy",                   _PyCFunction_CAST(interp_destroy),
     METH_VARARGS | METH_KEYWORDS, destroy_doc},
    {"list_all",                  interp_list_all,
     METH_NOARGS, list_all_doc},
    {"get_current",               interp_get_current,
     METH_NOARGS, get_current_doc},
    {"get_main",                  interp_get_main,
     METH_NOARGS, get_main_doc},
    {"is_running",                _PyCFunction_CAST(interp_is_running),
     METH_VARARGS | METH_KEYWORDS, is_running_doc},
    {"run_string",                _PyCFunction_CAST(interp_run_string),
     METH_VARARGS | METH_KEYWORDS, run_string_doc},

    {"is_shareable",              _PyCFunction_CAST(object_is_shareable),
     METH_VARARGS | METH_KEYWORDS, is_shareable_doc},

    {"channel_create",            channel_create,
     METH_NOARGS, channel_create_doc},
    {"channel_destroy",           _PyCFunction_CAST(channel_destroy),
     METH_VARARGS | METH_KEYWORDS, channel_destroy_doc},
    {"channel_list_all",          channel_list_all,
     METH_NOARGS, channel_list_all_doc},
    {"channel_list_interpreters", _PyCFunction_CAST(channel_list_interpreters),
     METH_VARARGS | METH_KEYWORDS, channel_list_interpreters_doc},
    {"channel_send",              _PyCFunction_CAST(channel_send),
     METH_VARARGS | METH_KEYWORDS, channel_send_doc},
    {"channel_recv",              _PyCFunction_CAST(channel_recv),
     METH_VARARGS | METH_KEYWORDS, channel_recv_doc},
    {"channel_close",             _PyCFunction_CAST(channel_close),
     METH_VARARGS | METH_KEYWORDS, channel_close_doc},
    {"channel_release",           _PyCFunction_CAST(channel_release),
     METH_VARARGS | METH_KEYWORDS, channel_release_doc},
    {"_channel_id",               _PyCFunction_CAST(channel__channel_id),
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
    if (_globals_init() != 0) {
        return -1;
    }

    /* Add exception types */
    if (interp_exceptions_init(mod) != 0) {
        goto error;
    }
    if (channel_exceptions_init(mod) != 0) {
        goto error;
    }

    /* Add other types */
    if (add_new_type(mod, &ChannelIDType, _channelid_shared) == NULL) {
        goto error;
    }
    if (PyModule_AddType(mod, &_PyInterpreterID_Type) < 0) {
        goto error;
    }

    return 0;

error:
    (void)_PyCrossInterpreterData_UnregisterClass(&ChannelIDType);
    _globals_fini();
    return -1;
}

static struct PyModuleDef moduledef = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME,
    .m_doc = module_doc,
    .m_size = -1,
    .m_methods = module_functions,
};


PyMODINIT_FUNC
PyInit__xxsubinterpreters(void)
{
    /* Create the module */
    PyObject *mod = PyModule_Create(&moduledef);
    if (mod == NULL) {
        return NULL;
    }
    if (module_exec(mod) < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    return mod;
}
