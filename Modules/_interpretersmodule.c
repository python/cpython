
/* interpreters module */
/* low-level access to interpreter primitives */

#include "Python.h"
#include "frameobject.h"
#include "internal/pystate.h"


static PyInterpreterState *
_get_current(void)
{
    PyThreadState *tstate;

    tstate = PyThreadState_Get();
    if (tstate == NULL)
        return NULL;
    return tstate->interp;
}

/* data-sharing-specific code ***********************************************/

struct _shareditem {
    Py_UNICODE *name;
    Py_ssize_t namelen;
    _PyCrossInterpreterData data;
};

void
_sharedns_clear(struct _shareditem *shared)
{
    for (struct _shareditem *item=shared; item->name != NULL; item += 1) {
        _PyCrossInterpreterData_Release(&item->data);
    }
}

static struct _shareditem *
_get_shared_ns(PyObject *shareable, Py_ssize_t *lenp)
{
    if (shareable == NULL || shareable == Py_None)
        return NULL;
    Py_ssize_t len = PyDict_Size(shareable);
    *lenp = len;
    if (len == 0)
        return NULL;

    struct _shareditem *shared = PyMem_NEW(struct _shareditem, len+1);
    for (Py_ssize_t i=0; i < len; i++) {
        *(shared + i) = (struct _shareditem){0};
    }
    Py_ssize_t pos = 0;
    for (Py_ssize_t i=0; i < len; i++) {
        PyObject *key, *value;
        if (PyDict_Next(shareable, &pos, &key, &value) == 0) {
            break;
        }
        struct _shareditem *item = shared + i;

        if (_PyObject_GetCrossInterpreterData(value, &item->data) != 0)
            break;
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
_shareditem_apply(struct _shareditem *item, PyObject *ns)
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

struct _shared_exception {
    char *msg;
};

static struct _shared_exception *
_get_shared_exception(void)
{
    struct _shared_exception *err = PyMem_NEW(struct _shared_exception, 1);
    if (err == NULL)
        return NULL;
    PyObject *exc;
    PyObject *value;
    PyObject *tb;
    PyErr_Fetch(&exc, &value, &tb);
    PyObject *msg;
    if (value == NULL)
        msg = PyUnicode_FromFormat("%S", exc);
    else
        msg = PyUnicode_FromFormat("%S: %S", exc, value);
    if (msg == NULL) {
        err->msg = "unable to format exception";
        return NULL;
    }
    err->msg = (char *)PyUnicode_AsUTF8(msg);
    if (err->msg == NULL) {
        err->msg = "unable to encode exception";
    }
    return err;
}

static PyObject * _interp_failed_error;

static void
_apply_shared_exception(struct _shared_exception *exc)
{
    PyErr_SetString(_interp_failed_error, exc->msg);
}

/* channel-specific code */

struct _channelitem;

struct _channelitem {
    _PyCrossInterpreterData *data;
    struct _channelitem *next;
};

struct _channel;

typedef struct _channel {
    struct _channel *next;

    int64_t id;

    PyThread_type_lock mutex;

    int64_t count;
    struct _channelitem *first;
    struct _channelitem *last;
} _PyChannelState;

static _PyChannelState *
_channel_new(void)
{
    _PyChannelState *chan = PyMem_Malloc(sizeof(_PyChannelState));
    if (chan == NULL)
        return NULL;
    chan->next = NULL;
    chan->id = -1;
    chan->mutex = PyThread_allocate_lock();
    if (chan->mutex == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "can't initialize mutex for new channel");
        return NULL;
    }
    chan->count = 0;
    chan->first = NULL;
    chan->last = NULL;
    return chan;
}

static int
_channel_add(_PyChannelState *chan, _PyCrossInterpreterData *data)
{
    struct _channelitem *item = PyMem_Malloc(sizeof(struct _channelitem));
    if (item == NULL)
        return -1;
    item->data = data;
    item->next = NULL;

    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    chan->count += 1;
    if (chan->first == NULL)
        chan->first = item;
    chan->last = item;
    PyThread_release_lock(chan->mutex);

    return 0;
}

static _PyCrossInterpreterData *
_channel_next(_PyChannelState *chan)
{
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    struct _channelitem *item = chan->first;
    if (item == NULL) {
        PyThread_release_lock(chan->mutex);
        return NULL;
    }

    chan->first = item->next;
    if (chan->last == item)
        chan->last = NULL;
    chan->count -= 1;
    PyThread_release_lock(chan->mutex);

    _PyCrossInterpreterData *data = item->data;
    PyMem_Free(item);
    return data;
}

static void
_channel_clear(_PyChannelState *chan)
{
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    _PyCrossInterpreterData *data = _channel_next(chan);
    for (; data != NULL; data = _channel_next(chan)) {
        _PyCrossInterpreterData_Release(data);
        PyMem_Free(data);
    }
    PyThread_release_lock(chan->mutex);
}

static void
_channel_free(_PyChannelState *chan)
{
    _channel_clear(chan);
    PyThread_free_lock(chan->mutex);
    PyMem_Free(chan);
}

struct _channels {
    PyThread_type_lock mutex;
    _PyChannelState *head;
    int64_t count;
    int64_t next_id;
};

static int
_channels_init(struct _channels *channels)
{
    if (channels->mutex == NULL) {
        channels->mutex = PyThread_allocate_lock();
        if (channels->mutex == NULL) {
            PyMem_Free(channels);
            PyErr_SetString(PyExc_RuntimeError, "can't initialize mutex for new channel");
            return -1;
        }
    }
    channels->head = NULL;
    return 0;
}

static int64_t
_channels_next_id(struct _channels *channels)
{
    int64_t id = channels->next_id;
    if (id < 0) {
        /* overflow */
        PyErr_SetString(PyExc_RuntimeError,
                        "failed to get a channel ID");
        return -1;
    }
    channels->next_id += 1;
    return id;
}

_PyChannelState *
_channels_lookup(struct _channels *channels, int64_t id)
{
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    _PyChannelState *chan = channels->head;
    for (;chan != NULL; chan = chan->next) {
        if (chan->id == id)
            break;
    }
    PyThread_release_lock(channels->mutex);
    if (chan == NULL) {
        PyErr_Format(PyExc_RuntimeError, "channel %d not found", id);
    }
    return chan;
}

static int
_channels_add(struct _channels *channels, _PyChannelState *chan)
{
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    int64_t id = _channels_next_id(channels);
    if (id < 0) {
        PyThread_release_lock(channels->mutex);
        return -1;
    }
    if (channels->head != NULL) {
        chan->next = channels->head;
    }
    channels->head = chan;
    channels->count += 1;
    chan->id = id;
    PyThread_release_lock(channels->mutex);

    return 0;
}

_PyChannelState *
_channels_remove(struct _channels *channels, int64_t id)
{
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    if (channels->head == NULL) {
        PyThread_release_lock(channels->mutex);
        return NULL;
    }

    _PyChannelState *prev = NULL;
    _PyChannelState *chan = channels->head;
    for (;chan != NULL; chan = chan->next) {
        if (chan->id == id)
            break;
        prev = chan;
    }
    if (chan == NULL) {
        PyErr_Format(PyExc_RuntimeError, "channel %d not found", id);
        PyThread_release_lock(channels->mutex);
        return NULL;
    }

    if (chan == channels->head) {
        channels->head = chan->next;
    } else {
        prev->next = chan->next;
    }
    chan->next = NULL;

    channels->count -= 1;
    PyThread_release_lock(channels->mutex);
    return chan;
}

int64_t *
_channels_list_all(struct _channels *channels, int64_t *count)
{
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    int64_t *ids = PyMem_Malloc(sizeof(int64_t) * channels->count);
    if (ids == NULL) {
        PyThread_release_lock(channels->mutex);
        return NULL;
    }
    _PyChannelState *chan = channels->head;
    for (int64_t i=0; chan != NULL; chan = chan->next, i++) {
        ids[i] = chan->id;
    }
    *count = channels->count;
    PyThread_release_lock(channels->mutex);
    return ids;
}

/*
int
channel_list_interpreters(int id)
{
}

int
channel_close(void)
{
}
*/

/* "high"-level channel-related functions */

static int64_t
_channel_create(struct _channels *channels)
{
    _PyChannelState *chan = _channel_new();
    if (chan == NULL)
        return -1;
    if (_channels_add(channels, chan) != 0) {
        _channel_free(chan);
        return -1;
    }
    return chan->id;
}

static int
_channel_destroy(struct _channels *channels, int64_t id)
{
    _PyChannelState *chan = _channels_remove(channels, id);
    if (chan == NULL)
        return -1;
    _channel_free(chan);
    return 0;
}

int
_channel_send(struct _channels *channels, int64_t id, PyObject *obj)
{
    _PyCrossInterpreterData *data = PyMem_Malloc(sizeof(_PyCrossInterpreterData));
    if (_PyObject_GetCrossInterpreterData(obj, data) != 0)
        return -1;

    // XXX lock _PyChannelState to avoid race on destroy here?
    _PyChannelState *chan = _channels_lookup(channels, id);
    if (chan == NULL) {
        PyMem_Free(data);
        return -1;
    }
    if (_channel_add(chan, data) != 0) {
        PyMem_Free(data);
        return -1;
    }

    return 0;
}

PyObject *
_channel_recv(struct _channels *channels, int64_t id)
{
    // XXX lock _PyChannelState to avoid race on destroy here?
    _PyChannelState *chan = _channels_lookup(channels, id);
    if (chan == NULL)
        return NULL;
    _PyCrossInterpreterData *data = _channel_next(chan);
    if (data == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "empty channel");
        return NULL;
    }

    PyObject *obj = _PyCrossInterpreterData_NewObject(data);
    if (obj == NULL)
        return NULL;
    _PyCrossInterpreterData_Release(data);
    return obj;
}

