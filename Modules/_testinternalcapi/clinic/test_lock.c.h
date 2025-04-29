/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testinternalcapi_benchmark_locks__doc__,
"benchmark_locks($module, num_threads, use_pymutex=True,\n"
"                critical_section_length=1, time_ms=1000, /)\n"
"--\n"
"\n");

#define _TESTINTERNALCAPI_BENCHMARK_LOCKS_METHODDEF    \
    {"benchmark_locks", _PyCFunction_CAST(_testinternalcapi_benchmark_locks), METH_FASTCALL, _testinternalcapi_benchmark_locks__doc__},

static PyObject *
_testinternalcapi_benchmark_locks_impl(PyObject *module,
                                       Py_ssize_t num_threads,
                                       int use_pymutex,
                                       int critical_section_length,
                                       int time_ms);

static PyObject *
_testinternalcapi_benchmark_locks(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t num_threads;
    int use_pymutex = 1;
    int critical_section_length = 1;
    int time_ms = 1000;

    if (!_PyArg_CheckPositional("benchmark_locks", nargs, 1, 4)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        num_threads = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    use_pymutex = PyObject_IsTrue(args[1]);
    if (use_pymutex < 0) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    critical_section_length = PyLong_AsInt(args[2]);
    if (critical_section_length == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    time_ms = PyLong_AsInt(args[3]);
    if (time_ms == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _testinternalcapi_benchmark_locks_impl(module, num_threads, use_pymutex, critical_section_length, time_ms);

exit:
    return return_value;
}
/*[clinic end generated code: output=105105d759c0c271 input=a9049054013a1b77]*/
