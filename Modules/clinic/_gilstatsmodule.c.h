/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_long.h"          // _PyLong_UnsignedInt_Converter()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_gilstats_enable__doc__,
"enable($module, /, *, ring_size=65536, record_threshold_us=100,\n"
"       drain_interval_min=0.01, drain_interval_max=2.0)\n"
"--\n"
"\n"
"Start collecting GIL contention statistics.\n"
"\n"
"  ring_size\n"
"    Capacity of the event ring (a power of two).  Events that do not\n"
"    fit between two drains are dropped and counted in \"dropped\".\n"
"  record_threshold_us\n"
"    Contended waits shorter than this are only counted\n"
"    (\"below_threshold\"), not recorded individually.  0 records all.\n"
"  drain_interval_max\n"
"    Bounds, in seconds, of the adaptive interval at which the\n"
"    aggregator thread drains the ring.\n"
"\n"
"If statistics are already enabled, they are restarted with the new\n"
"settings and the current window is discarded.");

#define _GILSTATS_ENABLE_METHODDEF    \
    {"enable", _PyCFunction_CAST(_gilstats_enable), METH_FASTCALL|METH_KEYWORDS, _gilstats_enable__doc__},

static PyObject *
_gilstats_enable_impl(PyObject *module, unsigned int ring_size,
                      unsigned int record_threshold_us,
                      double drain_interval_min, double drain_interval_max);

