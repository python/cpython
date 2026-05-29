/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_tracemalloc_is_tracing__doc__,
"is_tracing($module, /)\n"
"--\n"
"\n"
"Return True if the tracemalloc module is tracing Python memory allocations.");

#define _TRACEMALLOC_IS_TRACING_METHODDEF    \
    {"is_tracing", (PyCFunction)_tracemalloc_is_tracing, METH_NOARGS, _tracemalloc_is_tracing__doc__},

static PyObject *
_tracemalloc_is_tracing_impl(PyObject *module);

static PyObject *
_tracemalloc_is_tracing(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _tracemalloc_is_tracing_impl(module);
}

PyDoc_STRVAR(_tracemalloc_clear_traces__doc__,
"clear_traces($module, /)\n"
"--\n"
"\n"
"Clear traces of memory blocks allocated by Python.");

#define _TRACEMALLOC_CLEAR_TRACES_METHODDEF    \
    {"clear_traces", (PyCFunction)_tracemalloc_clear_traces, METH_NOARGS, _tracemalloc_clear_traces__doc__},

static PyObject *
_tracemalloc_clear_traces_impl(PyObject *module);

static PyObject *
_tracemalloc_clear_traces(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _tracemalloc_clear_traces_impl(module);
}

PyDoc_STRVAR(_tracemalloc__get_traces__doc__,
"_get_traces($module, /)\n"
"--\n"
"\n"
"Get traces of all memory blocks allocated by Python.\n"
"\n"
"Return a list of (size: int, traceback: tuple) tuples.\n"
"traceback is a tuple of (filename: str, lineno: int) tuples.\n"
"\n"
"Return an empty list if the tracemalloc module is disabled.");

#define _TRACEMALLOC__GET_TRACES_METHODDEF    \
    {"_get_traces", (PyCFunction)_tracemalloc__get_traces, METH_NOARGS, _tracemalloc__get_traces__doc__},

static PyObject *
_tracemalloc__get_traces_impl(PyObject *module);

static PyObject *
_tracemalloc__get_traces(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _tracemalloc__get_traces_impl(module);
}

PyDoc_STRVAR(_tracemalloc__get_object_traceback__doc__,
"_get_object_traceback($module, obj, /)\n"
"--\n"
"\n"
"Get the traceback where the Python object obj was allocated.\n"
"\n"
"Return a tuple of (filename: str, lineno: int) tuples.\n"
"Return None if the tracemalloc module is disabled or did not\n"
"trace the allocation of the object.");

#define _TRACEMALLOC__GET_OBJECT_TRACEBACK_METHODDEF    \
    {"_get_object_traceback", (PyCFunction)_tracemalloc__get_object_traceback, METH_O, _tracemalloc__get_object_traceback__doc__},

PyDoc_STRVAR(_tracemalloc_start__doc__,
"start($module, /, nframe=1, sample_interval=0)\n"
"--\n"
"\n"
"Start tracing Python memory allocations.\n"
"\n"
"Also set the maximum number of frames stored in the traceback of a\n"
"trace to nframe.\n"
"\n"
"If sample_interval is 0, every allocation is traced.  If sample_interval\n"
"is N > 0, allocations are sampled using a Poisson process with a mean\n"
"inter-arrival of N bytes.  In sampled mode, Trace.size is an upscaled\n"
"estimate of the bytes represented by the sample, and Trace.real_size is\n"
"the actual allocation size.");

#define _TRACEMALLOC_START_METHODDEF    \
    {"start", _PyCFunction_CAST(_tracemalloc_start), METH_FASTCALL|METH_KEYWORDS, _tracemalloc_start__doc__},

static PyObject *
_tracemalloc_start_impl(PyObject *module, int nframe,
                        Py_ssize_t sample_interval);

static PyObject *
_tracemalloc_start(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(nframe), &_Py_ID(sample_interval), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"nframe", "sample_interval", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "start",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int nframe = 1;
    Py_ssize_t sample_interval = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        nframe = PyLong_AsInt(args[0]);
        if (nframe == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        sample_interval = ival;
    }
skip_optional_pos:
    return_value = _tracemalloc_start_impl(module, nframe, sample_interval);

exit:
    return return_value;
}

