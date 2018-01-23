/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(sys_set_coroutine_origin_tracking_depth__doc__,
"set_coroutine_origin_tracking_depth($module, /, depth)\n"
"--\n"
"\n"
"Enable or disable origin tracking for coroutine objects in this thread.\n"
"\n"
"Coroutine objects will track \'depth\' frames of traceback information about\n"
"where they came from, available in their cr_origin attribute. Set depth of 0\n"
"to disable.");

#define SYS_SET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF    \
    {"set_coroutine_origin_tracking_depth", (PyCFunction)sys_set_coroutine_origin_tracking_depth, METH_FASTCALL|METH_KEYWORDS, sys_set_coroutine_origin_tracking_depth__doc__},

static PyObject *
sys_set_coroutine_origin_tracking_depth_impl(PyObject *module, int depth);

static PyObject *
sys_set_coroutine_origin_tracking_depth(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"depth", NULL};
    static _PyArg_Parser _parser = {"i:set_coroutine_origin_tracking_depth", _keywords, 0};
    int depth;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &depth)) {
        goto exit;
    }
    return_value = sys_set_coroutine_origin_tracking_depth_impl(module, depth);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_coroutine_origin_tracking_depth__doc__,
"get_coroutine_origin_tracking_depth($module, /)\n"
"--\n"
"\n"
"Check status of origin tracking for coroutine objects in this thread.");

#define SYS_GET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF    \
    {"get_coroutine_origin_tracking_depth", (PyCFunction)sys_get_coroutine_origin_tracking_depth, METH_NOARGS, sys_get_coroutine_origin_tracking_depth__doc__},

static int
sys_get_coroutine_origin_tracking_depth_impl(PyObject *module);

static PyObject *
sys_get_coroutine_origin_tracking_depth(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = sys_get_coroutine_origin_tracking_depth_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_set_unawaited_coroutine_tracking_enabled__doc__,
"set_unawaited_coroutine_tracking_enabled($module, /, enabled)\n"
"--\n"
"\n"
"Enabled or disable tracking of unawaited coroutines.");

#define SYS_SET_UNAWAITED_COROUTINE_TRACKING_ENABLED_METHODDEF    \
    {"set_unawaited_coroutine_tracking_enabled", (PyCFunction)sys_set_unawaited_coroutine_tracking_enabled, METH_FASTCALL|METH_KEYWORDS, sys_set_unawaited_coroutine_tracking_enabled__doc__},

static PyObject *
sys_set_unawaited_coroutine_tracking_enabled_impl(PyObject *module,
                                                  int enabled);

static PyObject *
sys_set_unawaited_coroutine_tracking_enabled(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"enabled", NULL};
    static _PyArg_Parser _parser = {"p:set_unawaited_coroutine_tracking_enabled", _keywords, 0};
    int enabled;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &enabled)) {
        goto exit;
    }
    return_value = sys_set_unawaited_coroutine_tracking_enabled_impl(module, enabled);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_unawaited_coroutine_tracking_enabled__doc__,
"get_unawaited_coroutine_tracking_enabled($module, /)\n"
"--\n"
"\n"
"Enabled or disable tracking of unawaited coroutines.");

#define SYS_GET_UNAWAITED_COROUTINE_TRACKING_ENABLED_METHODDEF    \
    {"get_unawaited_coroutine_tracking_enabled", (PyCFunction)sys_get_unawaited_coroutine_tracking_enabled, METH_NOARGS, sys_get_unawaited_coroutine_tracking_enabled__doc__},

static int
sys_get_unawaited_coroutine_tracking_enabled_impl(PyObject *module);

static PyObject *
sys_get_unawaited_coroutine_tracking_enabled(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = sys_get_unawaited_coroutine_tracking_enabled_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_unawaited_coroutines__doc__,
"get_unawaited_coroutines($module, /)\n"
"--\n"
"\n"
"Get a list of unawaited coroutines, clearing them from the internal list.\n"
"\n"
"Raises an error if unawaited coroutine tracking has not been enabled.");

#define SYS_GET_UNAWAITED_COROUTINES_METHODDEF    \
    {"get_unawaited_coroutines", (PyCFunction)sys_get_unawaited_coroutines, METH_NOARGS, sys_get_unawaited_coroutines__doc__},

static PyObject *
sys_get_unawaited_coroutines_impl(PyObject *module);

static PyObject *
sys_get_unawaited_coroutines(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_get_unawaited_coroutines_impl(module);
}

PyDoc_STRVAR(sys_have_unawaited_coroutines__doc__,
"have_unawaited_coroutines($module, /)\n"
"--\n"
"\n"
"Check whether there are any unawaited coroutines. Never fails.");

#define SYS_HAVE_UNAWAITED_COROUTINES_METHODDEF    \
    {"have_unawaited_coroutines", (PyCFunction)sys_have_unawaited_coroutines, METH_NOARGS, sys_have_unawaited_coroutines__doc__},

static int
sys_have_unawaited_coroutines_impl(PyObject *module);

static PyObject *
sys_have_unawaited_coroutines(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = sys_have_unawaited_coroutines_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=d5a55ad2be66d3ac input=a9049054013a1b77]*/