static PyObject *
_gilstats_enable(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(ring_size), &_Py_ID(record_threshold_us), &_Py_ID(drain_interval_min), &_Py_ID(drain_interval_max), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"ring_size", "record_threshold_us", "drain_interval_min", "drain_interval_max", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    unsigned int ring_size = 65536;
    unsigned int record_threshold_us = 100;
    double drain_interval_min = 0.01;
    double drain_interval_max = 2.0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[0]) {
        if (!_PyLong_UnsignedInt_Converter(args[0], &ring_size)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[1]) {
        if (!_PyLong_UnsignedInt_Converter(args[1], &record_threshold_us)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        if (PyFloat_CheckExact(args[2])) {
            drain_interval_min = PyFloat_AS_DOUBLE(args[2]);
        }
        else
        {
            drain_interval_min = PyFloat_AsDouble(args[2]);
            if (drain_interval_min == -1.0 && PyErr_Occurred()) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (PyFloat_CheckExact(args[3])) {
        drain_interval_max = PyFloat_AS_DOUBLE(args[3]);
    }
    else
    {
        drain_interval_max = PyFloat_AsDouble(args[3]);
        if (drain_interval_max == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
skip_optional_kwonly:
    return_value = _gilstats_enable_impl(module, ring_size, record_threshold_us, drain_interval_min, drain_interval_max);

exit:
    return return_value;
}

PyDoc_STRVAR(_gilstats_disable__doc__,
"disable($module, /)\n"
"--\n"
"\n"
"Stop collecting GIL contention statistics.\n"
"\n"
"Stops and joins the aggregator thread.  The callback and the last\n"
"published snapshot are kept.");

#define _GILSTATS_DISABLE_METHODDEF    \
    {"disable", (PyCFunction)_gilstats_disable, METH_NOARGS, _gilstats_disable__doc__},

static PyObject *
_gilstats_disable_impl(PyObject *module);

static PyObject *
_gilstats_disable(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _gilstats_disable_impl(module);
}

PyDoc_STRVAR(_gilstats_is_enabled__doc__,
"is_enabled($module, /)\n"
"--\n"
"\n"
"Return True if GIL contention statistics are being collected.");

#define _GILSTATS_IS_ENABLED_METHODDEF    \
    {"is_enabled", (PyCFunction)_gilstats_is_enabled, METH_NOARGS, _gilstats_is_enabled__doc__},

static int
_gilstats_is_enabled_impl(PyObject *module);

static PyObject *
_gilstats_is_enabled(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _gilstats_is_enabled_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_gilstats_set_callback__doc__,
"set_callback($module, callback, /, period=60.0, min_events=1000,\n"
"             max_period=600.0)\n"
"--\n"
"\n"
"Set the function called with each published window.\n"
"\n"
"  callback\n"
"    Called as callback(snapshot) from the aggregator thread, with the\n"
"    GIL held, each time a window is published.  None removes it.\n"
"  period\n"
"    Minimum window length in seconds.\n"
"  min_events\n"
"    Minimum number of contended waits before a window is published.\n"
"  max_period\n"
"    A window is published after this many seconds regardless of\n"
"    min_events.\n"
"\n"
"The window settings apply whether or not a callback is set; they also\n"
"control what snapshot() returns.");

#define _GILSTATS_SET_CALLBACK_METHODDEF    \
    {"set_callback", _PyCFunction_CAST(_gilstats_set_callback), METH_FASTCALL|METH_KEYWORDS, _gilstats_set_callback__doc__},

static PyObject *
_gilstats_set_callback_impl(PyObject *module, PyObject *callback,
                            double period, unsigned long long min_events,
                            double max_period);

static PyObject *
_gilstats_set_callback(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(period), &_Py_ID(min_events), &_Py_ID(max_period), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "period", "min_events", "max_period", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *callback;
    double period = 60.0;
    unsigned long long min_events = 1000;
    double max_period = 600.0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    callback = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        if (PyFloat_CheckExact(args[1])) {
            period = PyFloat_AS_DOUBLE(args[1]);
        }
        else
        {
            period = PyFloat_AsDouble(args[1]);
            if (period == -1.0 && PyErr_Occurred()) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        if (!_PyLong_UnsignedLongLong_Converter(args[2], &min_events)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (PyFloat_CheckExact(args[3])) {
        max_period = PyFloat_AS_DOUBLE(args[3]);
    }
    else
    {
        max_period = PyFloat_AsDouble(args[3]);
        if (max_period == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
skip_optional_pos:
    return_value = _gilstats_set_callback_impl(module, callback, period, min_events, max_period);

exit:
    return return_value;
}

PyDoc_STRVAR(_gilstats_snapshot__doc__,
"snapshot($module, /)\n"
"--\n"
"\n"
"Return the last published window as a dict, or None.");

#define _GILSTATS_SNAPSHOT_METHODDEF    \
    {"snapshot", (PyCFunction)_gilstats_snapshot, METH_NOARGS, _gilstats_snapshot__doc__},

static PyObject *
_gilstats_snapshot_impl(PyObject *module);

static PyObject *
_gilstats_snapshot(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _gilstats_snapshot_impl(module);
}

PyDoc_STRVAR(_gilstats__summarize__doc__,
"_summarize($module, waits, /, below_threshold=0, threshold_us=0)\n"
"--\n"
"\n"
"Summarize synthetic waits with the aggregator\'s code (for tests).\n"
"\n"
"  waits\n"
"    Iterable of wait_us ints or (wait_us, waiters_at_entry) tuples.");

#define _GILSTATS__SUMMARIZE_METHODDEF    \
    {"_summarize", _PyCFunction_CAST(_gilstats__summarize), METH_FASTCALL|METH_KEYWORDS, _gilstats__summarize__doc__},

static PyObject *
_gilstats__summarize_impl(PyObject *module, PyObject *waits,
                          unsigned long long below_threshold,
                          unsigned int threshold_us);

static PyObject *
_gilstats__summarize(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(below_threshold), &_Py_ID(threshold_us), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "below_threshold", "threshold_us", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_summarize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *waits;
    unsigned long long below_threshold = 0;
    unsigned int threshold_us = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    waits = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        if (!_PyLong_UnsignedLongLong_Converter(args[1], &below_threshold)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!_PyLong_UnsignedInt_Converter(args[2], &threshold_us)) {
        goto exit;
    }
skip_optional_pos:
    return_value = _gilstats__summarize_impl(module, waits, below_threshold, threshold_us);

exit:
    return return_value;
}
/*[clinic end generated code: output=e1f160d0442fb2b6 input=a9049054013a1b77]*/