/* interpreter-specific functions *******************************************/

static PyInterpreterState *
_look_up(PyObject *requested_id)
{
    long long id = PyLong_AsLongLong(requested_id);
    if (id == -1 && PyErr_Occurred() != NULL)
        return NULL;
    assert(id <= INT64_MAX);
    return _PyInterpreterState_LookUpID(id);
}

static PyObject *
_get_id(PyInterpreterState *interp)
{
    PY_INT64_T id = PyInterpreterState_GetID(interp);
    if (id < 0)
        return NULL;
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
        if (PyErr_Occurred() != NULL)
            return -1;
        return 0;
    }
    return (int)(frame->f_executing);
}

static int
_ensure_not_running(PyInterpreterState *interp)
{
    int is_running = _is_running(interp);
    if (is_running < 0)
        return -1;
    if (is_running) {
        PyErr_Format(PyExc_RuntimeError, "interpreter already running");
        return -1;
    }
    return 0;
}

static int
_run_script(PyInterpreterState *interp, const char *codestr,
            struct _shareditem *shared, Py_ssize_t num_shared,
            struct _shared_exception **exc)
{
    PyObject *main_mod = PyMapping_GetItemString(interp->modules, "__main__");
    if (main_mod == NULL)
        goto error;
    PyObject *ns = PyModule_GetDict(main_mod);  // borrowed
    Py_DECREF(main_mod);
    if (ns == NULL)
        goto error;
    Py_INCREF(ns);

