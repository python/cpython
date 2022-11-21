#include "parts.h"


// Test dict watching
static PyObject *g_dict_watch_events;
static int g_dict_watchers_installed;

static int
dict_watch_callback(PyDict_WatchEvent event,
                    PyObject *dict,
                    PyObject *key,
                    PyObject *new_value)
{
    PyObject *msg;
    switch (event) {
        case PyDict_EVENT_CLEARED:
            msg = PyUnicode_FromString("clear");
            break;
        case PyDict_EVENT_DEALLOCATED:
            msg = PyUnicode_FromString("dealloc");
            break;
        case PyDict_EVENT_CLONED:
            msg = PyUnicode_FromString("clone");
            break;
        case PyDict_EVENT_ADDED:
            msg = PyUnicode_FromFormat("new:%S:%S", key, new_value);
            break;
        case PyDict_EVENT_MODIFIED:
            msg = PyUnicode_FromFormat("mod:%S:%S", key, new_value);
            break;
        case PyDict_EVENT_DELETED:
            msg = PyUnicode_FromFormat("del:%S", key);
            break;
        default:
            msg = PyUnicode_FromString("unknown");
    }
    if (msg == NULL) {
        return -1;
    }
    assert(PyList_Check(g_dict_watch_events));
    if (PyList_Append(g_dict_watch_events, msg) < 0) {
        Py_DECREF(msg);
        return -1;
    }
    Py_DECREF(msg);
    return 0;
}

static int
dict_watch_callback_second(PyDict_WatchEvent event,
                           PyObject *dict,
                           PyObject *key,
                           PyObject *new_value)
{
    PyObject *msg = PyUnicode_FromString("second");
    if (msg == NULL) {
        return -1;
    }
    int rc = PyList_Append(g_dict_watch_events, msg);
    Py_DECREF(msg);
    if (rc < 0) {
        return -1;
    }
    return 0;
}

static int
dict_watch_callback_error(PyDict_WatchEvent event,
                          PyObject *dict,
                          PyObject *key,
                          PyObject *new_value)
{
    PyErr_SetString(PyExc_RuntimeError, "boom!");
    return -1;
}

static PyObject *
add_dict_watcher(PyObject *self, PyObject *kind)
{
    int watcher_id;
    assert(PyLong_Check(kind));
    long kind_l = PyLong_AsLong(kind);
    if (kind_l == 2) {
        watcher_id = PyDict_AddWatcher(dict_watch_callback_second);
    }
    else if (kind_l == 1) {
        watcher_id = PyDict_AddWatcher(dict_watch_callback_error);
    }
    else {
        watcher_id = PyDict_AddWatcher(dict_watch_callback);
    }
    if (watcher_id < 0) {
        return NULL;
    }
    if (!g_dict_watchers_installed) {
        assert(!g_dict_watch_events);
        if (!(g_dict_watch_events = PyList_New(0))) {
            return NULL;
        }
    }
    g_dict_watchers_installed++;
    return PyLong_FromLong(watcher_id);
}

static PyObject *
clear_dict_watcher(PyObject *self, PyObject *watcher_id)
{
    if (PyDict_ClearWatcher(PyLong_AsLong(watcher_id))) {
        return NULL;
    }
    g_dict_watchers_installed--;
    if (!g_dict_watchers_installed) {
        assert(g_dict_watch_events);
        Py_CLEAR(g_dict_watch_events);
    }
    Py_RETURN_NONE;
}

