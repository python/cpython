
/* interpreters module */
/* low-level access to interpreter primitives */

#include "Python.h"
#include "frameobject.h"
#include "internal/pystate.h"


static PyInterpreterState *
_get_current(void)
{
    PyThreadState *tstate = PyThreadState_Get();
    // PyThreadState_Get() aborts if lookup fails, so we don't need
    // to check the result for NULL.
    return tstate->interp;
}

static int64_t
_coerce_id(PyObject *id)
{
    id = PyNumber_Long(id);
    if (id == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_SetString(PyExc_TypeError,
                            "'id' must be a non-negative int");
        }
        else {
            PyErr_SetString(PyExc_ValueError,
                            "'id' must be a non-negative int");
        }
        return -1;
    }
    long long cid = PyLong_AsLongLong(id);
    if (cid == -1 && PyErr_Occurred() != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "'id' must be a non-negative int");
        return -1;
    }
    if (cid < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "'id' must be a non-negative int");
        return -1;
    }
    if (cid > INT64_MAX) {
        PyErr_SetString(PyExc_ValueError,
                        "'id' too large (must be 64-bit int)");
        return -1;
    }
    return cid;
}

/* data-sharing-specific code ***********************************************/

typedef struct _shareditem {
    Py_UNICODE *name;
    Py_ssize_t namelen;
    _PyCrossInterpreterData data;
} _shareditem;

void
_sharedns_clear(_shareditem *shared)
{
    for (_shareditem *item=shared; item->name != NULL; item += 1) {
        _PyCrossInterpreterData_Release(&item->data);
    }
}

static _shareditem *
_get_shared_ns(PyObject *shareable, Py_ssize_t *lenp)
{
    if (shareable == NULL || shareable == Py_None) {
        *lenp = 0;
        return NULL;
    }
    Py_ssize_t len = PyDict_Size(shareable);
    *lenp = len;
    if (len == 0) {
        return NULL;
    }

    _shareditem *shared = PyMem_NEW(_shareditem, len+1);
    if (shared == NULL) {
        return NULL;
    }
    for (Py_ssize_t i=0; i < len; i++) {
        *(shared + i) = (_shareditem){0};
    }
    Py_ssize_t pos = 0;
    for (Py_ssize_t i=0; i < len; i++) {
        PyObject *key, *value;
        if (PyDict_Next(shareable, &pos, &key, &value) == 0) {
            break;
        }
        _shareditem *item = shared + i;

        if (_PyObject_GetCrossInterpreterData(value, &item->data) != 0) {
            break;
        }
        item->name = PyUnicode_AsUnicodeAndSize(key, &item->namelen);
        if (item->name == NULL) {
            _PyCrossInterpreterData_Release(&item->data);
            break;
        }
        (item + 1)->name = NULL;  // Mark the next one as the last.
    }
    if (PyErr_Occurred()) {
        _sharedns_clear(shared);
        PyMem_Free(shared);
        return NULL;
    }
    return shared;
}

static int
_shareditem_apply(_shareditem *item, PyObject *ns)
{
    PyObject *name = PyUnicode_FromUnicode(item->name, item->namelen);
    if (name == NULL) {
        return 1;
    }
    PyObject *value = _PyCrossInterpreterData_NewObject(&item->data);
    if (value == NULL) {
        Py_DECREF(name);
        return 1;
    }
    int res = PyDict_SetItem(ns, name, value);
    Py_DECREF(name);
    Py_DECREF(value);
    return res;
}

// Ultimately we'd like to preserve enough information about the
// exception and traceback that we could re-constitute (or at least
// simulate, a la traceback.TracebackException), and even chain, a copy
// of the exception in the calling interpreter.

typedef struct _sharedexception {
    char *msg;
} _sharedexception;

static _sharedexception *
_get_shared_exception(void)
{
    _sharedexception *err = PyMem_NEW(_sharedexception, 1);
    if (err == NULL) {
        return NULL;
    }
    PyObject *exc;
    PyObject *value;
    PyObject *tb;
    PyErr_Fetch(&exc, &value, &tb);
    PyObject *msg;
    if (value == NULL) {
        msg = PyUnicode_FromFormat("%S", exc);
    }
    else {
        msg = PyUnicode_FromFormat("%S: %S", exc, value);
    }
    if (msg == NULL) {
        err->msg = "unable to format exception";
        return err;
    }
    err->msg = (char *)PyUnicode_AsUTF8(msg);
    if (err->msg == NULL) {
        err->msg = "unable to encode exception";
    }
    return err;
}

static PyObject * RunFailedError;

static int
interp_exceptions_init(PyObject *ns)
{
    // XXX Move the exceptions into per-module memory?

    // An uncaught exception came out of interp_run_string().
    RunFailedError = PyErr_NewException("_xxsubinterpreters.RunFailedError",
                                        PyExc_RuntimeError, NULL);
    if (RunFailedError == NULL) {
        return -1;
    }
    if (PyDict_SetItemString(ns, "RunFailedError", RunFailedError) != 0) {
        return -1;
    }

    return 0;
}

static void
_apply_shared_exception(_sharedexception *exc)
{
    PyErr_SetString(RunFailedError, exc->msg);
}

