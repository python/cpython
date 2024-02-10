/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

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
_lsprof_Profiler_getstats(ProfilerObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "getstats() takes no arguments");
        return NULL;
    }
    return _lsprof_Profiler_getstats_impl(self, cls);
}

PyDoc_STRVAR(_lsprof_Profiler__pystart_callback__doc__,
"_pystart_callback($self, /, code, instruction_offset)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__PYSTART_CALLBACK_METHODDEF    \
    {"_pystart_callback", _PyCFunction_CAST(_lsprof_Profiler__pystart_callback), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _lsprof_Profiler__pystart_callback__doc__},

static PyObject *
_lsprof_Profiler__pystart_callback_impl(ProfilerObject *self,
                                        PyTypeObject *cls, PyObject *code,
                                        int instruction_offset);

static PyObject *
_lsprof_Profiler__pystart_callback(ProfilerObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(code), &_Py_ID(instruction_offset), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"code", "instruction_offset", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_pystart_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *code;
    int instruction_offset;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    code = args[0];
    instruction_offset = PyLong_AsInt(args[1]);
    if (instruction_offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _lsprof_Profiler__pystart_callback_impl(self, cls, code, instruction_offset);

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__pyreturn_callback__doc__,
"_pyreturn_callback($self, /, code, instruction_offset, retval)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__PYRETURN_CALLBACK_METHODDEF    \
    {"_pyreturn_callback", _PyCFunction_CAST(_lsprof_Profiler__pyreturn_callback), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _lsprof_Profiler__pyreturn_callback__doc__},

static PyObject *
_lsprof_Profiler__pyreturn_callback_impl(ProfilerObject *self,
                                         PyTypeObject *cls, PyObject *code,
                                         int instruction_offset,
                                         PyObject *retval);

static PyObject *
_lsprof_Profiler__pyreturn_callback(ProfilerObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(code), &_Py_ID(instruction_offset), &_Py_ID(retval), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"code", "instruction_offset", "retval", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_pyreturn_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject *code;
    int instruction_offset;
    PyObject *retval;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    code = args[0];
    instruction_offset = PyLong_AsInt(args[1]);
    if (instruction_offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    retval = args[2];
    return_value = _lsprof_Profiler__pyreturn_callback_impl(self, cls, code, instruction_offset, retval);

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__ccall_callback__doc__,
"_ccall_callback($self, /, code, instruction_offset, callable, self_arg)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__CCALL_CALLBACK_METHODDEF    \
    {"_ccall_callback", _PyCFunction_CAST(_lsprof_Profiler__ccall_callback), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _lsprof_Profiler__ccall_callback__doc__},

static PyObject *
_lsprof_Profiler__ccall_callback_impl(ProfilerObject *self,
                                      PyTypeObject *cls, PyObject *code,
                                      int instruction_offset,
                                      PyObject *callable, PyObject *self_arg);

static PyObject *
_lsprof_Profiler__ccall_callback(ProfilerObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(code), &_Py_ID(instruction_offset), &_Py_ID(callable), &_Py_ID(self_arg), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"code", "instruction_offset", "callable", "self_arg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_ccall_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject *code;
    int instruction_offset;
    PyObject *callable;
    PyObject *self_arg;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 4, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    code = args[0];
    instruction_offset = PyLong_AsInt(args[1]);
    if (instruction_offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    callable = args[2];
    self_arg = args[3];
    return_value = _lsprof_Profiler__ccall_callback_impl(self, cls, code, instruction_offset, callable, self_arg);

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__creturn_callback__doc__,
"_creturn_callback($self, /, code, instruction_offset, callable,\n"
"                  self_arg)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__CRETURN_CALLBACK_METHODDEF    \
    {"_creturn_callback", _PyCFunction_CAST(_lsprof_Profiler__creturn_callback), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _lsprof_Profiler__creturn_callback__doc__},

static PyObject *
_lsprof_Profiler__creturn_callback_impl(ProfilerObject *self,
                                        PyTypeObject *cls, PyObject *code,
                                        int instruction_offset,
                                        PyObject *callable,
                                        PyObject *self_arg);

static PyObject *
_lsprof_Profiler__creturn_callback(ProfilerObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(code), &_Py_ID(instruction_offset), &_Py_ID(callable), &_Py_ID(self_arg), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"code", "instruction_offset", "callable", "self_arg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_creturn_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject *code;
    int instruction_offset;
    PyObject *callable;
    PyObject *self_arg;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 4, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    code = args[0];
    instruction_offset = PyLong_AsInt(args[1]);
    if (instruction_offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    callable = args[2];
    self_arg = args[3];
    return_value = _lsprof_Profiler__creturn_callback_impl(self, cls, code, instruction_offset, callable, self_arg);

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler_enable__doc__,
"enable($self, /, subclass=True, builtins=True)\n"
"--\n"
"\n"
"Start collecting profiling information.\n"
"\n"
"If \'subcalls\' is True, also records for each function\n"
"statistics separated according to its current caller.\n"
"If \'builtins\' is True, records the time spent in\n"
"built-in functions separately from their caller.");

#define _LSPROF_PROFILER_ENABLE_METHODDEF    \
    {"enable", _PyCFunction_CAST(_lsprof_Profiler_enable), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _lsprof_Profiler_enable__doc__},

static PyObject *
_lsprof_Profiler_enable_impl(ProfilerObject *self, PyTypeObject *cls,
                             int subclass, int builtins);

static PyObject *
_lsprof_Profiler_enable(ProfilerObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(subclass), &_Py_ID(builtins), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"subclass", "builtins", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int subclass = 1;
    int builtins = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        subclass = PyObject_IsTrue(args[0]);
        if (subclass < 0) {
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
    return_value = _lsprof_Profiler_enable_impl(self, cls, subclass, builtins);

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler_disable__doc__,
"disable($self, /)\n"
"--\n"
"\n"
"Stop collecting profiling information.");

#define _LSPROF_PROFILER_DISABLE_METHODDEF    \
    {"disable", _PyCFunction_CAST(_lsprof_Profiler_disable), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _lsprof_Profiler_disable__doc__},

static PyObject *
_lsprof_Profiler_disable_impl(ProfilerObject *self, PyTypeObject *cls);

static PyObject *
_lsprof_Profiler_disable(ProfilerObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "disable() takes no arguments");
        return NULL;
    }
    return _lsprof_Profiler_disable_impl(self, cls);
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
_lsprof_Profiler_clear(ProfilerObject *self, PyObject *Py_UNUSED(ignored))
{
    return _lsprof_Profiler_clear_impl(self);
}
/*[clinic end generated code: output=93734254d71b6773 input=a9049054013a1b77]*/
