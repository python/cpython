// clinic/watchers.c.h uses internal pycore_modsupport.h API
#define PYTESTCAPI_NEED_INTERNAL_API

#include "parts.h"

#include "clinic/watchers.c.h"

#define Py_BUILD_CORE
#include "pycore_function.h"      // FUNC_MAX_WATCHERS
#include "pycore_interp_structs.h" // CODE_MAX_WATCHERS
#include "pycore_context.h"       // CONTEXT_MAX_WATCHERS

/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/

// Test dict watching
static PyObject *g_dict_watch_events = NULL;
static int g_dict_watchers_installed = 0;

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

/*[clinic input]
_testcapi.watch_dict
    watcher_id: int
    dict: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_watch_dict_impl(PyObject *module, int watcher_id, PyObject *dict)
/*[clinic end generated code: output=1426e0273cebe2d8 input=269b006d60c358bd]*/
{
    if (PyDict_Watch(watcher_id, dict)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_testcapi.unwatch_dict = _testcapi.watch_dict
[clinic start generated code]*/

static PyObject *
_testcapi_unwatch_dict_impl(PyObject *module, int watcher_id, PyObject *dict)
/*[clinic end generated code: output=512b1a71ae33c351 input=cae7dc1b6f7713b8]*/
{
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

/*[clinic input]
_testcapi.watch_type
    watcher_id: int
    type: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_watch_type_impl(PyObject *module, int watcher_id, PyObject *type)
/*[clinic end generated code: output=fdf4777126724fc4 input=5a808bf12be7e3ed]*/
{
    if (PyType_Watch(watcher_id, type)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_testcapi.unwatch_type = _testcapi.watch_type
[clinic start generated code]*/

static PyObject *
_testcapi_unwatch_type_impl(PyObject *module, int watcher_id, PyObject *type)
/*[clinic end generated code: output=0389672d4ad5f68b input=6701911fb45edc9e]*/
{
    if (PyType_Unwatch(watcher_id, type)) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test code object watching

#define NUM_CODE_WATCHERS 2
static int code_watcher_ids[NUM_CODE_WATCHERS] = {-1, -1};
static int num_code_object_created_events[NUM_CODE_WATCHERS] = {0, 0};
static int num_code_object_destroyed_events[NUM_CODE_WATCHERS] = {0, 0};

static int
handle_code_object_event(int which_watcher, PyCodeEvent event, PyCodeObject *co) {
    if (event == PY_CODE_EVENT_CREATE) {
        num_code_object_created_events[which_watcher]++;
    }
    else if (event == PY_CODE_EVENT_DESTROY) {
        num_code_object_destroyed_events[which_watcher]++;
    }
    else {
        return -1;
    }
    return 0;
}

static int
first_code_object_callback(PyCodeEvent event, PyCodeObject *co)
{
    return handle_code_object_event(0, event, co);
}

static int
second_code_object_callback(PyCodeEvent event, PyCodeObject *co)
{
    return handle_code_object_event(1, event, co);
}

static int
noop_code_event_handler(PyCodeEvent event, PyCodeObject *co)
{
    return 0;
}

static int
error_code_event_handler(PyCodeEvent event, PyCodeObject *co)
{
    PyErr_SetString(PyExc_RuntimeError, "boom!");
    return -1;
}

static PyObject *
add_code_watcher(PyObject *self, PyObject *which_watcher)
{
    int watcher_id;
    assert(PyLong_Check(which_watcher));
    long which_l = PyLong_AsLong(which_watcher);
    if (which_l == 0) {
        watcher_id = PyCode_AddWatcher(first_code_object_callback);
        code_watcher_ids[0] = watcher_id;
        num_code_object_created_events[0] = 0;
        num_code_object_destroyed_events[0] = 0;
    }
    else if (which_l == 1) {
        watcher_id = PyCode_AddWatcher(second_code_object_callback);
        code_watcher_ids[1] = watcher_id;
        num_code_object_created_events[1] = 0;
        num_code_object_destroyed_events[1] = 0;
    }
    else if (which_l == 2) {
        watcher_id = PyCode_AddWatcher(error_code_event_handler);
    }
    else {
        PyErr_Format(PyExc_ValueError, "invalid watcher %d", which_l);
        return NULL;
    }
    if (watcher_id < 0) {
        return NULL;
    }
    return PyLong_FromLong(watcher_id);
}

static PyObject *
clear_code_watcher(PyObject *self, PyObject *watcher_id)
{
    assert(PyLong_Check(watcher_id));
    long watcher_id_l = PyLong_AsLong(watcher_id);
    if (PyCode_ClearWatcher(watcher_id_l) < 0) {
        return NULL;
    }
    // reset static events counters
    if (watcher_id_l >= 0) {
        for (int i = 0; i < NUM_CODE_WATCHERS; i++) {
            if (watcher_id_l == code_watcher_ids[i]) {
                code_watcher_ids[i] = -1;
                num_code_object_created_events[i] = 0;
                num_code_object_destroyed_events[i] = 0;
            }
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
get_code_watcher_num_created_events(PyObject *self, PyObject *watcher_id)
{
    assert(PyLong_Check(watcher_id));
    long watcher_id_l = PyLong_AsLong(watcher_id);
    assert(watcher_id_l >= 0 && watcher_id_l < NUM_CODE_WATCHERS);
    return PyLong_FromLong(num_code_object_created_events[watcher_id_l]);
}

static PyObject *
get_code_watcher_num_destroyed_events(PyObject *self, PyObject *watcher_id)
{
    assert(PyLong_Check(watcher_id));
    long watcher_id_l = PyLong_AsLong(watcher_id);
    assert(watcher_id_l >= 0 && watcher_id_l < NUM_CODE_WATCHERS);
    return PyLong_FromLong(num_code_object_destroyed_events[watcher_id_l]);
}

static PyObject *
allocate_too_many_code_watchers(PyObject *self, PyObject *Py_UNUSED(args))
{
    int watcher_ids[CODE_MAX_WATCHERS + 1];
    int num_watchers = 0;
    for (unsigned long i = 0; i < sizeof(watcher_ids) / sizeof(int); i++) {
        int watcher_id = PyCode_AddWatcher(noop_code_event_handler);
        if (watcher_id == -1) {
            break;
        }
        watcher_ids[i] = watcher_id;
        num_watchers++;
    }
    PyObject *exc = PyErr_GetRaisedException();
    for (int i = 0; i < num_watchers; i++) {
        if (PyCode_ClearWatcher(watcher_ids[i]) < 0) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "clearing code watcher");
            break;
        }
    }
    if (exc) {
        PyErr_SetRaisedException(exc);
        return NULL;
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

// Test function watchers

#define NUM_TEST_FUNC_WATCHERS 2
static PyObject *pyfunc_watchers[NUM_TEST_FUNC_WATCHERS];
static int func_watcher_ids[NUM_TEST_FUNC_WATCHERS] = {-1, -1};

static PyObject *
get_id(PyObject *obj)
{
    PyObject *builtins = PyEval_GetBuiltins();  // borrowed ref.
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
call_pyfunc_watcher(PyObject *watcher, PyFunction_WatchEvent event,
                    PyFunctionObject *func, PyObject *new_value)
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
    }
    else {
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
first_func_watcher_callback(PyFunction_WatchEvent event, PyFunctionObject *func,
                            PyObject *new_value)
{
    return call_pyfunc_watcher(pyfunc_watchers[0], event, func, new_value);
}

static int
second_func_watcher_callback(PyFunction_WatchEvent event,
                             PyFunctionObject *func, PyObject *new_value)
{
    return call_pyfunc_watcher(pyfunc_watchers[1], event, func, new_value);
}

static PyFunction_WatchCallback func_watcher_callbacks[NUM_TEST_FUNC_WATCHERS] = {
    first_func_watcher_callback,
    second_func_watcher_callback
};

static int
add_func_event(PyObject *module, const char *name, PyFunction_WatchEvent event)
{
    return PyModule_Add(module, name, PyLong_FromLong(event));
}

static PyObject *
add_func_watcher(PyObject *self, PyObject *func)
{
    if (!PyFunction_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "'func' must be a function");
        return NULL;
    }
    int idx = -1;
    for (int i = 0; i < NUM_TEST_FUNC_WATCHERS; i++) {
        if (func_watcher_ids[i] == -1) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        PyErr_SetString(PyExc_RuntimeError, "no free test watchers");
        return NULL;
    }
    func_watcher_ids[idx] = PyFunction_AddWatcher(func_watcher_callbacks[idx]);
    if (func_watcher_ids[idx] < 0) {
        return NULL;
    }
    pyfunc_watchers[idx] = Py_NewRef(func);
    PyObject *result = PyLong_FromLong(func_watcher_ids[idx]);
    if (result == NULL) {
        return NULL;
    }
    return result;
}

static PyObject *
clear_func_watcher(PyObject *self, PyObject *watcher_id_obj)
{
    long watcher_id = PyLong_AsLong(watcher_id_obj);
    if ((watcher_id < INT_MIN) || (watcher_id > INT_MAX)) {
        PyErr_SetString(PyExc_ValueError, "invalid watcher ID");
        return NULL;
    }
    int wid = (int) watcher_id;
    if (PyFunction_ClearWatcher(wid) < 0) {
        return NULL;
    }
    int idx = -1;
    for (int i = 0; i < NUM_TEST_FUNC_WATCHERS; i++) {
        if (func_watcher_ids[i] == wid) {
            idx = i;
            break;
        }
    }
    assert(idx != -1);
    Py_CLEAR(pyfunc_watchers[idx]);
    func_watcher_ids[idx] = -1;
    Py_RETURN_NONE;
}

static int
noop_func_event_handler(PyFunction_WatchEvent event, PyFunctionObject *func,
             PyObject *new_value)
{
    return 0;
}

static PyObject *
allocate_too_many_func_watchers(PyObject *self, PyObject *args)
{
    int watcher_ids[FUNC_MAX_WATCHERS + 1];
    int num_watchers = 0;
    for (unsigned long i = 0; i < sizeof(watcher_ids) / sizeof(int); i++) {
        int watcher_id = PyFunction_AddWatcher(noop_func_event_handler);
        if (watcher_id == -1) {
            break;
        }
        watcher_ids[i] = watcher_id;
        num_watchers++;
    }
    PyObject *exc = PyErr_GetRaisedException();
    for (int i = 0; i < num_watchers; i++) {
        if (PyFunction_ClearWatcher(watcher_ids[i]) < 0) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "clearing function watcher");
            break;
        }
    }
    if (exc) {
        PyErr_SetRaisedException(exc);
        return NULL;
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

// Test contexct object watchers
#define NUM_CONTEXT_WATCHERS 2
static int context_watcher_ids[NUM_CONTEXT_WATCHERS] = {-1, -1};
static PyObject *context_switches[NUM_CONTEXT_WATCHERS];

static int
handle_context_watcher_event(int which_watcher, PyContextEvent event, PyObject *ctx) {
    if (event == Py_CONTEXT_SWITCHED) {
        PyList_Append(context_switches[which_watcher], ctx);
    }
    else {
        return -1;
    }
    return 0;
}

static int
first_context_watcher_callback(PyContextEvent event, PyObject *ctx) {
    return handle_context_watcher_event(0, event, ctx);
}

static int
second_context_watcher_callback(PyContextEvent event, PyObject *ctx) {
    return handle_context_watcher_event(1, event, ctx);
}

static int
noop_context_event_handler(PyContextEvent event, PyObject *ctx) {
    return 0;
}

static int
error_context_event_handler(PyContextEvent event, PyObject *ctx) {
    PyErr_SetString(PyExc_RuntimeError, "boom!");
    return -1;
}

static PyObject *
add_context_watcher(PyObject *self, PyObject *which_watcher)
{
    static const PyContext_WatchCallback callbacks[] = {
        &first_context_watcher_callback,
        &second_context_watcher_callback,
        &error_context_event_handler,
    };
    assert(PyLong_Check(which_watcher));
    long which_l = PyLong_AsLong(which_watcher);
    if (which_l < 0 || which_l >= (long)Py_ARRAY_LENGTH(callbacks)) {
        PyErr_Format(PyExc_ValueError, "invalid watcher %d", which_l);
        return NULL;
    }
    int watcher_id = PyContext_AddWatcher(callbacks[which_l]);
    if (watcher_id < 0) {
        return NULL;
    }
    if (which_l >= 0 && which_l < NUM_CONTEXT_WATCHERS) {
        context_watcher_ids[which_l] = watcher_id;
        Py_XSETREF(context_switches[which_l], PyList_New(0));
        if (context_switches[which_l] == NULL) {
            return NULL;
        }
    }
    return PyLong_FromLong(watcher_id);
}

static PyObject *
clear_context_watcher(PyObject *self, PyObject *watcher_id)
{
    assert(PyLong_Check(watcher_id));
    long watcher_id_l = PyLong_AsLong(watcher_id);
    if (PyContext_ClearWatcher(watcher_id_l) < 0) {
        return NULL;
    }
    // reset static events counters
    if (watcher_id_l >= 0) {
        for (int i = 0; i < NUM_CONTEXT_WATCHERS; i++) {
            if (watcher_id_l == context_watcher_ids[i]) {
                context_watcher_ids[i] = -1;
                Py_CLEAR(context_switches[i]);
            }
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
clear_context_stack(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyThreadState *tstate = PyThreadState_Get();
    if (tstate->context == NULL) {
        Py_RETURN_NONE;
    }
    if (((PyContext *)tstate->context)->ctx_prev != NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "must first exit all non-base contexts");
        return NULL;
    }
    Py_CLEAR(tstate->context);
    Py_RETURN_NONE;
}

static PyObject *
get_context_switches(PyObject *Py_UNUSED(self), PyObject *watcher_id)
{
    assert(PyLong_Check(watcher_id));
    long watcher_id_l = PyLong_AsLong(watcher_id);
    if (watcher_id_l < 0 || watcher_id_l >= NUM_CONTEXT_WATCHERS) {
        PyErr_Format(PyExc_ValueError, "invalid watcher %ld", watcher_id_l);
        return NULL;
    }
    if (context_switches[watcher_id_l] == NULL) {
        return PyList_New(0);
    }
    return Py_NewRef(context_switches[watcher_id_l]);
}

static PyObject *
allocate_too_many_context_watchers(PyObject *self, PyObject *Py_UNUSED(args))
{
    int watcher_ids[CONTEXT_MAX_WATCHERS + 1];
    int num_watchers = 0;
    for (unsigned long i = 0; i < sizeof(watcher_ids) / sizeof(int); i++) {
        int watcher_id = PyContext_AddWatcher(noop_context_event_handler);
        if (watcher_id == -1) {
            break;
        }
        watcher_ids[i] = watcher_id;
        num_watchers++;
    }
    PyObject *exc = PyErr_GetRaisedException();
    for (int i = 0; i < num_watchers; i++) {
        if (PyContext_ClearWatcher(watcher_ids[i]) < 0) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "clearing context watcher");
            break;
        }
    }
    if (exc) {
        PyErr_SetRaisedException(exc);
        return NULL;
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_testcapi.set_func_defaults_via_capi
    func: object
    defaults: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_set_func_defaults_via_capi_impl(PyObject *module, PyObject *func,
                                          PyObject *defaults)
/*[clinic end generated code: output=caf0cb39db31ac24 input=e04a8508ca9d42fc]*/
{
    if (PyFunction_SetDefaults(func, defaults) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_testcapi.set_func_kwdefaults_via_capi = _testcapi.set_func_defaults_via_capi
[clinic start generated code]*/

static PyObject *
_testcapi_set_func_kwdefaults_via_capi_impl(PyObject *module, PyObject *func,
                                            PyObject *defaults)
/*[clinic end generated code: output=9ed3b08177025070 input=f3cd1ca3c18de8ce]*/
{
    if (PyFunction_SetKwDefaults(func, defaults) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    // Dict watchers.
    {"add_dict_watcher",         add_dict_watcher,        METH_O,       NULL},
    {"clear_dict_watcher",       clear_dict_watcher,      METH_O,       NULL},
    _TESTCAPI_WATCH_DICT_METHODDEF
    _TESTCAPI_UNWATCH_DICT_METHODDEF
    {"get_dict_watcher_events",  get_dict_watcher_events, METH_NOARGS,  NULL},

    // Type watchers.
    {"add_type_watcher",         add_type_watcher,        METH_O,       NULL},
    {"clear_type_watcher",       clear_type_watcher,      METH_O,       NULL},
    _TESTCAPI_WATCH_TYPE_METHODDEF
    _TESTCAPI_UNWATCH_TYPE_METHODDEF
    {"get_type_modified_events",
     get_type_modified_events,                            METH_NOARGS, NULL},

    // Code object watchers.
    {"add_code_watcher",         add_code_watcher,        METH_O,       NULL},
    {"clear_code_watcher",       clear_code_watcher,      METH_O,       NULL},
    {"get_code_watcher_num_created_events",
     get_code_watcher_num_created_events,                 METH_O,       NULL},
    {"get_code_watcher_num_destroyed_events",
     get_code_watcher_num_destroyed_events,               METH_O,       NULL},
    {"allocate_too_many_code_watchers",
     allocate_too_many_code_watchers,                     METH_NOARGS,  NULL},

    // Function watchers.
    {"add_func_watcher",         add_func_watcher,        METH_O,       NULL},
    {"clear_func_watcher",       clear_func_watcher,      METH_O,       NULL},
    _TESTCAPI_SET_FUNC_DEFAULTS_VIA_CAPI_METHODDEF
    _TESTCAPI_SET_FUNC_KWDEFAULTS_VIA_CAPI_METHODDEF
    {"allocate_too_many_func_watchers", allocate_too_many_func_watchers,
     METH_NOARGS, NULL},

    // Code object watchers.
    {"add_context_watcher",         add_context_watcher,        METH_O,       NULL},
    {"clear_context_watcher",       clear_context_watcher,      METH_O,       NULL},
    {"clear_context_stack",      clear_context_stack,     METH_NOARGS,  NULL},
    {"get_context_switches",     get_context_switches,    METH_O,       NULL},
    {"allocate_too_many_context_watchers",
     allocate_too_many_context_watchers,                  METH_NOARGS,  NULL},
    {NULL},
};

int
_PyTestCapi_Init_Watchers(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    /* Expose each event as an attribute on the module */
#define ADD_EVENT(event)  \
    if (add_func_event(mod, "PYFUNC_EVENT_" #event,   \
                       PyFunction_EVENT_##event)) {   \
        return -1;                                    \
    }
    PY_FOREACH_FUNC_EVENT(ADD_EVENT);
#undef ADD_EVENT

    return 0;
}
