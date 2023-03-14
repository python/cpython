/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(syslog_openlog__doc__,
"openlog($module, /, ident=<unrepresentable>, logoption=0,\n"
"        facility=LOG_USER)\n"
"--\n"
"\n"
"Set logging options of subsequent syslog() calls.");

#define SYSLOG_OPENLOG_METHODDEF    \
    {"openlog", _PyCFunction_CAST(syslog_openlog), METH_FASTCALL|METH_KEYWORDS, syslog_openlog__doc__},

static PyObject *
syslog_openlog_impl(PyObject *module, PyObject *ident, long logopt,
                    long facility);

static PyObject *
syslog_openlog(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(ident), &_Py_ID(logoption), &_Py_ID(facility), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"ident", "logoption", "facility", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "openlog",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *ident = NULL;
    long logopt = 0;
    long facility = LOG_USER;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        if (!PyUnicode_Check(args[0])) {
            _PyArg_BadArgument("openlog", "argument 'ident'", "str", args[0]);
            goto exit;
        }
        if (PyUnicode_READY(args[0]) == -1) {
            goto exit;
        }
        ident = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        logopt = PyLong_AsLong(args[1]);
        if (logopt == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    facility = PyLong_AsLong(args[2]);
    if (facility == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = syslog_openlog_impl(module, ident, logopt, facility);

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_syslog__doc__,
"syslog([priority=LOG_INFO,] message)\n"
"Send the string message to the system logger.");

#define SYSLOG_SYSLOG_METHODDEF    \
    {"syslog", (PyCFunction)syslog_syslog, METH_VARARGS, syslog_syslog__doc__},

static PyObject *
syslog_syslog_impl(PyObject *module, int group_left_1, int priority,
                   const char *message);

static PyObject *
syslog_syslog(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int priority = LOG_INFO;
    const char *message;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "s:syslog", &message)) {
                goto exit;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "is:syslog", &priority, &message)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "syslog.syslog requires 1 to 2 arguments");
            goto exit;
    }
    return_value = syslog_syslog_impl(module, group_left_1, priority, message);

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_closelog__doc__,
"closelog($module, /)\n"
"--\n"
"\n"
"Reset the syslog module values and call the system library closelog().");

#define SYSLOG_CLOSELOG_METHODDEF    \
    {"closelog", (PyCFunction)syslog_closelog, METH_NOARGS, syslog_closelog__doc__},

static PyObject *
syslog_closelog_impl(PyObject *module);

static PyObject *
syslog_closelog(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return syslog_closelog_impl(module);
}

PyDoc_STRVAR(syslog_setlogmask__doc__,
"setlogmask($module, maskpri, /)\n"
"--\n"
"\n"
"Set the priority mask to maskpri and return the previous mask value.");

#define SYSLOG_SETLOGMASK_METHODDEF    \
    {"setlogmask", (PyCFunction)syslog_setlogmask, METH_O, syslog_setlogmask__doc__},

static long
syslog_setlogmask_impl(PyObject *module, long maskpri);

static PyObject *
syslog_setlogmask(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    long maskpri;
    long _return_value;

    maskpri = PyLong_AsLong(arg);
    if (maskpri == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = syslog_setlogmask_impl(module, maskpri);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_LOG_MASK__doc__,
"LOG_MASK($module, pri, /)\n"
"--\n"
"\n"
"Calculates the mask for the individual priority pri.");

#define SYSLOG_LOG_MASK_METHODDEF    \
    {"LOG_MASK", (PyCFunction)syslog_LOG_MASK, METH_O, syslog_LOG_MASK__doc__},

static long
syslog_LOG_MASK_impl(PyObject *module, long pri);

static PyObject *
syslog_LOG_MASK(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    long pri;
    long _return_value;

    pri = PyLong_AsLong(arg);
    if (pri == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = syslog_LOG_MASK_impl(module, pri);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(syslog_LOG_UPTO__doc__,
"LOG_UPTO($module, pri, /)\n"
"--\n"
"\n"
"Calculates the mask for all priorities up to and including pri.");

#define SYSLOG_LOG_UPTO_METHODDEF    \
    {"LOG_UPTO", (PyCFunction)syslog_LOG_UPTO, METH_O, syslog_LOG_UPTO__doc__},

static long
syslog_LOG_UPTO_impl(PyObject *module, long pri);

static PyObject *
syslog_LOG_UPTO(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    long pri;
    long _return_value;

    pri = PyLong_AsLong(arg);
    if (pri == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = syslog_LOG_UPTO_impl(module, pri);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=3b1bdb16565b8fda input=a9049054013a1b77]*/