static PyObject *
watch_dict(PyObject *self, PyObject *args)
{
    PyObject *dict;
    int watcher_id;
    if (!PyArg_ParseTuple(args, "iO", &watcher_id, &dict)) {
        return NULL;
    }
    if (PyDict_Watch(watcher_id, dict)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
unwatch_dict(PyObject *self, PyObject *args)
{
    PyObject *dict;
    int watcher_id;
    if (!PyArg_ParseTuple(args, "iO", &watcher_id, &dict)) {
        return NULL;
    }
    if (PyDict_Unwatch(watcher_id, dict)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
get_dict_watcher_events(PyObject *self, PyObject *Py_UNUSED(args))
{
    if (!g_dict_watch_events) {
        PyErr_SetString(PyExc_RuntimeError, "no watchers active");
        return NULL;
    }
    return Py_NewRef(g_dict_watch_events);
}

// Test type watchers
static PyObject *g_type_modified_events;
static int g_type_watchers_installed;

static int
type_modified_callback(PyTypeObject *type)
{
    assert(PyList_Check(g_type_modified_events));
    if(PyList_Append(g_type_modified_events, (PyObject *)type) < 0) {
        return -1;
    }
    return 0;
}

static int
type_modified_callback_wrap(PyTypeObject *type)
{
    assert(PyList_Check(g_type_modified_events));
    PyObject *list = PyList_New(0);
    if (list == NULL) {
        return -1;
    }
    if (PyList_Append(list, (PyObject *)type) < 0) {
        Py_DECREF(list);
        return -1;
    }
    if (PyList_Append(g_type_modified_events, list) < 0) {
        Py_DECREF(list);
        return -1;
    }
    Py_DECREF(list);
    return 0;
}

static int
type_modified_callback_error(PyTypeObject *type)
{
    PyErr_SetString(PyExc_RuntimeError, "boom!");
    return -1;
}

static PyObject *
add_type_watcher(PyObject *self, PyObject *kind)
{
    int watcher_id;
    assert(PyLong_Check(kind));
    long kind_l = PyLong_AsLong(kind);
    if (kind_l == 2) {
        watcher_id = PyType_AddWatcher(type_modified_callback_wrap);
    }
    else if (kind_l == 1) {
        watcher_id = PyType_AddWatcher(type_modified_callback_error);
    }
    else {
        watcher_id = PyType_AddWatcher(type_modified_callback);
    }
    if (watcher_id < 0) {
        return NULL;
    }
    if (!g_type_watchers_installed) {
        assert(!g_type_modified_events);
        if (!(g_type_modified_events = PyList_New(0))) {
            return NULL;
        }
    }
    g_type_watchers_installed++;
    return PyLong_FromLong(watcher_id);
}

static PyObject *
clear_type_watcher(PyObject *self, PyObject *watcher_id)
{
    if (PyType_ClearWatcher(PyLong_AsLong(watcher_id))) {
        return NULL;
    }
    g_type_watchers_installed--;
    if (!g_type_watchers_installed) {
        assert(g_type_modified_events);
        Py_CLEAR(g_type_modified_events);
    }
    Py_RETURN_NONE;
}

static PyObject *
get_type_modified_events(PyObject *self, PyObject *Py_UNUSED(args))
{
    if (!g_type_modified_events) {
        PyErr_SetString(PyExc_RuntimeError, "no watchers active");
        return NULL;
    }
    return Py_NewRef(g_type_modified_events);
}

static PyObject *
watch_type(PyObject *self, PyObject *args)
{
    PyObject *type;
    int watcher_id;
    if (!PyArg_ParseTuple(args, "iO", &watcher_id, &type)) {
        return NULL;
    }
    if (PyType_Watch(watcher_id, type)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
unwatch_type(PyObject *self, PyObject *args)
{
    PyObject *type;
    int watcher_id;
    if (!PyArg_ParseTuple(args, "iO", &watcher_id, &type)) {
        return NULL;
    }
    if (PyType_Unwatch(watcher_id, type)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    // Dict watchers.
    {"add_dict_watcher",         add_dict_watcher,        METH_O,       NULL},
    {"clear_dict_watcher",       clear_dict_watcher,      METH_O,       NULL},
    {"watch_dict",               watch_dict,              METH_VARARGS, NULL},
    {"unwatch_dict",             unwatch_dict,            METH_VARARGS, NULL},
    {"get_dict_watcher_events",  get_dict_watcher_events, METH_NOARGS,  NULL},

    // Type watchers.
    {"add_type_watcher",         add_type_watcher,        METH_O,       NULL},
    {"clear_type_watcher",       clear_type_watcher,      METH_O,       NULL},
    {"watch_type",               watch_type,              METH_VARARGS, NULL},
    {"unwatch_type",             unwatch_type,            METH_VARARGS, NULL},
    {"get_type_modified_events", get_type_modified_events, METH_NOARGS, NULL},
    {NULL},
};

int
_PyTestCapi_Init_Watchers(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
