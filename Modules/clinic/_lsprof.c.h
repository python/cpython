/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_lsprof_Profiler_getstats__doc__,
"getstats($self, /)\n"
"--\n"
"\n"
"list of profiler_entry objects.\n"
"\n"
"getstats() -> list of profiler_entry objects\n"
"\n"
"Return all information collected by the profiler.\n"
"Each profiler_entry is a tuple-like object with the\n"
"following attributes:\n"
"\n"
"    code          code object\n"
"    callcount     how many times this was called\n"
"    reccallcount  how many times called recursively\n"
"    totaltime     total time in this entry\n"
"    inlinetime    inline time in this entry (not in subcalls)\n"
"    calls         details of the calls\n"
"\n"
"The calls attribute is either None or a list of\n"
"profiler_subentry objects:\n"
"\n"
"    code          called code object\n"
"    callcount     how many times this is called\n"
"    reccallcount  how many times this is called recursively\n"
"    totaltime     total time spent in this call\n"
"    inlinetime    inline time (not in further subcalls)");

#define _LSPROF_PROFILER_GETSTATS_METHODDEF    \
    {"getstats", _PyCFunction_CAST(_lsprof_Profiler_getstats), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _lsprof_Profiler_getstats__doc__},

static PyObject *
_lsprof_Profiler_getstats_impl(ProfilerObject *self, PyTypeObject *cls);