    // Apply the cross-interpreter data.
    if (shared != NULL) {
        for (Py_ssize_t i=0; i < num_shared; i++) {
            struct _shareditem *item = &shared[i];
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
    } else {
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
    // XXX lock?
    if (_ensure_not_running(interp) < 0)
        return -1;

    Py_ssize_t num_shared;
    struct _shareditem *shared = _get_shared_ns(shareables, &num_shared);
    if (shared == NULL && PyErr_Occurred())
        return -1;

    // Switch to interpreter.
    PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
    PyThreadState *save_tstate = PyThreadState_Swap(tstate);

    // Run the script.
    struct _shared_exception *exc = NULL;
    int result = _run_script(interp, codestr, shared, num_shared, &exc);

    // Switch back.
    if (save_tstate != NULL)
        PyThreadState_Swap(save_tstate);

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
    struct _channels channels;
} _globals = {0};

static int
_init_globals(void)
{
    if (_channels_init(&_globals.channels) != 0)
        return -1;
    return 0;
}

static PyObject *
interp_create(PyObject *self, PyObject *args)
{
    if (!PyArg_UnpackTuple(args, "create", 0, 0))
        return NULL;

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
    if (!PyArg_UnpackTuple(args, "destroy", 1, 1, &id))
        return NULL;
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "ID must be an int");
        return NULL;
    }