/* channel-specific code */

static PyObject *ChannelError;
static PyObject *ChannelNotFoundError;
static PyObject *ChannelClosedError;
static PyObject *ChannelEmptyError;

static int
channel_exceptions_init(PyObject *ns)
{
    // XXX Move the exceptions into per-module memory?

    // A channel-related operation failed.
    ChannelError = PyErr_NewException("_xxsubinterpreters.ChannelError",
                                      PyExc_RuntimeError, NULL);
    if (ChannelError == NULL) {
        return -1;
    }
    if (PyDict_SetItemString(ns, "ChannelError", ChannelError) != 0) {
        return -1;
    }

    // An operation tried to use a channel that doesn't exist.
    ChannelNotFoundError = PyErr_NewException(
            "_xxsubinterpreters.ChannelNotFoundError", ChannelError, NULL);
    if (ChannelNotFoundError == NULL) {
        return -1;
    }
    if (PyDict_SetItemString(ns, "ChannelNotFoundError", ChannelNotFoundError) != 0) {
        return -1;
    }

    // An operation tried to use a closed channel.
    ChannelClosedError = PyErr_NewException(
            "_xxsubinterpreters.ChannelClosedError", ChannelError, NULL);
    if (ChannelClosedError == NULL) {
        return -1;
    }
    if (PyDict_SetItemString(ns, "ChannelClosedError", ChannelClosedError) != 0) {
        return -1;
    }

    // An operation tried to pop from an empty channel.
    ChannelEmptyError = PyErr_NewException(
            "_xxsubinterpreters.ChannelEmptyError", ChannelError, NULL);
    if (ChannelEmptyError == NULL) {
        return -1;
    }
    if (PyDict_SetItemString(ns, "ChannelEmptyError", ChannelEmptyError) != 0) {
        return -1;
    }

    return 0;
}

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
        return NULL;
    }

    end->next = NULL;
    end->interp = interp;

    end->open = 1;

    return end;
}

