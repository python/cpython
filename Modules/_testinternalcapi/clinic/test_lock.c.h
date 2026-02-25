/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_testinternalcapi_benchmark_locks__doc__,
"benchmark_locks($module, /, num_threads, work_inside=1, work_outside=0,\n"
"                time_ms=1000, num_acquisitions=1, total_iters=0,\n"
"                num_locks=1, random_locks=False)\n"
"--\n"
"\n");

#define _TESTINTERNALCAPI_BENCHMARK_LOCKS_METHODDEF    \
    {"benchmark_locks", _PyCFunction_CAST(_testinternalcapi_benchmark_locks), METH_FASTCALL|METH_KEYWORDS, _testinternalcapi_benchmark_locks__doc__},

static PyObject *
_testinternalcapi_benchmark_locks_impl(PyObject *module,
                                       Py_ssize_t num_threads,
                                       int work_inside, int work_outside,
                                       int time_ms, int num_acquisitions,
                                       Py_ssize_t total_iters,
                                       Py_ssize_t num_locks,
                                       int random_locks);

static PyObject *
_testinternalcapi_benchmark_locks(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 8
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(num_threads), &_Py_ID(work_inside), &_Py_ID(work_outside), &_Py_ID(time_ms), &_Py_ID(num_acquisitions), &_Py_ID(total_iters), &_Py_ID(num_locks), &_Py_ID(random_locks), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"num_threads", "work_inside", "work_outside", "time_ms", "num_acquisitions", "total_iters", "num_locks", "random_locks", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "benchmark_locks",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[8];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_ssize_t num_threads;
    int work_inside = 1;
    int work_outside = 0;
    int time_ms = 1000;
    int num_acquisitions = 1;
    Py_ssize_t total_iters = 0;
    Py_ssize_t num_locks = 1;
    int random_locks = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 8, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
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
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        work_inside = PyLong_AsInt(args[1]);
        if (work_inside == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        work_outside = PyLong_AsInt(args[2]);
        if (work_outside == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        time_ms = PyLong_AsInt(args[3]);
        if (time_ms == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[4]) {
        num_acquisitions = PyLong_AsInt(args[4]);
        if (num_acquisitions == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[5]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[5]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            total_iters = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[6]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[6]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            num_locks = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    random_locks = PyObject_IsTrue(args[7]);
    if (random_locks < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = _testinternalcapi_benchmark_locks_impl(module, num_threads, work_inside, work_outside, time_ms, num_acquisitions, total_iters, num_locks, random_locks);

exit:
    return return_value;
}
/*[clinic end generated code: output=0aef3a96b1a2f317 input=a9049054013a1b77]*/
