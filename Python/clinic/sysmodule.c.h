/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(sys__is_interned__doc__,
"_is_interned($module, string, /)\n"
"--\n"
"\n"
"Return True if the given string is \"interned\".");

#define SYS__IS_INTERNED_METHODDEF    \
    {"_is_interned", (PyCFunction)sys__is_interned, METH_O, sys__is_interned__doc__},

static int
sys__is_interned_impl(PyObject *module, PyObject *string);

static PyObject *
sys__is_interned(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *string;
    int _return_value;

    if (!PyArg_Parse(arg, "U:_is_interned", &string)) {
        goto exit;
    }
    _return_value = sys__is_interned_impl(module, string);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

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
/*[clinic end generated code: output=95761ded68d29915 input=a9049054013a1b77]*/