    // Look up the interpreter.
    PyInterpreterState *interp = _look_up(id);
    if (interp == NULL)
        return NULL;

    // Ensure we don't try to destroy the current interpreter.
    PyInterpreterState *current = _get_current();
    if (current == NULL)
        return NULL;
    if (interp == current) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot destroy the current interpreter");
        return NULL;
    }

    // Ensure the interpreter isn't running.
    /* XXX We *could* support destroying a running interpreter but
       aren't going to worry about it for now. */
    if (_ensure_not_running(interp) < 0)
        return NULL;

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
    if (ids == NULL)
        return NULL;

    interp = PyInterpreterState_Head();
    while (interp != NULL) {
        id = _get_id(interp);
        if (id == NULL)
            return NULL;
        // insert at front of list
        if (PyList_Insert(ids, 0, id) < 0)
            return NULL;

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
    if (interp == NULL)
        return NULL;
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
    if (!PyArg_UnpackTuple(args, "run_string", 2, 3, &id, &code, &shared))
        return NULL;
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
    if (interp == NULL)
        return NULL;

    // Extract code.
    Py_ssize_t size;
    const char *codestr = PyUnicode_AsUTF8AndSize(code, &size);
    if (codestr == NULL)
        return NULL;
    if (strlen(codestr) != (size_t)size) {
        PyErr_SetString(PyExc_ValueError,
                        "source code string cannot contain null bytes");
        return NULL;
    }

    // Run the code in the interpreter.
    if (_run_script_in_interpreter(interp, codestr, shared) != 0)
        return NULL;
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
    if (!PyArg_UnpackTuple(args, "is_shareable", 1, 1, &obj))
        return NULL;
    if (_PyObject_CheckCrossInterpreterData(obj) == 0)
        Py_RETURN_TRUE;
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
    if (!PyArg_UnpackTuple(args, "is_running", 1, 1, &id))
        return NULL;
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "ID must be an int");
        return NULL;
    }

    PyInterpreterState *interp = _look_up(id);
    if (interp == NULL)
        return NULL;
    int is_running = _is_running(interp);
    if (is_running < 0)
        return NULL;
    if (is_running)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(is_running_doc,
"is_running(id) -> bool\n\
\n\
Return whether or not the identified interpreter is running.");

static PyObject *
channel_list_all(PyObject *self)
{
    // XXX finish
    PyErr_SetString(PyExc_NotImplementedError, "not implemented yet");
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_list_all_doc,
"channel_list_all() -> [ID]\n\
\n\
Return the list of all IDs for active channels.");

static PyObject *
channel_create(PyObject *self)
{
    int64_t cid = _channel_create(&_globals.channels);
    if (cid < 0)
        return NULL;
    PyObject *id = PyLong_FromLongLong(cid);
    if (id == NULL) {
        _channel_destroy(&_globals.channels, cid);
        return NULL;
    }
    return id;
}

PyDoc_STRVAR(channel_create_doc,
"channel_create() -> ID\n\
\n\
Create a new cross-interpreter channel and return a unique generated ID.");

static PyObject *
channel_send(PyObject *self, PyObject *args)
{
    PyObject *id;
    PyObject *obj;
    if (!PyArg_UnpackTuple(args, "channel_send", 2, 2, &id, &obj))
        return NULL;
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "ID must be an int");
        return NULL;
    }

    long long cid = PyLong_AsLongLong(id);
    if (cid == -1 && PyErr_Occurred() != NULL)
        return NULL;
    assert(cid <= INT64_MAX);
    if (_channel_send(&_globals.channels, cid, obj) != 0)
        return NULL;
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
    if (!PyArg_UnpackTuple(args, "channel_recv", 1, 1, &id))
        return NULL;
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "ID must be an int");
        return NULL;
    }

    long long cid = PyLong_AsLongLong(id);
    if (cid == -1 && PyErr_Occurred() != NULL)
        return NULL;
    assert(cid <= INT64_MAX);
    return _channel_recv(&_globals.channels, cid);
}

