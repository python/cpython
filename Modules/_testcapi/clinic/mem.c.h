/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(_testcapi_set_nomemory__doc__,
"set_nomemory($module, start, end=0, /)\n"
"--\n"
"\n");

#define _TESTCAPI_SET_NOMEMORY_METHODDEF    \
    {"set_nomemory", _PyCFunction_CAST(_testcapi_set_nomemory), METH_FASTCALL, _testcapi_set_nomemory__doc__},

static PyObject *
_testcapi_set_nomemory_impl(PyObject *module, int start, int stop);

static PyObject *
_testcapi_set_nomemory(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int start;
    int stop = 0;

    if (!_PyArg_CheckPositional("set_nomemory", nargs, 1, 2)) {
        goto exit;
    }
    start = _PyLong_AsInt(args[0]);
    if (start == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    stop = _PyLong_AsInt(args[1]);
    if (stop == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _testcapi_set_nomemory_impl(module, start, stop);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_remove_mem_hooks__doc__,
"remove_mem_hooks($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_REMOVE_MEM_HOOKS_METHODDEF    \
    {"remove_mem_hooks", (PyCFunction)_testcapi_remove_mem_hooks, METH_NOARGS, _testcapi_remove_mem_hooks__doc__},

static PyObject *
_testcapi_remove_mem_hooks_impl(PyObject *module);

static PyObject *
_testcapi_remove_mem_hooks(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_remove_mem_hooks_impl(module);
}

PyDoc_STRVAR(_testcapi_test_pyobject_setallocators__doc__,
"test_pyobject_setallocators($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_TEST_PYOBJECT_SETALLOCATORS_METHODDEF    \
    {"test_pyobject_setallocators", (PyCFunction)_testcapi_test_pyobject_setallocators, METH_NOARGS, _testcapi_test_pyobject_setallocators__doc__},

static PyObject *
_testcapi_test_pyobject_setallocators_impl(PyObject *module);

static PyObject *
_testcapi_test_pyobject_setallocators(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_test_pyobject_setallocators_impl(module);
}

PyDoc_STRVAR(_testcapi_test_pyobject_new__doc__,
"test_pyobject_new($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_TEST_PYOBJECT_NEW_METHODDEF    \
    {"test_pyobject_new", (PyCFunction)_testcapi_test_pyobject_new, METH_NOARGS, _testcapi_test_pyobject_new__doc__},

static PyObject *
_testcapi_test_pyobject_new_impl(PyObject *module);

static PyObject *
_testcapi_test_pyobject_new(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_test_pyobject_new_impl(module);
}

PyDoc_STRVAR(_testcapi_test_pymem_alloc0__doc__,
"test_pymem_alloc0($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_TEST_PYMEM_ALLOC0_METHODDEF    \
    {"test_pymem_alloc0", (PyCFunction)_testcapi_test_pymem_alloc0, METH_NOARGS, _testcapi_test_pymem_alloc0__doc__},

static PyObject *
_testcapi_test_pymem_alloc0_impl(PyObject *module);

static PyObject *
_testcapi_test_pymem_alloc0(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_test_pymem_alloc0_impl(module);
}

PyDoc_STRVAR(_testcapi_test_pymem_setrawallocators__doc__,
"test_pymem_setrawallocators($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_TEST_PYMEM_SETRAWALLOCATORS_METHODDEF    \
    {"test_pymem_setrawallocators", (PyCFunction)_testcapi_test_pymem_setrawallocators, METH_NOARGS, _testcapi_test_pymem_setrawallocators__doc__},

static PyObject *
_testcapi_test_pymem_setrawallocators_impl(PyObject *module);

static PyObject *
_testcapi_test_pymem_setrawallocators(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_test_pymem_setrawallocators_impl(module);
}

PyDoc_STRVAR(_testcapi_test_pymem_setallocators__doc__,
"test_pymem_setallocators($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_TEST_PYMEM_SETALLOCATORS_METHODDEF    \
    {"test_pymem_setallocators", (PyCFunction)_testcapi_test_pymem_setallocators, METH_NOARGS, _testcapi_test_pymem_setallocators__doc__},

static PyObject *
_testcapi_test_pymem_setallocators_impl(PyObject *module);

static PyObject *
_testcapi_test_pymem_setallocators(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_test_pymem_setallocators_impl(module);
}

PyDoc_STRVAR(_testcapi_pyobject_malloc_without_gil__doc__,
"pyobject_malloc_without_gil($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYOBJECT_MALLOC_WITHOUT_GIL_METHODDEF    \
    {"pyobject_malloc_without_gil", (PyCFunction)_testcapi_pyobject_malloc_without_gil, METH_NOARGS, _testcapi_pyobject_malloc_without_gil__doc__},

static PyObject *
_testcapi_pyobject_malloc_without_gil_impl(PyObject *module);

static PyObject *
_testcapi_pyobject_malloc_without_gil(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_pyobject_malloc_without_gil_impl(module);
}

PyDoc_STRVAR(_testcapi_pymem_buffer_overflow__doc__,
"pymem_buffer_overflow($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYMEM_BUFFER_OVERFLOW_METHODDEF    \
    {"pymem_buffer_overflow", (PyCFunction)_testcapi_pymem_buffer_overflow, METH_NOARGS, _testcapi_pymem_buffer_overflow__doc__},

static PyObject *
_testcapi_pymem_buffer_overflow_impl(PyObject *module);

static PyObject *
_testcapi_pymem_buffer_overflow(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_pymem_buffer_overflow_impl(module);
}

PyDoc_STRVAR(_testcapi_pymem_api_misuse__doc__,
"pymem_api_misuse($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYMEM_API_MISUSE_METHODDEF    \
    {"pymem_api_misuse", (PyCFunction)_testcapi_pymem_api_misuse, METH_NOARGS, _testcapi_pymem_api_misuse__doc__},

static PyObject *
_testcapi_pymem_api_misuse_impl(PyObject *module);

static PyObject *
_testcapi_pymem_api_misuse(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_pymem_api_misuse_impl(module);
}

PyDoc_STRVAR(_testcapi_pymem_malloc_without_gil__doc__,
"pymem_malloc_without_gil($module, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYMEM_MALLOC_WITHOUT_GIL_METHODDEF    \
    {"pymem_malloc_without_gil", (PyCFunction)_testcapi_pymem_malloc_without_gil, METH_NOARGS, _testcapi_pymem_malloc_without_gil__doc__},

static PyObject *
_testcapi_pymem_malloc_without_gil_impl(PyObject *module);

static PyObject *
_testcapi_pymem_malloc_without_gil(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testcapi_pymem_malloc_without_gil_impl(module);
}

PyDoc_STRVAR(_testcapi_tracemalloc_track__doc__,
"tracemalloc_track($module, domain, ptr_obj, size, release_gil=0, /)\n"
"--\n"
"\n");

#define _TESTCAPI_TRACEMALLOC_TRACK_METHODDEF    \
    {"tracemalloc_track", _PyCFunction_CAST(_testcapi_tracemalloc_track), METH_FASTCALL, _testcapi_tracemalloc_track__doc__},

static PyObject *
_testcapi_tracemalloc_track_impl(PyObject *module, unsigned int domain,
                                 PyObject *ptr_obj, Py_ssize_t size,
                                 int release_gil);

static PyObject *
_testcapi_tracemalloc_track(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned int domain;
    PyObject *ptr_obj;
    Py_ssize_t size;
    int release_gil = 0;

    if (!_PyArg_CheckPositional("tracemalloc_track", nargs, 3, 4)) {
        goto exit;
    }
    domain = (unsigned int)PyLong_AsUnsignedLongMask(args[0]);
    if (domain == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
    ptr_obj = args[1];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    release_gil = _PyLong_AsInt(args[3]);
    if (release_gil == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _testcapi_tracemalloc_track_impl(module, domain, ptr_obj, size, release_gil);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_tracemalloc_untrack__doc__,
"tracemalloc_untrack($module, domain, ptr_obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_TRACEMALLOC_UNTRACK_METHODDEF    \
    {"tracemalloc_untrack", _PyCFunction_CAST(_testcapi_tracemalloc_untrack), METH_FASTCALL, _testcapi_tracemalloc_untrack__doc__},

static PyObject *
_testcapi_tracemalloc_untrack_impl(PyObject *module, int domain,
                                   PyObject *ptr_obj);

static PyObject *
_testcapi_tracemalloc_untrack(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int domain;
    PyObject *ptr_obj;

    if (!_PyArg_CheckPositional("tracemalloc_untrack", nargs, 2, 2)) {
        goto exit;
    }
    domain = _PyLong_AsInt(args[0]);
    if (domain == -1 && PyErr_Occurred()) {
        goto exit;
    }
    ptr_obj = args[1];
    return_value = _testcapi_tracemalloc_untrack_impl(module, domain, ptr_obj);

exit:
    return return_value;
}
/*[clinic end generated code: output=f76fed573d5d653f input=a9049054013a1b77]*/