static PyObject *
_lsprof_Profiler_getstats(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "getstats() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _lsprof_Profiler_getstats_impl((ProfilerObject *)self, cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__pystart_callback__doc__,
"_pystart_callback($self, code, instruction_offset, /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__PYSTART_CALLBACK_METHODDEF    \
    {"_pystart_callback", _PyCFunction_CAST(_lsprof_Profiler__pystart_callback), METH_FASTCALL, _lsprof_Profiler__pystart_callback__doc__},

static PyObject *
_lsprof_Profiler__pystart_callback_impl(ProfilerObject *self, PyObject *code,
                                        PyObject *instruction_offset);

static PyObject *
_lsprof_Profiler__pystart_callback(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *code;
    PyObject *instruction_offset;

    if (!_PyArg_CheckPositional("_pystart_callback", nargs, 2, 2)) {
        goto exit;
    }
    code = args[0];
    instruction_offset = args[1];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _lsprof_Profiler__pystart_callback_impl((ProfilerObject *)self, code, instruction_offset);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__pythrow_callback__doc__,
"_pythrow_callback($self, code, instruction_offset, exception, /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__PYTHROW_CALLBACK_METHODDEF    \
    {"_pythrow_callback", _PyCFunction_CAST(_lsprof_Profiler__pythrow_callback), METH_FASTCALL, _lsprof_Profiler__pythrow_callback__doc__},

static PyObject *
_lsprof_Profiler__pythrow_callback_impl(ProfilerObject *self, PyObject *code,
                                        PyObject *instruction_offset,
                                        PyObject *exception);

static PyObject *
_lsprof_Profiler__pythrow_callback(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *code;
    PyObject *instruction_offset;
    PyObject *exception;

    if (!_PyArg_CheckPositional("_pythrow_callback", nargs, 3, 3)) {
        goto exit;
    }
    code = args[0];
    instruction_offset = args[1];
    exception = args[2];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _lsprof_Profiler__pythrow_callback_impl((ProfilerObject *)self, code, instruction_offset, exception);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__pyreturn_callback__doc__,
"_pyreturn_callback($self, code, instruction_offset, retval, /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__PYRETURN_CALLBACK_METHODDEF    \
    {"_pyreturn_callback", _PyCFunction_CAST(_lsprof_Profiler__pyreturn_callback), METH_FASTCALL, _lsprof_Profiler__pyreturn_callback__doc__},

static PyObject *
_lsprof_Profiler__pyreturn_callback_impl(ProfilerObject *self,
                                         PyObject *code,
                                         PyObject *instruction_offset,
                                         PyObject *retval);

static PyObject *
_lsprof_Profiler__pyreturn_callback(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *code;
    PyObject *instruction_offset;
    PyObject *retval;

    if (!_PyArg_CheckPositional("_pyreturn_callback", nargs, 3, 3)) {
        goto exit;
    }
    code = args[0];
    instruction_offset = args[1];
    retval = args[2];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _lsprof_Profiler__pyreturn_callback_impl((ProfilerObject *)self, code, instruction_offset, retval);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__ccall_callback__doc__,
"_ccall_callback($self, code, instruction_offset, callable, self_arg, /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__CCALL_CALLBACK_METHODDEF    \
    {"_ccall_callback", _PyCFunction_CAST(_lsprof_Profiler__ccall_callback), METH_FASTCALL, _lsprof_Profiler__ccall_callback__doc__},

static PyObject *
_lsprof_Profiler__ccall_callback_impl(ProfilerObject *self, PyObject *code,
                                      PyObject *instruction_offset,
                                      PyObject *callable, PyObject *self_arg);

static PyObject *
_lsprof_Profiler__ccall_callback(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *code;
    PyObject *instruction_offset;
    PyObject *callable;
    PyObject *self_arg;

    if (!_PyArg_CheckPositional("_ccall_callback", nargs, 4, 4)) {
        goto exit;
    }
    code = args[0];
    instruction_offset = args[1];
    callable = args[2];
    self_arg = args[3];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _lsprof_Profiler__ccall_callback_impl((ProfilerObject *)self, code, instruction_offset, callable, self_arg);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__creturn_callback__doc__,
"_creturn_callback($self, code, instruction_offset, callable, self_arg,\n"
"                  /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__CRETURN_CALLBACK_METHODDEF    \
    {"_creturn_callback", _PyCFunction_CAST(_lsprof_Profiler__creturn_callback), METH_FASTCALL, _lsprof_Profiler__creturn_callback__doc__},

static PyObject *
_lsprof_Profiler__creturn_callback_impl(ProfilerObject *self, PyObject *code,
                                        PyObject *instruction_offset,
                                        PyObject *callable,
                                        PyObject *self_arg);

static PyObject *
_lsprof_Profiler__creturn_callback(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *code;
    PyObject *instruction_offset;
    PyObject *callable;
    PyObject *self_arg;

    if (!_PyArg_CheckPositional("_creturn_callback", nargs, 4, 4)) {
        goto exit;
    }
    code = args[0];
    instruction_offset = args[1];
    callable = args[2];
    self_arg = args[3];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _lsprof_Profiler__creturn_callback_impl((ProfilerObject *)self, code, instruction_offset, callable, self_arg);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler_enable__doc__,
"enable($self, /, subcalls=True, builtins=True)\n"
"--\n"
"\n"
"Start collecting profiling information.\n"
"\n"
"  subcalls\n"
"    If True, also records for each function\n"
"    statistics separated according to its current caller.\n"
"  builtins\n"
"    If True, records the time spent in\n"
"    built-in functions separately from their caller.");

#define _LSPROF_PROFILER_ENABLE_METHODDEF    \
    {"enable", _PyCFunction_CAST(_lsprof_Profiler_enable), METH_FASTCALL|METH_KEYWORDS, _lsprof_Profiler_enable__doc__},

static PyObject *
_lsprof_Profiler_enable_impl(ProfilerObject *self, int subcalls,
                             int builtins);

static PyObject *
_lsprof_Profiler_enable(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(subcalls), &_Py_ID(builtins), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"subcalls", "builtins", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int subcalls = 1;
    int builtins = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        subcalls = PyObject_IsTrue(args[0]);
        if (subcalls < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    builtins = PyObject_IsTrue(args[1]);
    if (builtins < 0) {
        goto exit;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _lsprof_Profiler_enable_impl((ProfilerObject *)self, subcalls, builtins);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler_disable__doc__,
"disable($self, /)\n"
"--\n"
"\n"
"Stop collecting profiling information.");

#define _LSPROF_PROFILER_DISABLE_METHODDEF    \
    {"disable", (PyCFunction)_lsprof_Profiler_disable, METH_NOARGS, _lsprof_Profiler_disable__doc__},

static PyObject *
_lsprof_Profiler_disable_impl(ProfilerObject *self);

static PyObject *
_lsprof_Profiler_disable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _lsprof_Profiler_disable_impl((ProfilerObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Clear all profiling information collected so far.");

#define _LSPROF_PROFILER_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)_lsprof_Profiler_clear, METH_NOARGS, _lsprof_Profiler_clear__doc__},

static PyObject *
_lsprof_Profiler_clear_impl(ProfilerObject *self);

static PyObject *
_lsprof_Profiler_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _lsprof_Profiler_clear_impl((ProfilerObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(profiler_init__doc__,
"Profiler(timer=None, timeunit=0.0, subcalls=True, builtins=True)\n"
"--\n"
"\n"
"Build a profiler object using the specified timer function.\n"
"\n"
"The default timer is a fast built-in one based on real time.\n"
"For custom timer functions returning integers, \'timeunit\' can\n"
"be a float specifying a scale (that is, how long each integer unit\n"
"is, in seconds).");

static int
profiler_init_impl(ProfilerObject *self, PyObject *timer, double timeunit,
                   int subcalls, int builtins);

static int
profiler_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(timer), &_Py_ID(timeunit), &_Py_ID(subcalls), &_Py_ID(builtins), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"timer", "timeunit", "subcalls", "builtins", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Profiler",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *timer = NULL;
    double timeunit = 0.0;
    int subcalls = 1;
    int builtins = 1;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        timer = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        if (PyFloat_CheckExact(fastargs[1])) {
            timeunit = PyFloat_AS_DOUBLE(fastargs[1]);
        }
        else
        {
            timeunit = PyFloat_AsDouble(fastargs[1]);
            if (timeunit == -1.0 && PyErr_Occurred()) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        subcalls = PyObject_IsTrue(fastargs[2]);
        if (subcalls < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    builtins = PyObject_IsTrue(fastargs[3]);
    if (builtins < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = profiler_init_impl((ProfilerObject *)self, timer, timeunit, subcalls, builtins);

exit:
    return return_value;
}
/*[clinic end generated code: output=af26a0b0ddcc3351 input=a9049054013a1b77]*/
