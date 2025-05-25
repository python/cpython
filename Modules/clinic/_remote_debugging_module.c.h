/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder___init____doc__,
"RemoteUnwinder(pid, *, all_threads=False)\n"
"--\n"
"\n"
"Something");

static int
_remote_debugging_RemoteUnwinder___init___impl(RemoteUnwinderObject *self,
                                               int pid, int all_threads);

static int
_remote_debugging_RemoteUnwinder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
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
        .ob_item = { &_Py_ID(pid), &_Py_ID(all_threads), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "all_threads", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "RemoteUnwinder",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    int pid;
    int all_threads = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    pid = PyLong_AsInt(fastargs[0]);
    if (pid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    all_threads = PyObject_IsTrue(fastargs[1]);
    if (all_threads < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _remote_debugging_RemoteUnwinder___init___impl((RemoteUnwinderObject *)self, pid, all_threads);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_stack_trace__doc__,
"get_stack_trace($self, /)\n"
"--\n"
"\n"
"Blah blah blah");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_STACK_TRACE_METHODDEF    \
    {"get_stack_trace", (PyCFunction)_remote_debugging_RemoteUnwinder_get_stack_trace, METH_NOARGS, _remote_debugging_RemoteUnwinder_get_stack_trace__doc__},

static PyObject *
_remote_debugging_RemoteUnwinder_get_stack_trace_impl(RemoteUnwinderObject *self);

static PyObject *
_remote_debugging_RemoteUnwinder_get_stack_trace(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _remote_debugging_RemoteUnwinder_get_stack_trace_impl((RemoteUnwinderObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_all_awaited_by__doc__,
"get_all_awaited_by($self, /)\n"
"--\n"
"\n"
"Get all tasks and their awaited_by from the remote process");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_ALL_AWAITED_BY_METHODDEF    \
    {"get_all_awaited_by", (PyCFunction)_remote_debugging_RemoteUnwinder_get_all_awaited_by, METH_NOARGS, _remote_debugging_RemoteUnwinder_get_all_awaited_by__doc__},

static PyObject *
_remote_debugging_RemoteUnwinder_get_all_awaited_by_impl(RemoteUnwinderObject *self);

static PyObject *
_remote_debugging_RemoteUnwinder_get_all_awaited_by(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _remote_debugging_RemoteUnwinder_get_all_awaited_by_impl((RemoteUnwinderObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_async_stack_trace__doc__,
"get_async_stack_trace($self, /)\n"
"--\n"
"\n"
"Get the asyncio stack from the remote process");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_ASYNC_STACK_TRACE_METHODDEF    \
    {"get_async_stack_trace", (PyCFunction)_remote_debugging_RemoteUnwinder_get_async_stack_trace, METH_NOARGS, _remote_debugging_RemoteUnwinder_get_async_stack_trace__doc__},

static PyObject *
_remote_debugging_RemoteUnwinder_get_async_stack_trace_impl(RemoteUnwinderObject *self);

static PyObject *
_remote_debugging_RemoteUnwinder_get_async_stack_trace(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _remote_debugging_RemoteUnwinder_get_async_stack_trace_impl((RemoteUnwinderObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=90c412b99a4f973f input=a9049054013a1b77]*/
