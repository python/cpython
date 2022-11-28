/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(monitoring_use_tool__doc__,
"use_tool($module, tool_id, name, /)\n"
"--\n"
"\n");

#define MONITORING_USE_TOOL_METHODDEF    \
    {"use_tool", _PyCFunction_CAST(monitoring_use_tool), METH_FASTCALL, monitoring_use_tool__doc__},

static PyObject *
monitoring_use_tool_impl(PyObject *module, int tool_id, PyObject *name);

static PyObject *
monitoring_use_tool(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int tool_id;
    PyObject *name;

    if (!_PyArg_CheckPositional("use_tool", nargs, 2, 2)) {
        goto exit;
    }
    tool_id = _PyLong_AsInt(args[0]);
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    name = args[1];
    return_value = monitoring_use_tool_impl(module, tool_id, name);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_free_tool__doc__,
"free_tool($module, tool_id, /)\n"
"--\n"
"\n");

#define MONITORING_FREE_TOOL_METHODDEF    \
    {"free_tool", (PyCFunction)monitoring_free_tool, METH_O, monitoring_free_tool__doc__},

static PyObject *
monitoring_free_tool_impl(PyObject *module, int tool_id);

static PyObject *
monitoring_free_tool(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int tool_id;

    tool_id = _PyLong_AsInt(arg);
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = monitoring_free_tool_impl(module, tool_id);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_get_tool__doc__,
"get_tool($module, tool_id, /)\n"
"--\n"
"\n");

#define MONITORING_GET_TOOL_METHODDEF    \
    {"get_tool", (PyCFunction)monitoring_get_tool, METH_O, monitoring_get_tool__doc__},

static PyObject *
monitoring_get_tool_impl(PyObject *module, int tool_id);

static PyObject *
monitoring_get_tool(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int tool_id;

    tool_id = _PyLong_AsInt(arg);
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = monitoring_get_tool_impl(module, tool_id);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_register_callback__doc__,
"register_callback($module, tool_id, event, func, /)\n"
"--\n"
"\n");

#define MONITORING_REGISTER_CALLBACK_METHODDEF    \
    {"register_callback", _PyCFunction_CAST(monitoring_register_callback), METH_FASTCALL, monitoring_register_callback__doc__},

static PyObject *
monitoring_register_callback_impl(PyObject *module, int tool_id, int event,
                                  PyObject *func);

static PyObject *
monitoring_register_callback(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int tool_id;
    int event;
    PyObject *func;

    if (!_PyArg_CheckPositional("register_callback", nargs, 3, 3)) {
        goto exit;
    }
    tool_id = _PyLong_AsInt(args[0]);
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    event = _PyLong_AsInt(args[1]);
    if (event == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = args[2];
    return_value = monitoring_register_callback_impl(module, tool_id, event, func);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_get_events__doc__,
"get_events($module, tool_id, /)\n"
"--\n"
"\n");

#define MONITORING_GET_EVENTS_METHODDEF    \
    {"get_events", (PyCFunction)monitoring_get_events, METH_O, monitoring_get_events__doc__},

static PyObject *
monitoring_get_events_impl(PyObject *module, int tool_id);

static PyObject *
monitoring_get_events(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int tool_id;

    tool_id = _PyLong_AsInt(arg);
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = monitoring_get_events_impl(module, tool_id);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_set_events__doc__,
"set_events($module, tool_id, event_set, /)\n"
"--\n"
"\n");

#define MONITORING_SET_EVENTS_METHODDEF    \
    {"set_events", _PyCFunction_CAST(monitoring_set_events), METH_FASTCALL, monitoring_set_events__doc__},

static PyObject *
monitoring_set_events_impl(PyObject *module, int tool_id, int event_set);

static PyObject *
monitoring_set_events(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int tool_id;
    int event_set;

    if (!_PyArg_CheckPositional("set_events", nargs, 2, 2)) {
        goto exit;
    }
    tool_id = _PyLong_AsInt(args[0]);
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    event_set = _PyLong_AsInt(args[1]);
    if (event_set == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = monitoring_set_events_impl(module, tool_id, event_set);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_restart_events__doc__,
"restart_events($module, /)\n"
"--\n"
"\n");

#define MONITORING_RESTART_EVENTS_METHODDEF    \
    {"restart_events", (PyCFunction)monitoring_restart_events, METH_NOARGS, monitoring_restart_events__doc__},

static PyObject *
monitoring_restart_events_impl(PyObject *module);

static PyObject *
monitoring_restart_events(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return monitoring_restart_events_impl(module);
}
/*[clinic end generated code: output=3997247efd06367f input=a9049054013a1b77]*/