PyDoc_STRVAR(_tracemalloc_stop__doc__,
"stop($module, /)\n"
"--\n"
"\n"
"Stop tracing Python memory allocations.\n"
"\n"
"Also clear traces of memory blocks allocated by Python.");

#define _TRACEMALLOC_STOP_METHODDEF    \
    {"stop", (PyCFunction)_tracemalloc_stop, METH_NOARGS, _tracemalloc_stop__doc__},

static PyObject *
_tracemalloc_stop_impl(PyObject *module);

static PyObject *
_tracemalloc_stop(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _tracemalloc_stop_impl(module);
}

PyDoc_STRVAR(_tracemalloc_get_traceback_limit__doc__,
"get_traceback_limit($module, /)\n"
"--\n"
"\n"
"Get the maximum number of frames stored in the traceback of a trace.\n"
"\n"
"By default, a trace of an allocated memory block only stores\n"
"the most recent frame: the limit is 1.");

#define _TRACEMALLOC_GET_TRACEBACK_LIMIT_METHODDEF    \
    {"get_traceback_limit", (PyCFunction)_tracemalloc_get_traceback_limit, METH_NOARGS, _tracemalloc_get_traceback_limit__doc__},

static PyObject *
_tracemalloc_get_traceback_limit_impl(PyObject *module);

static PyObject *
_tracemalloc_get_traceback_limit(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _tracemalloc_get_traceback_limit_impl(module);
}

PyDoc_STRVAR(_tracemalloc_get_tracemalloc_memory__doc__,
"get_tracemalloc_memory($module, /)\n"
"--\n"
"\n"
"Get the memory usage in bytes of the tracemalloc module.\n"
"\n"
"This memory is used internally to trace memory allocations.");

#define _TRACEMALLOC_GET_TRACEMALLOC_MEMORY_METHODDEF    \
    {"get_tracemalloc_memory", (PyCFunction)_tracemalloc_get_tracemalloc_memory, METH_NOARGS, _tracemalloc_get_tracemalloc_memory__doc__},

static PyObject *
_tracemalloc_get_tracemalloc_memory_impl(PyObject *module);

static PyObject *
_tracemalloc_get_tracemalloc_memory(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _tracemalloc_get_tracemalloc_memory_impl(module);
}

PyDoc_STRVAR(_tracemalloc_get_traced_memory__doc__,
"get_traced_memory($module, /)\n"
"--\n"
"\n"
"Get the current size and peak size of memory blocks traced by tracemalloc.\n"
"\n"
"Returns a tuple: (current: int, peak: int).");

#define _TRACEMALLOC_GET_TRACED_MEMORY_METHODDEF    \
    {"get_traced_memory", (PyCFunction)_tracemalloc_get_traced_memory, METH_NOARGS, _tracemalloc_get_traced_memory__doc__},

static PyObject *
_tracemalloc_get_traced_memory_impl(PyObject *module);

static PyObject *
_tracemalloc_get_traced_memory(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _tracemalloc_get_traced_memory_impl(module);
}

PyDoc_STRVAR(_tracemalloc_reset_peak__doc__,
"reset_peak($module, /)\n"
"--\n"
"\n"
"Set the peak size of memory blocks traced by tracemalloc to the current size.\n"
"\n"
"Do nothing if the tracemalloc module is not tracing memory allocations.");

#define _TRACEMALLOC_RESET_PEAK_METHODDEF    \
    {"reset_peak", (PyCFunction)_tracemalloc_reset_peak, METH_NOARGS, _tracemalloc_reset_peak__doc__},

static PyObject *
_tracemalloc_reset_peak_impl(PyObject *module);

static PyObject *
_tracemalloc_reset_peak(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _tracemalloc_reset_peak_impl(module);
}
/*[clinic end generated code: output=304f592415e8c883 input=a9049054013a1b77]*/
