/*[clinic input]
preserve
[clinic start generated code]*/

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
_lsprof_Profiler_getstats(ProfilerObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "getstats() takes no arguments");
        return NULL;
    }
    return _lsprof_Profiler_getstats_impl(self, cls);
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
_lsprof_Profiler__pystart_callback(ProfilerObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *code;
    PyObject *instruction_offset;

    if (!_PyArg_CheckPositional("_pystart_callback", nargs, 2, 2)) {
        goto exit;
    }
    code = args[0];
    instruction_offset = args[1];
    return_value = _lsprof_Profiler__pystart_callback_impl(self, code, instruction_offset);

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
_lsprof_Profiler__pyreturn_callback(ProfilerObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _lsprof_Profiler__pyreturn_callback_impl(self, code, instruction_offset, retval);

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
_lsprof_Profiler__ccall_callback(ProfilerObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _lsprof_Profiler__ccall_callback_impl(self, code, instruction_offset, callable, self_arg);

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
_lsprof_Profiler__creturn_callback(ProfilerObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _lsprof_Profiler__creturn_callback_impl(self, code, instruction_offset, callable, self_arg);

exit:
    return return_value;
}
/*[clinic end generated code: output=ee788c83b5c85fb1 input=a9049054013a1b77]*/
