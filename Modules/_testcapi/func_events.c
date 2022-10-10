#include "parts.h"

static const int num_watchers = 2;
static PyObject *pyfunc_watchers[num_watchers];
static int watcher_ids[num_watchers] = {-1, -1};

static PyObject *get_id(PyObject *obj) {
    PyObject *builtins = PyEval_GetBuiltins();
    if (builtins == NULL) {
        return NULL;
    }
    PyObject *id_str = PyUnicode_FromString("id");
    if (id_str == NULL) {
        return NULL;
    }
    PyObject *id_func = PyObject_GetItem(builtins, id_str);
    Py_DECREF(id_str);
    if (id_func == NULL) {
        return NULL;
    }
    PyObject *stack[] = {obj};
    PyObject *id = PyObject_Vectorcall(id_func, stack, 1, NULL);
    Py_DECREF(id_func);
    return id;
}

static int
call_pyfunc_watcher(PyObject *watcher, PyFunction_WatchEvent event, PyFunctionObject *func, PyObject *new_value)
{
    PyObject *event_obj = PyLong_FromLong(event);
    if (event_obj == NULL) {
        return -1;
    }
    if (new_value == NULL) {
        new_value = Py_None;
    }
    Py_INCREF(new_value);
    PyObject *func_or_id = NULL;
    if (event == PyFunction_EVENT_DESTROY) {
        /* Don't expose a function that's about to be destroyed to managed code */
        func_or_id = get_id((PyObject *) func);
        if (func_or_id == NULL) {
            Py_DECREF(event_obj);
            Py_DECREF(new_value);
            return -1;
        }
    } else {
        Py_INCREF(func);
        func_or_id = (PyObject *) func;
    }
    PyObject *stack[] = {event_obj, func_or_id, new_value};
    PyObject *res = PyObject_Vectorcall(watcher, stack, 3, NULL);
    int st = (res == NULL) ? -1 : 0;
    Py_XDECREF(res);
    Py_DECREF(new_value);
    Py_DECREF(event_obj);
    Py_DECREF(func_or_id);
    return st;
}

static int
first_watcher_callback(PyFunction_WatchEvent event, PyFunctionObject *func, PyObject *new_value)
{
    return call_pyfunc_watcher(pyfunc_watchers[0], event, func, new_value);
}

static int
second_watcher_callback(PyFunction_WatchEvent event, PyFunctionObject *func, PyObject *new_value)
{
    return call_pyfunc_watcher(pyfunc_watchers[1], event, func, new_value);
}

static PyFunction_WatchCallback watcher_callbacks[num_watchers] = {
  first_watcher_callback,
  second_watcher_callback
};

static int
add_event(PyObject *module, const char *name, PyFunction_WatchEvent event)
{
    PyObject *value = PyLong_FromLong(event);
    if (value == NULL) {
        return -1;
    }
    int ok = PyModule_AddObjectRef(module, name, value);
    Py_DECREF(value);
    return ok;
}

static PyObject *
add_watcher(PyObject *self, PyObject *func)
{
    if (!PyFunction_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "'func' must be a function");
        return NULL;
    }
    int idx = -1;
    for (int i = 0; i < num_watchers; i++) {
        if (watcher_ids[i] == -1) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        PyErr_SetString(PyExc_RuntimeError, "no free watchers");
        return NULL;
    }
    PyObject *result = PyLong_FromLong(idx);
    if (result == NULL) {
        return NULL;
    }
    watcher_ids[idx] = PyFunction_AddWatcher(watcher_callbacks[idx]);
    if (watcher_ids[idx] < 0) {
        Py_DECREF(result);
        return NULL;
    }
    pyfunc_watchers[idx] = func;
    Py_INCREF(func);
    return result;
}

static PyObject *
clear_watcher(PyObject *self, PyObject *watcher_id_obj)
{
    long watcher_id = PyLong_AsLong(watcher_id_obj);
    if ((watcher_id < INT_MIN) || (watcher_id > INT_MAX)) {
        PyErr_SetString(PyExc_ValueError, "invalid watcher id");
        return NULL;
    }
    int wid = (int) watcher_id;
    if (PyFunction_ClearWatcher(wid) < 0) {
        return NULL;
    }
    int idx = -1;
    for (int i = 0; i < num_watchers; i++) {
      if (watcher_ids[i] == wid) {
          idx = i;
          break;
      }
    }
    assert(idx != -1);
    Py_CLEAR(pyfunc_watchers[idx]);
    watcher_ids[idx] = -1;
    Py_RETURN_NONE;
}

static PyMethodDef TestMethods[] = {
    {"_add_func_watcher", add_watcher, METH_O},
    {"_clear_func_watcher", clear_watcher, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_FuncEvents(PyObject *m) {
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    /* Expose each event as an attribute on the module */
#define ADD_EVENT(event)  \
    if (add_event(m, "PYFUNC_EVENT_" #event, PyFunction_EVENT_##event)) { \
        return -1;                                    \
    }
    FOREACH_FUNC_EVENT(ADD_EVENT);
#undef ADD_EVENT

    return 0;
}