PyDoc_STRVAR(channel_recv_doc,
"channel_recv(ID) -> obj\n\
\n\
Return a new object from the data at the from of the channel's queue.");

static PyObject *
channel_close(PyObject *self, PyObject *args)
{
    PyObject *id;
    if (!PyArg_UnpackTuple(args, "channel_close", 1, 1, &id))
        return NULL;
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "ID must be an int");
        return NULL;
    }

    // XXX finish
    PyErr_SetString(PyExc_NotImplementedError, "not implemented yet");
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_close_doc,
"channel_close(ID, *, send=None, recv=None)\n\
\n\
Close the channel for the current interpreter.  'send' and 'receive'\n\
(bool) may be used to indicate the ends to close.  By default both\n\
ends are closed.  Closing an already closed end is a noop.");

static PyObject *
channel_list_interpreters(PyObject *self, PyObject *args)
{
    PyObject *id;
    if (!PyArg_UnpackTuple(args, "...", 1, 1, &id))
        return NULL;
    if (!PyLong_Check(id)) {
        PyErr_SetString(PyExc_TypeError, "ID must be an int");
        return NULL;
    }

    // XXX finish
    PyErr_SetString(PyExc_NotImplementedError, "not implemented yet");
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channel_list_interpreters_doc,
"channel_list_interpreters(ID) -> ([ID], [ID])\n\
\n\
Return the list of intepreter IDs associated with the channel\n\
on the recv and send ends, respectively.");

static PyMethodDef module_functions[] = {
    {"create",                     (PyCFunction)interp_create,
     METH_VARARGS, create_doc},
    {"destroy",                    (PyCFunction)interp_destroy,
     METH_VARARGS, destroy_doc},

    {"list_all",                   (PyCFunction)interp_list_all,
     METH_NOARGS, list_all_doc},
    {"get_current",                (PyCFunction)interp_get_current,
     METH_NOARGS, get_current_doc},
    {"get_main",                   (PyCFunction)interp_get_main,
     METH_NOARGS, get_main_doc},
    {"is_running",                 (PyCFunction)interp_is_running,
     METH_VARARGS, is_running_doc},

    {"run_string",                 (PyCFunction)interp_run_string,
     METH_VARARGS, run_string_doc},

    {"is_shareable",               (PyCFunction)object_is_shareable,
     METH_VARARGS, is_shareable_doc},

    {"channel_list_all",           (PyCFunction)channel_list_all,
     METH_NOARGS, channel_list_all_doc},
    {"channel_create",             (PyCFunction)channel_create,
     METH_NOARGS, channel_create_doc},
    {"channel_send",               (PyCFunction)channel_send,
     METH_VARARGS, channel_send_doc},
    {"channel_recv",               (PyCFunction)channel_recv,
     METH_VARARGS, channel_recv_doc},
    {"channel_close",              (PyCFunction)channel_close,
     METH_VARARGS, channel_close_doc},
    {"channel_list_interpreters",  (PyCFunction)channel_list_interpreters,
     METH_VARARGS, channel_list_interpreters_doc},

    {NULL,                      NULL}           /* sentinel */
};


/* initialization function */

PyDoc_STRVAR(module_doc,
"This module provides primitive operations to manage Python interpreters.\n\
The 'interpreters' module provides a more convenient interface.");

static struct PyModuleDef interpretersmodule = {
    PyModuleDef_HEAD_INIT,
    "_interpreters",  /* m_name */
    module_doc,       /* m_doc */
    -1,               /* m_size */
    module_functions, /* m_methods */
    NULL,             /* m_slots */
    NULL,             /* m_traverse */
    NULL,             /* m_clear */
    NULL              /* m_free */
};


PyMODINIT_FUNC
PyInit__interpreters(void)
{
    if (_init_globals() != 0)
        return NULL;

    PyObject *module = PyModule_Create(&interpretersmodule);
    if (module == NULL)
        return NULL;
    PyObject *ns = PyModule_GetDict(module);  // borrowed

    _interp_failed_error = PyErr_NewException("_interpreters.RunFailedError",
                                              PyExc_RuntimeError, NULL);
    if (_interp_failed_error == NULL)
        return NULL;
    PyDict_SetItemString(ns, "RunFailedError", _interp_failed_error);

    return module;
}