static void
_channelend_free_all(_channelend *end) {
    while (end != NULL) {
        _channelend *last = end;
        end = end->next;
        PyMem_Free(last);
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

struct _channelitem;

typedef struct _channelitem {
    _PyCrossInterpreterData *data;
    struct _channelitem *next;
} _channelitem;

struct _channel;

typedef struct _channel {
    PyThread_type_lock mutex;

    int open;

    int64_t count;
    _channelitem *first;
    _channelitem *last;

    // Note that the list entries are never removed for interpreter
    // for which the channel is closed.  This should be a problem in
    // practice.  Also, a channel isn't automatically closed when an
    // interpreter is destroyed.
    int64_t numsendopen;
    int64_t numrecvopen;
    _channelend *send;
    _channelend *recv;
} _PyChannelState;

static _PyChannelState *
_channel_new(void)
{
    _PyChannelState *chan = PyMem_NEW(_PyChannelState, 1);
    if (chan == NULL) {
        return NULL;
    }
    chan->mutex = PyThread_allocate_lock();
    if (chan->mutex == NULL) {
        PyMem_Free(chan);
        PyErr_SetString(ChannelError,
                        "can't initialize mutex for new channel");
        return NULL;
    }

    chan->open = 1;

    chan->count = 0;
    chan->first = NULL;
    chan->last = NULL;

    chan->numsendopen = 0;
    chan->numrecvopen = 0;
    chan->send = NULL;
    chan->recv = NULL;

    return chan;
}

static _channelend *
_channel_add_end(_PyChannelState *chan, _channelend *prev, int64_t interp,
                 int send)
{
    _channelend *end = _channelend_new(interp);
    if (end == NULL) {
        return NULL;
    }

    if (prev == NULL) {
        if (send) {
            chan->send = end;
        }
        else {
            chan->recv = end;
        }
    }
    else {
        prev->next = end;
    }
    if (send) {
        chan->numsendopen += 1;
    }
    else {
        chan->numrecvopen += 1;
    }
    return end;
}

static _channelend *
_channel_associate_end(_PyChannelState *chan, int64_t interp, int send)
{
    if (!chan->open) {
        PyErr_SetString(ChannelClosedError, "channel closed");
        return NULL;
    }

    _channelend *prev;
    _channelend *end = _channelend_find(send ? chan->send : chan->recv,
                                        interp, &prev);
    if (end != NULL) {
        if (!end->open) {
            PyErr_SetString(ChannelClosedError, "channel already closed");
            return NULL;
        }
        // already associated
        return end;
    }
    return _channel_add_end(chan, prev, interp, send);
}

static void
_channel_close_channelend(_PyChannelState *chan, _channelend *end, int send)
{
    end->open = 0;
    if (send) {
        chan->numsendopen -= 1;
    }
    else {
        chan->numrecvopen -= 1;
    }
}

static int
_channel_close_interpreter(_PyChannelState *chan, int64_t interp, int which)
{
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    int res = -1;
    if (!chan->open) {
        PyErr_SetString(ChannelClosedError, "channel already closed");
        goto done;
    }

    _channelend *prev;
    _channelend *end;
    if (which >= 0) {  // send/both
        end = _channelend_find(chan->send, interp, &prev);
        if (end == NULL) {
            // never associated so add it
            end = _channel_add_end(chan, prev, interp, 1);
            if (end == NULL) {
                goto done;
            }
        }
        _channel_close_channelend(chan, end, 1);
    }
    if (which <= 0) {  // recv/both
        end = _channelend_find(chan->recv, interp, &prev);
        if (end == NULL) {
            // never associated so add it
            end = _channel_add_end(chan, prev, interp, 0);
            if (end == NULL) {
                goto done;
            }
        }
        _channel_close_channelend(chan, end, 0);
    }

    if (chan->numsendopen == 0 && chan->numrecvopen == 0) {
        if (chan->send != NULL || chan->recv != NULL) {
            chan->open = 0;
        }
    }

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

static int
_channel_close_all(_PyChannelState *chan)
{
    int res = -1;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    if (!chan->open) {
        PyErr_SetString(ChannelClosedError, "channel already closed");
        goto done;
    }

    chan->open = 0;

    // We *could* also just leave these in place, since we've marked
    // the channel as closed already.

    // Ensure all the "send"-associated interpreters are closed.
    _channelend *end;
    for (end = chan->send; end != NULL; end = end->next) {
        _channel_close_channelend(chan, end, 1);
    }

    // Ensure all the "recv"-associated interpreters are closed.
    for (end = chan->recv; end != NULL; end = end->next) {
        _channel_close_channelend(chan, end, 0);
    }

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

static int
_channel_add(_PyChannelState *chan, int64_t interp,
             _PyCrossInterpreterData *data)
{
    int res = -1;

    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    if (_channel_associate_end(chan, interp, 1) == NULL) {
        goto done;
    }

    _channelitem *item = PyMem_NEW(_channelitem, 1);
    if (item == NULL) {
        goto done;
    }
    item->data = data;
    item->next = NULL;

    chan->count += 1;
    if (chan->first == NULL) {
        chan->first = item;
    }
    chan->last = item;

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

static _PyCrossInterpreterData *
_channel_next(_PyChannelState *chan, int64_t interp)
{
    _PyCrossInterpreterData *data = NULL;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    if (_channel_associate_end(chan, interp, 0) == NULL) {
        goto done;
    }

    _channelitem *item = chan->first;
    if (item == NULL) {
        goto done;
    }
    chan->first = item->next;
    if (chan->last == item) {
        chan->last = NULL;
    }
    chan->count -= 1;

    data = item->data;
    PyMem_Free(item);

done:
    PyThread_release_lock(chan->mutex);
    return data;
}

static void
_channel_clear(_PyChannelState *chan)
{
    _channelitem *item = chan->first;
    while (item != NULL) {
        _PyCrossInterpreterData_Release(item->data);
        PyMem_Free(item->data);
        _channelitem *last = item;
        item = item->next;
        PyMem_Free(last);
    }
    chan->first = NULL;
    chan->last = NULL;
}

static void
_channel_free(_PyChannelState *chan)
{
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    _channel_clear(chan);
    _channelend_free_all(chan->send);
    _channelend_free_all(chan->recv);
    PyThread_release_lock(chan->mutex);

    PyThread_free_lock(chan->mutex);
    PyMem_Free(chan);
}

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

static int
_channels_init(_channels *channels)
{
    if (channels->mutex == NULL) {
        channels->mutex = PyThread_allocate_lock();
        if (channels->mutex == NULL) {
            PyMem_Free(channels);
            PyErr_SetString(ChannelError,
                            "can't initialize mutex for channel management");
            return -1;
        }
    }
    channels->head = NULL;
    channels->numopen = 0;
    channels->next_id = 0;
    return 0;
}

static int64_t
_channels_next_id(_channels *channels)  // needs lock
{
    int64_t id = channels->next_id;
    if (id < 0) {
        /* overflow */
        PyErr_SetString(ChannelError,
                        "failed to get a channel ID");
        return -1;
    }
    channels->next_id += 1;
    return id;
}

static _PyChannelState *
_channels_lookup(_channels *channels, int64_t id, PyThread_type_lock *pmutex)
{
    _PyChannelState *chan = NULL;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    if (pmutex != NULL) {
        *pmutex = NULL;
    }

    _channelref *ref = _channelref_find(channels->head, id, NULL);
    if (ref == NULL) {
        PyErr_Format(ChannelNotFoundError, "channel %d not found", id);
        goto done;
    }
    if (ref->chan == NULL || !ref->chan->open) {
        PyErr_Format(ChannelClosedError, "channel %d closed", id);
        goto done;
    }

    if (pmutex != NULL) {
        // The mutex will be closed by the caller.
        *pmutex = channels->mutex;
    }

    chan = ref->chan;
done:
    if (pmutex == NULL || *pmutex == NULL) {
        PyThread_release_lock(channels->mutex);
    }
    return chan;
}

static int64_t
_channels_add(_channels *channels, _PyChannelState *chan)
{
    int64_t cid = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    // Create a new ref.
    int64_t id = _channels_next_id(channels);
    if (id < 0) {
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

static int
_channels_close(_channels *channels, int64_t cid, _PyChannelState **pchan)
{
    int res = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    if (pchan != NULL) {
        *pchan = NULL;
    }

    _channelref *ref = _channelref_find(channels->head, cid, NULL);
    if (ref == NULL) {
        PyErr_Format(ChannelNotFoundError, "channel %d not found", cid);
        goto done;
    }

    if (ref->chan == NULL) {
        PyErr_Format(ChannelClosedError, "channel %d closed", cid);
        goto done;
    }
    else {
        if (_channel_close_all(ref->chan) != 0) {
            goto done;
        }
        if (pchan != NULL) {
            *pchan = ref->chan;
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
    PyMem_Free(ref);
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
        PyErr_Format(ChannelNotFoundError, "channel %d not found", id);
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
        PyErr_Format(ChannelNotFoundError, "channel %d not found", id);
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

int64_t *
_channels_list_all(_channels *channels, int64_t *count)
{
    int64_t *cids = NULL;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    int64_t numopen = channels->numopen;
    if (numopen >= PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_RuntimeError, "too many channels open");
        goto done;
    }
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

/* "high"-level channel-related functions */

static int64_t
_channel_create(_channels *channels)
{
    _PyChannelState *chan = _channel_new();
    if (chan == NULL) {
        return -1;
    }
    int64_t id = _channels_add(channels, chan);
    if (id < 0) {
        _channel_free(chan);
        return -1;
    }
    return id;
}

static int
_channel_destroy(_channels *channels, int64_t id)
{
    _PyChannelState *chan = NULL;
    if (_channels_remove(channels, id, &chan) != 0) {
        return -1;
    }
    if (chan != NULL) {
        _channel_free(chan);
    }
    return 0;
}

static int
_channel_send(_channels *channels, int64_t id, PyObject *obj)
{
    PyInterpreterState *interp = _get_current();
    if (interp == NULL) {
        return -1;
    }

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = _channels_lookup(channels, id, &mutex);
    if (chan == NULL) {
        return -1;
    }
    // Past this point we are responsible for releasing the mutex.

    // Convert the object to cross-interpreter data.
    _PyCrossInterpreterData *data = PyMem_NEW(_PyCrossInterpreterData, 1);
    if (data == NULL) {
        PyThread_release_lock(mutex);
        return -1;
    }
    if (_PyObject_GetCrossInterpreterData(obj, data) != 0) {
        PyThread_release_lock(mutex);
        return -1;
    }

    // Add the data to the channel.
    int res = _channel_add(chan, interp->id, data);
    PyThread_release_lock(mutex);
    if (res != 0) {
        _PyCrossInterpreterData_Release(data);
        PyMem_Free(data);
        return -1;
    }

    return 0;
}

static PyObject *
_channel_recv(_channels *channels, int64_t id)
{
    PyInterpreterState *interp = _get_current();
    if (interp == NULL) {
        return NULL;
    }

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = _channels_lookup(channels, id, &mutex);
    if (chan == NULL) {
        return NULL;
    }
    // Past this point we are responsible for releasing the mutex.

    // Pop off the next item from the channel.
    _PyCrossInterpreterData *data = _channel_next(chan, interp->id);
    PyThread_release_lock(mutex);
    if (data == NULL) {
        PyErr_Format(ChannelEmptyError, "channel %d is empty", id);
        return NULL;
    }

    // Convert the data back to an object.
    PyObject *obj = _PyCrossInterpreterData_NewObject(data);
    if (obj == NULL) {
        return NULL;
    }
    _PyCrossInterpreterData_Release(data);

    return obj;
}

static int
_channel_drop(_channels *channels, int64_t id, int send, int recv)
{
    PyInterpreterState *interp = _get_current();
    if (interp == NULL) {
        return -1;
    }

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = _channels_lookup(channels, id, &mutex);
    if (chan == NULL) {
        return -1;
    }
    // Past this point we are responsible for releasing the mutex.

    // Close one or both of the two ends.
    int res =_channel_close_interpreter(chan, interp->id, send-recv);
    PyThread_release_lock(mutex);
    return res;
}

static int
_channel_close(_channels *channels, int64_t id)
{
    return _channels_close(channels, id, NULL);
}

/* ChannelID class */

#define CHANNEL_SEND 1
#define CHANNEL_RECV -1

static PyTypeObject ChannelIDtype;

typedef struct channelid {
    PyObject_HEAD
    int64_t id;
    int end;
    _channels *channels;
} channelid;

static channelid *
newchannelid(PyTypeObject *cls, int64_t cid, int end, _channels *channels,
             int force)
{
    channelid *self = PyObject_New(channelid, cls);
    if (self == NULL) {
        return NULL;
    }
    self->id = cid;
    self->end = end;
    self->channels = channels;

    if (_channels_add_id_object(channels, cid) != 0) {
        if (force && PyErr_ExceptionMatches(ChannelNotFoundError)) {
            PyErr_Clear();
        }
        else {
            Py_DECREF((PyObject *)self);
            return NULL;
        }
    }

    return self;
}

static _channels * _global_channels(void);

static PyObject *
channelid_new(PyTypeObject *cls, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "send", "recv", "force", NULL};
    PyObject *id;
    int send = -1;
    int recv = -1;
    int force = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O|$ppp:ChannelID.__init__", kwlist,
                                     &id, &send, &recv, &force))
        return NULL;

    // Coerce and check the ID.
    int64_t cid;
    if (PyObject_TypeCheck(id, &ChannelIDtype)) {
        cid = ((channelid *)id)->id;
    }
    else {
        cid = _coerce_id(id);
        if (cid < 0) {
            return NULL;
        }
    }

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

    return (PyObject *)newchannelid(cls, cid, end, _global_channels(), force);
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
        fmt = "%s(%d, send=True)";
    }
    else if (cid->end == CHANNEL_RECV) {
        fmt = "%s(%d, recv=True)";
    }
    else {
        fmt = "%s(%d)";
    }
    return PyUnicode_FromFormat(fmt, name, cid->id);
}

PyObject *
channelid_int(PyObject *self)
{
    channelid *cid = (channelid *)self;
    return PyLong_FromLongLong(cid->id);
}

static PyNumberMethods channelid_as_number = {
     0,                        /* nb_add */
     0,                        /* nb_subtract */
     0,                        /* nb_multiply */
     0,                        /* nb_remainder */
     0,                        /* nb_divmod */
     0,                        /* nb_power */
     0,                        /* nb_negative */
     0,                        /* nb_positive */
     0,                        /* nb_absolute */
     0,                        /* nb_bool */
     0,                        /* nb_invert */
     0,                        /* nb_lshift */
     0,                        /* nb_rshift */
     0,                        /* nb_and */
     0,                        /* nb_xor */
     0,                        /* nb_or */
     (unaryfunc)channelid_int, /* nb_int */
     0,                        /* nb_reserved */
     0,                        /* nb_float */

     0,                        /* nb_inplace_add */
     0,                        /* nb_inplace_subtract */
     0,                        /* nb_inplace_multiply */
     0,                        /* nb_inplace_remainder */
     0,                        /* nb_inplace_power */
     0,                        /* nb_inplace_lshift */
     0,                        /* nb_inplace_rshift */
     0,                        /* nb_inplace_and */
     0,                        /* nb_inplace_xor */
     0,                        /* nb_inplace_or */

     0,                        /* nb_floor_divide */
     0,                        /* nb_true_divide */
     0,                        /* nb_inplace_floor_divide */
     0,                        /* nb_inplace_true_divide */

     (unaryfunc)channelid_int, /* nb_index */
};

static Py_hash_t
channelid_hash(PyObject *self)
{
    channelid *cid = (channelid *)self;
    PyObject *id = PyLong_FromLongLong(cid->id);
    if (id == NULL) {
        return -1;
    }
    return PyObject_Hash(id);
}

static PyObject *
channelid_richcompare(PyObject *self, PyObject *other, int op)
{
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (!PyObject_TypeCheck(self, &ChannelIDtype)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    channelid *cid = (channelid *)self;
    int equal;
    if (PyObject_TypeCheck(other, &ChannelIDtype)) {
        channelid *othercid = (channelid *)other;
        if (cid->end != othercid->end) {
            equal = 0;
        }
        else {
            equal = (cid->id == othercid->id);
        }
    }
    else {
        other = PyNumber_Long(other);
        if (other == NULL) {
            PyErr_Clear();
            Py_RETURN_NOTIMPLEMENTED;
        }
        int64_t othercid = PyLong_AsLongLong(other);
        // XXX decref other here?
        if (othercid == -1 && PyErr_Occurred() != NULL) {
            return NULL;
        }
        if (othercid < 0 || othercid > INT64_MAX) {
            equal = 0;
        }
        else {
            equal = (cid->id == othercid);
        }
    }

    if ((op == Py_EQ && equal) || (op == Py_NE && !equal)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

struct _channelid_xid {
    int64_t id;
    int end;
};

static PyObject *
_channelid_from_xid(_PyCrossInterpreterData *data)
{
    struct _channelid_xid *xid = (struct _channelid_xid *)data->data;
    return (PyObject *)newchannelid(&ChannelIDtype, xid->id, xid->end,
                                    _global_channels(), 0);
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

    data->data = xid;
    data->obj = obj;
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
        return (PyObject *)newchannelid(Py_TYPE(self), cid->id, *(int *)end,
                                        cid->channels, force);
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

static PyTypeObject ChannelIDtype = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "_xxsubinterpreters.ChannelID", /* tp_name */
    sizeof(channelid),              /* tp_size */
    0,                              /* tp_itemsize */
    (destructor)channelid_dealloc,  /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_as_async */
    (reprfunc)channelid_repr,       /* tp_repr */
    &channelid_as_number,           /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    channelid_hash,                 /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_LONG_SUBCLASS,   /* tp_flags */
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
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    0,                              /* tp_alloc */
    // Note that we do not set tp_new to channelid_new.  Instead we
    // set it to NULL, meaning it cannot be instantiated from Python
    // code.  We do this because there is a strong relationship between
    // channel IDs and the channel lifecycle, so this limitation avoids
    // related complications.
    NULL,                           /* tp_new */
};

/* interpreter-specific functions *******************************************/

static PyInterpreterState *
_look_up(PyObject *requested_id)
{
    long long id = PyLong_AsLongLong(requested_id);
    if (id == -1 && PyErr_Occurred() != NULL) {
        return NULL;
    }
    assert(id <= INT64_MAX);
    return _PyInterpreterState_LookUpID(id);
}

static PyObject *
_get_id(PyInterpreterState *interp)
{
    PY_INT64_T id = PyInterpreterState_GetID(interp);
    if (id < 0) {
        return NULL;
    }
    return PyLong_FromLongLong(id);
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
    PyFrameObject *frame = tstate->frame;
    if (frame == NULL) {
        if (PyErr_Occurred() != NULL) {
            return -1;
        }
        return 0;
    }
    return (int)(frame->f_executing);
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
            _shareditem *shared, Py_ssize_t num_shared,
            _sharedexception **exc)
{
    assert(num_shared >= 0);
    PyObject *main_mod = PyMapping_GetItemString(interp->modules, "__main__");
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
        for (Py_ssize_t i=0; i < num_shared; i++) {
            _shareditem *item = &shared[i];
            if (_shareditem_apply(item, ns) != 0) {
                Py_DECREF(ns);
                goto error;
            }
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

    return 0;

error:
    *exc = _get_shared_exception();
    PyErr_Clear();
    return -1;
}

static int
_run_script_in_interpreter(PyInterpreterState *interp, const char *codestr,
                           PyObject *shareables)
{
    if (_ensure_not_running(interp) < 0) {
        return -1;
    }

    Py_ssize_t num_shared = -1;
    _shareditem *shared = _get_shared_ns(shareables, &num_shared);
    if (shared == NULL && PyErr_Occurred()) {
        return -1;
    }

    // Switch to interpreter.
    PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
    PyThreadState *save_tstate = PyThreadState_Swap(tstate);

    // Run the script.
    _sharedexception *exc = NULL;
    int result = _run_script(interp, codestr, shared, num_shared, &exc);

    // Switch back.
    if (save_tstate != NULL) {
        PyThreadState_Swap(save_tstate);
    }

    // Propagate any exception out to the caller.
    if (exc != NULL) {
        _apply_shared_exception(exc);
        PyMem_Free(exc);
    }
    else if (result != 0) {
        // We were unable to allocate a shared exception.
        PyErr_NoMemory();
    }

    if (shared != NULL) {
        _sharedns_clear(shared);
        PyMem_Free(shared);
    }

    return result;
}


/* module level code ********************************************************/

/* globals is the process-global state for the module.  It holds all
   the data that we need to share between interpreters, so it cannot
   hold PyObject values. */
static struct globals {
    _channels channels;
} _globals = {{0}};

static int
_init_globals(void)
{
    if (_channels_init(&_globals.channels) != 0) {
        return -1;
    }
    return 0;
}

static _channels *
_global_channels(void) {
    return &_globals.channels;
}

static PyObject *
interp_create(PyObject *self, PyObject *args)
{
    if (!PyArg_UnpackTuple(args, "create", 0, 0)) {
        return NULL;
    }

    // Create and initialize the new interpreter.
    PyThreadState *tstate, *save_tstate;
    save_tstate = PyThreadState_Swap(NULL);
    tstate = Py_NewInterpreter();
    PyThreadState_Swap(save_tstate);
    if (tstate == NULL) {
        /* Since no new thread state was created, there is no exception to
           propagate; raise a fresh one after swapping in the old thread
           state. */
        PyErr_SetString(PyExc_RuntimeError, "interpreter creation failed");
        return NULL;
    }
    return _get_id(tstate->interp);
}

PyDoc_STRVAR(create_doc,
"create() -> ID\n\
\n\
Create a new interpreter and return a unique generated ID.");


static PyObject *
interp_destroy(PyObject *self, PyObject *args)
{
    PyObject *id;
    if (!PyArg_UnpackTuple(args, "destroy", 1, 1, &id)) {
        return NULL;
    }
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "ID must be an int");
        return NULL;
    }

    // Look up the interpreter.
    PyInterpreterState *interp = _look_up(id);
    if (interp == NULL) {
        return NULL;
    }

    // Ensure we don't try to destroy the current interpreter.
    PyInterpreterState *current = _get_current();
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
    //PyInterpreterState_Delete(interp);
    PyThreadState *tstate, *save_tstate;
    tstate = PyInterpreterState_ThreadHead(interp);
    save_tstate = PyThreadState_Swap(tstate);
    Py_EndInterpreter(tstate);
    PyThreadState_Swap(save_tstate);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(destroy_doc,
"destroy(ID)\n\
\n\
Destroy the identified interpreter.\n\
\n\
Attempting to destroy the current interpreter results in a RuntimeError.\n\
So does an unrecognized ID.");


static PyObject *
interp_list_all(PyObject *self)
{
    PyObject *ids, *id;
    PyInterpreterState *interp;

    ids = PyList_New(0);
    if (ids == NULL) {
        return NULL;
    }

    interp = PyInterpreterState_Head();
    while (interp != NULL) {
        id = _get_id(interp);
        if (id == NULL) {
            Py_DECREF(ids);
            return NULL;
        }
        // insert at front of list
        if (PyList_Insert(ids, 0, id) < 0) {
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
interp_get_current(PyObject *self)
{
    PyInterpreterState *interp =_get_current();
    if (interp == NULL) {
        return NULL;
    }
    return _get_id(interp);
}

PyDoc_STRVAR(get_current_doc,
"get_current() -> ID\n\
\n\
Return the ID of current interpreter.");


static PyObject *
interp_get_main(PyObject *self)
{
    // Currently, 0 is always the main interpreter.
    return PyLong_FromLongLong(0);
}

PyDoc_STRVAR(get_main_doc,
"get_main() -> ID\n\
\n\
Return the ID of main interpreter.");


static PyObject *
interp_run_string(PyObject *self, PyObject *args)
{
    PyObject *id, *code;
    PyObject *shared = NULL;
    if (!PyArg_UnpackTuple(args, "run_string", 2, 3, &id, &code, &shared)) {
        return NULL;
    }
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "first arg (ID) must be an int");
        return NULL;
    }
    if (!PyUnicode_Check(code)) {
        PyErr_SetString(PyExc_TypeError,
                        "second arg (code) must be a string");
        return NULL;
    }

    // Look up the interpreter.
    PyInterpreterState *interp = _look_up(id);
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
    if (_run_script_in_interpreter(interp, codestr, shared) != 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(run_string_doc,
"run_string(ID, sourcetext)\n\
\n\
Execute the provided string in the identified interpreter.\n\
\n\
See PyRun_SimpleStrings.");


static PyObject *
object_is_shareable(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_UnpackTuple(args, "is_shareable", 1, 1, &obj)) {
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
interp_is_running(PyObject *self, PyObject *args)
{
    PyObject *id;
    if (!PyArg_UnpackTuple(args, "is_running", 1, 1, &id)) {
        return NULL;
    }
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "ID must be an int");
        return NULL;
    }

    PyInterpreterState *interp = _look_up(id);
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
channel_create(PyObject *self)
{
    int64_t cid = _channel_create(&_globals.channels);
    if (cid < 0) {
        return NULL;
    }
    PyObject *id = (PyObject *)newchannelid(&ChannelIDtype, cid, 0,
                                            &_globals.channels, 0);
    if (id == NULL) {
        if (_channel_destroy(&_globals.channels, cid) != 0) {
            // XXX issue a warning?
        }
        return NULL;
    }
    assert(((channelid *)id)->channels != NULL);
    return id;
}

PyDoc_STRVAR(channel_create_doc,
"channel_create() -> ID\n\
\n\
Create a new cross-interpreter channel and return a unique generated ID.");

static PyObject *
channel_destroy(PyObject *self, PyObject *args)
{
    PyObject *id;
    if (!PyArg_UnpackTuple(args, "channel_destroy", 1, 1, &id)) {
        return NULL;
    }
    int64_t cid = _coerce_id(id);
    if (cid < 0) {
        return NULL;
    }

    if (_channel_destroy(&_globals.channels, cid) != 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_destroy_doc,
"channel_destroy(ID)\n\
\n\
Close and finalize the channel.  Afterward attempts to use the channel\n\
will behave as though it never existed.");

static PyObject *
channel_list_all(PyObject *self)
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
        // XXX free cids
        return NULL;
    }
    for (int64_t i=0; i < count; cids++, i++) {
        PyObject *id = (PyObject *)newchannelid(&ChannelIDtype, *cids, 0,
                                                &_globals.channels, 0);
        if (id == NULL) {
            Py_DECREF(ids);
            ids = NULL;
            break;
        }
        PyList_SET_ITEM(ids, i, id);
    }
    // XXX free cids
    return ids;
}

PyDoc_STRVAR(channel_list_all_doc,
"channel_list_all() -> [ID]\n\
\n\
Return the list of all IDs for active channels.");

static PyObject *
channel_send(PyObject *self, PyObject *args)
{
    PyObject *id;
    PyObject *obj;
    if (!PyArg_UnpackTuple(args, "channel_send", 2, 2, &id, &obj)) {
        return NULL;
    }
    int64_t cid = _coerce_id(id);
    if (cid < 0) {
        return NULL;
    }

    if (_channel_send(&_globals.channels, cid, obj) != 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_send_doc,
"channel_send(ID, obj)\n\
\n\
Add the object's data to the channel's queue.");

static PyObject *
channel_recv(PyObject *self, PyObject *args)
{
    PyObject *id;
    if (!PyArg_UnpackTuple(args, "channel_recv", 1, 1, &id)) {
        return NULL;
    }
    int64_t cid = _coerce_id(id);
    if (cid < 0) {
        return NULL;
    }

    return _channel_recv(&_globals.channels, cid);
}

PyDoc_STRVAR(channel_recv_doc,
"channel_recv(ID) -> obj\n\
\n\
Return a new object from the data at the from of the channel's queue.");

static PyObject *
channel_close(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *id;
    if (!PyArg_UnpackTuple(args, "channel_recv", 1, 1, &id)) {
        return NULL;
    }
    int64_t cid = _coerce_id(id);
    if (cid < 0) {
        return NULL;
    }

    if (_channel_close(&_globals.channels, cid) != 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_close_doc,
"channel_close(ID)\n\
\n\
Close the channel for all interpreters.  Once the channel's ID has\n\
no more ref counts the channel will be destroyed.");

static PyObject *
channel_drop_interpreter(PyObject *self, PyObject *args, PyObject *kwds)
{
    // Note that only the current interpreter is affected.
    static char *kwlist[] = {"id", "send", "recv", NULL};
    PyObject *id;
    int send = -1;
    int recv = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O|$pp:channel_drop_interpreter", kwlist,
                                     &id, &send, &recv))
        return NULL;

    int64_t cid = _coerce_id(id);
    if (cid < 0) {
        return NULL;
    }
    if (send < 0 && recv < 0) {
        send = 1;
        recv = 1;
    }
    else {
        if (send < 0) {
            send = 0;
        }
        if (recv < 0) {
            recv = 0;
        }
    }
    if (_channel_drop(&_globals.channels, cid, send, recv) != 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_drop_interpreter_doc,
"channel_drop_interpreter(ID, *, send=None, recv=None)\n\
\n\
Close the channel for the current interpreter.  'send' and 'recv'\n\
(bool) may be used to indicate the ends to close.  By default both\n\
ends are closed.  Closing an already closed end is a noop.");

static PyObject *
channel__channel_id(PyObject *self, PyObject *args, PyObject *kwds)
{
    return channelid_new(&ChannelIDtype, args, kwds);
}

static PyMethodDef module_functions[] = {
    {"create",                    (PyCFunction)interp_create,
     METH_VARARGS, create_doc},
    {"destroy",                   (PyCFunction)interp_destroy,
     METH_VARARGS, destroy_doc},
    {"list_all",                  (PyCFunction)interp_list_all,
     METH_NOARGS, list_all_doc},
    {"get_current",               (PyCFunction)interp_get_current,
     METH_NOARGS, get_current_doc},
    {"get_main",                  (PyCFunction)interp_get_main,
     METH_NOARGS, get_main_doc},
    {"is_running",                (PyCFunction)interp_is_running,
     METH_VARARGS, is_running_doc},
    {"run_string",                (PyCFunction)interp_run_string,
     METH_VARARGS, run_string_doc},

    {"is_shareable",              (PyCFunction)object_is_shareable,
     METH_VARARGS, is_shareable_doc},

    {"channel_create",            (PyCFunction)channel_create,
     METH_NOARGS, channel_create_doc},
    {"channel_destroy",           (PyCFunction)channel_destroy,
     METH_VARARGS, channel_destroy_doc},
    {"channel_list_all",          (PyCFunction)channel_list_all,
     METH_NOARGS, channel_list_all_doc},
    {"channel_send",              (PyCFunction)channel_send,
     METH_VARARGS, channel_send_doc},
    {"channel_recv",              (PyCFunction)channel_recv,
     METH_VARARGS, channel_recv_doc},
    {"channel_close",             (PyCFunction)channel_close,
     METH_VARARGS, channel_close_doc},
    {"channel_drop_interpreter",  (PyCFunction)channel_drop_interpreter,
     METH_VARARGS | METH_KEYWORDS, channel_drop_interpreter_doc},
    {"_channel_id",               (PyCFunction)channel__channel_id,
     METH_VARARGS | METH_KEYWORDS, NULL},

    {NULL,                        NULL}           /* sentinel */
};


/* initialization function */

PyDoc_STRVAR(module_doc,
"This module provides primitive operations to manage Python interpreters.\n\
The 'interpreters' module provides a more convenient interface.");

static struct PyModuleDef interpretersmodule = {
    PyModuleDef_HEAD_INIT,
    "_xxsubinterpreters",  /* m_name */
    module_doc,            /* m_doc */
    -1,                    /* m_size */
    module_functions,      /* m_methods */
    NULL,                  /* m_slots */
    NULL,                  /* m_traverse */
    NULL,                  /* m_clear */
    NULL                   /* m_free */
};


PyMODINIT_FUNC
PyInit__xxsubinterpreters(void)
{
    if (_init_globals() != 0) {
        return NULL;
    }

    /* Initialize types */
    ChannelIDtype.tp_base = &PyLong_Type;
    if (PyType_Ready(&ChannelIDtype) != 0) {
        return NULL;
    }

    /* Create the module */
    PyObject *module = PyModule_Create(&interpretersmodule);
    if (module == NULL) {
        return NULL;
    }

    /* Add exception types */
    PyObject *ns = PyModule_GetDict(module);  // borrowed
    if (interp_exceptions_init(ns) != 0) {
        return NULL;
    }
    if (channel_exceptions_init(ns) != 0) {
        return NULL;
    }

    /* Add other types */
    Py_INCREF(&ChannelIDtype);
    if (PyDict_SetItemString(ns, "ChannelID", (PyObject *)&ChannelIDtype) != 0) {
        return NULL;
    }

    if (_PyCrossInterpreterData_Register_Class(&ChannelIDtype, _channelid_shared)) {
        return NULL;
    }

    return module;
}
