/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_asyncio_Future___init____doc__,
"Future(*, loop=None)\n"
"--\n"
"\n"
"This class is *almost* compatible with concurrent.futures.Future.\n"
"\n"
"    Differences:\n"
"\n"
"    - result() and exception() do not take a timeout argument and\n"
"      raise an exception when the future isn\'t done yet.\n"
"\n"
"    - Callbacks registered with add_done_callback() are always called\n"
"      via the event loop\'s call_soon_threadsafe().\n"
"\n"
"    - This class is not compatible with the wait() and as_completed()\n"
"      methods in the concurrent.futures package.");

static int
_asyncio_Future___init___impl(FutureObj *self, PyObject *loop);

static int
_asyncio_Future___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(loop), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"loop", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Future",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *loop = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    loop = fastargs[0];
skip_optional_kwonly:
    return_value = _asyncio_Future___init___impl((FutureObj *)self, loop);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_result__doc__,
"result($self, /)\n"
"--\n"
"\n"
"Return the result this future represents.\n"
"\n"
"If the future has been cancelled, raises CancelledError.  If the\n"
"future\'s result isn\'t yet available, raises InvalidStateError.  If\n"
"the future is done and has an exception set, this exception is raised.");

#define _ASYNCIO_FUTURE_RESULT_METHODDEF    \
    {"result", (PyCFunction)_asyncio_Future_result, METH_NOARGS, _asyncio_Future_result__doc__},

static PyObject *
_asyncio_Future_result_impl(FutureObj *self);

static PyObject *
_asyncio_Future_result(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_result_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_exception__doc__,
"exception($self, /)\n"
"--\n"
"\n"
"Return the exception that was set on this future.\n"
"\n"
"The exception (or None if no exception was set) is returned only if\n"
"the future is done.  If the future has been cancelled, raises\n"
"CancelledError.  If the future isn\'t done yet, raises\n"
"InvalidStateError.");

#define _ASYNCIO_FUTURE_EXCEPTION_METHODDEF    \
    {"exception", _PyCFunction_CAST(_asyncio_Future_exception), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _asyncio_Future_exception__doc__},

static PyObject *
_asyncio_Future_exception_impl(FutureObj *self, PyTypeObject *cls);

static PyObject *
_asyncio_Future_exception(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "exception() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_exception_impl((FutureObj *)self, cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_set_result__doc__,
"set_result($self, result, /)\n"
"--\n"
"\n"
"Mark the future done and set its result.\n"
"\n"
"If the future is already done when this method is called, raises\n"
"InvalidStateError.");

#define _ASYNCIO_FUTURE_SET_RESULT_METHODDEF    \
    {"set_result", _PyCFunction_CAST(_asyncio_Future_set_result), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _asyncio_Future_set_result__doc__},

static PyObject *
_asyncio_Future_set_result_impl(FutureObj *self, PyTypeObject *cls,
                                PyObject *result);

static PyObject *
_asyncio_Future_set_result(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_result",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *result;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    result = args[0];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_set_result_impl((FutureObj *)self, cls, result);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_set_exception__doc__,
"set_exception($self, exception, /)\n"
"--\n"
"\n"
"Mark the future done and set an exception.\n"
"\n"
"If the future is already done when this method is called, raises\n"
"InvalidStateError.");

#define _ASYNCIO_FUTURE_SET_EXCEPTION_METHODDEF    \
    {"set_exception", _PyCFunction_CAST(_asyncio_Future_set_exception), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _asyncio_Future_set_exception__doc__},

static PyObject *
_asyncio_Future_set_exception_impl(FutureObj *self, PyTypeObject *cls,
                                   PyObject *exception);

static PyObject *
_asyncio_Future_set_exception(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_exception",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *exception;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    exception = args[0];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_set_exception_impl((FutureObj *)self, cls, exception);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_add_done_callback__doc__,
"add_done_callback($self, fn, /, *, context=<unrepresentable>)\n"
"--\n"
"\n"
"Add a callback to be run when the future becomes done.\n"
"\n"
"The callback is called with a single argument - the future object. If\n"
"the future is already done when this is called, the callback is\n"
"scheduled with call_soon.");

#define _ASYNCIO_FUTURE_ADD_DONE_CALLBACK_METHODDEF    \
    {"add_done_callback", _PyCFunction_CAST(_asyncio_Future_add_done_callback), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _asyncio_Future_add_done_callback__doc__},

static PyObject *
_asyncio_Future_add_done_callback_impl(FutureObj *self, PyTypeObject *cls,
                                       PyObject *fn, PyObject *context);

static PyObject *
_asyncio_Future_add_done_callback(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(context), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "context", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "add_done_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *fn;
    PyObject *context = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    fn = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    context = args[1];
skip_optional_kwonly:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_add_done_callback_impl((FutureObj *)self, cls, fn, context);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_remove_done_callback__doc__,
"remove_done_callback($self, fn, /)\n"
"--\n"
"\n"
"Remove all instances of a callback from the \"call when done\" list.\n"
"\n"
"Returns the number of callbacks removed.");

#define _ASYNCIO_FUTURE_REMOVE_DONE_CALLBACK_METHODDEF    \
    {"remove_done_callback", _PyCFunction_CAST(_asyncio_Future_remove_done_callback), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _asyncio_Future_remove_done_callback__doc__},

static PyObject *
_asyncio_Future_remove_done_callback_impl(FutureObj *self, PyTypeObject *cls,
                                          PyObject *fn);

static PyObject *
_asyncio_Future_remove_done_callback(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "remove_done_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *fn;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    fn = args[0];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_remove_done_callback_impl((FutureObj *)self, cls, fn);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_cancel__doc__,
"cancel($self, /, msg=None)\n"
"--\n"
"\n"
"Cancel the future and schedule callbacks.\n"
"\n"
"If the future is already done or cancelled, return False.  Otherwise,\n"
"change the future\'s state to cancelled, schedule the callbacks and\n"
"return True.");

#define _ASYNCIO_FUTURE_CANCEL_METHODDEF    \
    {"cancel", _PyCFunction_CAST(_asyncio_Future_cancel), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _asyncio_Future_cancel__doc__},

static PyObject *
_asyncio_Future_cancel_impl(FutureObj *self, PyTypeObject *cls,
                            PyObject *msg);

static PyObject *
_asyncio_Future_cancel(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(msg), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"msg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cancel",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *msg = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    msg = args[0];
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_cancel_impl((FutureObj *)self, cls, msg);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_cancelled__doc__,
"cancelled($self, /)\n"
"--\n"
"\n"
"Return True if the future was cancelled.");

#define _ASYNCIO_FUTURE_CANCELLED_METHODDEF    \
    {"cancelled", (PyCFunction)_asyncio_Future_cancelled, METH_NOARGS, _asyncio_Future_cancelled__doc__},

static PyObject *
_asyncio_Future_cancelled_impl(FutureObj *self);

static PyObject *
_asyncio_Future_cancelled(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_cancelled_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_done__doc__,
"done($self, /)\n"
"--\n"
"\n"
"Return True if the future is done.\n"
"\n"
"Done means either that a result / exception are available, or that the\n"
"future was cancelled.");

#define _ASYNCIO_FUTURE_DONE_METHODDEF    \
    {"done", (PyCFunction)_asyncio_Future_done, METH_NOARGS, _asyncio_Future_done__doc__},

static PyObject *
_asyncio_Future_done_impl(FutureObj *self);

static PyObject *
_asyncio_Future_done(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_done_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Future_get_loop__doc__,
"get_loop($self, /)\n"
"--\n"
"\n"
"Return the event loop the Future is bound to.");

#define _ASYNCIO_FUTURE_GET_LOOP_METHODDEF    \
    {"get_loop", _PyCFunction_CAST(_asyncio_Future_get_loop), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _asyncio_Future_get_loop__doc__},

static PyObject *
_asyncio_Future_get_loop_impl(FutureObj *self, PyTypeObject *cls);

static PyObject *
_asyncio_Future_get_loop(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "get_loop() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future_get_loop_impl((FutureObj *)self, cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#if !defined(_asyncio_Future__asyncio_awaited_by_DOCSTR)
#  define _asyncio_Future__asyncio_awaited_by_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__ASYNCIO_AWAITED_BY_GETSETDEF)
#  undef _ASYNCIO_FUTURE__ASYNCIO_AWAITED_BY_GETSETDEF
#  define _ASYNCIO_FUTURE__ASYNCIO_AWAITED_BY_GETSETDEF {"_asyncio_awaited_by", (getter)_asyncio_Future__asyncio_awaited_by_get, (setter)_asyncio_Future__asyncio_awaited_by_set, _asyncio_Future__asyncio_awaited_by_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__ASYNCIO_AWAITED_BY_GETSETDEF {"_asyncio_awaited_by", (getter)_asyncio_Future__asyncio_awaited_by_get, NULL, _asyncio_Future__asyncio_awaited_by_DOCSTR},
#endif

static PyObject *
_asyncio_Future__asyncio_awaited_by_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__asyncio_awaited_by_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__asyncio_awaited_by_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__asyncio_future_blocking_DOCSTR)
#  define _asyncio_Future__asyncio_future_blocking_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__ASYNCIO_FUTURE_BLOCKING_GETSETDEF)
#  undef _ASYNCIO_FUTURE__ASYNCIO_FUTURE_BLOCKING_GETSETDEF
#  define _ASYNCIO_FUTURE__ASYNCIO_FUTURE_BLOCKING_GETSETDEF {"_asyncio_future_blocking", (getter)_asyncio_Future__asyncio_future_blocking_get, (setter)_asyncio_Future__asyncio_future_blocking_set, _asyncio_Future__asyncio_future_blocking_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__ASYNCIO_FUTURE_BLOCKING_GETSETDEF {"_asyncio_future_blocking", (getter)_asyncio_Future__asyncio_future_blocking_get, NULL, _asyncio_Future__asyncio_future_blocking_DOCSTR},
#endif

static PyObject *
_asyncio_Future__asyncio_future_blocking_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__asyncio_future_blocking_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__asyncio_future_blocking_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__asyncio_future_blocking_DOCSTR)
#  define _asyncio_Future__asyncio_future_blocking_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__ASYNCIO_FUTURE_BLOCKING_GETSETDEF)
#  undef _ASYNCIO_FUTURE__ASYNCIO_FUTURE_BLOCKING_GETSETDEF
#  define _ASYNCIO_FUTURE__ASYNCIO_FUTURE_BLOCKING_GETSETDEF {"_asyncio_future_blocking", (getter)_asyncio_Future__asyncio_future_blocking_get, (setter)_asyncio_Future__asyncio_future_blocking_set, _asyncio_Future__asyncio_future_blocking_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__ASYNCIO_FUTURE_BLOCKING_GETSETDEF {"_asyncio_future_blocking", NULL, (setter)_asyncio_Future__asyncio_future_blocking_set, NULL},
#endif

static int
_asyncio_Future__asyncio_future_blocking_set_impl(FutureObj *self,
                                                  PyObject *value);

static int
_asyncio_Future__asyncio_future_blocking_set(PyObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__asyncio_future_blocking_set_impl((FutureObj *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__log_traceback_DOCSTR)
#  define _asyncio_Future__log_traceback_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__LOG_TRACEBACK_GETSETDEF)
#  undef _ASYNCIO_FUTURE__LOG_TRACEBACK_GETSETDEF
#  define _ASYNCIO_FUTURE__LOG_TRACEBACK_GETSETDEF {"_log_traceback", (getter)_asyncio_Future__log_traceback_get, (setter)_asyncio_Future__log_traceback_set, _asyncio_Future__log_traceback_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__LOG_TRACEBACK_GETSETDEF {"_log_traceback", (getter)_asyncio_Future__log_traceback_get, NULL, _asyncio_Future__log_traceback_DOCSTR},
#endif

static PyObject *
_asyncio_Future__log_traceback_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__log_traceback_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__log_traceback_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__log_traceback_DOCSTR)
#  define _asyncio_Future__log_traceback_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__LOG_TRACEBACK_GETSETDEF)
#  undef _ASYNCIO_FUTURE__LOG_TRACEBACK_GETSETDEF
#  define _ASYNCIO_FUTURE__LOG_TRACEBACK_GETSETDEF {"_log_traceback", (getter)_asyncio_Future__log_traceback_get, (setter)_asyncio_Future__log_traceback_set, _asyncio_Future__log_traceback_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__LOG_TRACEBACK_GETSETDEF {"_log_traceback", NULL, (setter)_asyncio_Future__log_traceback_set, NULL},
#endif

static int
_asyncio_Future__log_traceback_set_impl(FutureObj *self, PyObject *value);

static int
_asyncio_Future__log_traceback_set(PyObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__log_traceback_set_impl((FutureObj *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__loop_DOCSTR)
#  define _asyncio_Future__loop_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__LOOP_GETSETDEF)
#  undef _ASYNCIO_FUTURE__LOOP_GETSETDEF
#  define _ASYNCIO_FUTURE__LOOP_GETSETDEF {"_loop", (getter)_asyncio_Future__loop_get, (setter)_asyncio_Future__loop_set, _asyncio_Future__loop_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__LOOP_GETSETDEF {"_loop", (getter)_asyncio_Future__loop_get, NULL, _asyncio_Future__loop_DOCSTR},
#endif

static PyObject *
_asyncio_Future__loop_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__loop_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__loop_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__callbacks_DOCSTR)
#  define _asyncio_Future__callbacks_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__CALLBACKS_GETSETDEF)
#  undef _ASYNCIO_FUTURE__CALLBACKS_GETSETDEF
#  define _ASYNCIO_FUTURE__CALLBACKS_GETSETDEF {"_callbacks", (getter)_asyncio_Future__callbacks_get, (setter)_asyncio_Future__callbacks_set, _asyncio_Future__callbacks_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__CALLBACKS_GETSETDEF {"_callbacks", (getter)_asyncio_Future__callbacks_get, NULL, _asyncio_Future__callbacks_DOCSTR},
#endif

static PyObject *
_asyncio_Future__callbacks_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__callbacks_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__callbacks_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__result_DOCSTR)
#  define _asyncio_Future__result_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__RESULT_GETSETDEF)
#  undef _ASYNCIO_FUTURE__RESULT_GETSETDEF
#  define _ASYNCIO_FUTURE__RESULT_GETSETDEF {"_result", (getter)_asyncio_Future__result_get, (setter)_asyncio_Future__result_set, _asyncio_Future__result_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__RESULT_GETSETDEF {"_result", (getter)_asyncio_Future__result_get, NULL, _asyncio_Future__result_DOCSTR},
#endif

static PyObject *
_asyncio_Future__result_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__result_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__result_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__exception_DOCSTR)
#  define _asyncio_Future__exception_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__EXCEPTION_GETSETDEF)
#  undef _ASYNCIO_FUTURE__EXCEPTION_GETSETDEF
#  define _ASYNCIO_FUTURE__EXCEPTION_GETSETDEF {"_exception", (getter)_asyncio_Future__exception_get, (setter)_asyncio_Future__exception_set, _asyncio_Future__exception_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__EXCEPTION_GETSETDEF {"_exception", (getter)_asyncio_Future__exception_get, NULL, _asyncio_Future__exception_DOCSTR},
#endif

static PyObject *
_asyncio_Future__exception_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__exception_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__exception_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__source_traceback_DOCSTR)
#  define _asyncio_Future__source_traceback_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__SOURCE_TRACEBACK_GETSETDEF)
#  undef _ASYNCIO_FUTURE__SOURCE_TRACEBACK_GETSETDEF
#  define _ASYNCIO_FUTURE__SOURCE_TRACEBACK_GETSETDEF {"_source_traceback", (getter)_asyncio_Future__source_traceback_get, (setter)_asyncio_Future__source_traceback_set, _asyncio_Future__source_traceback_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__SOURCE_TRACEBACK_GETSETDEF {"_source_traceback", (getter)_asyncio_Future__source_traceback_get, NULL, _asyncio_Future__source_traceback_DOCSTR},
#endif

static PyObject *
_asyncio_Future__source_traceback_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__source_traceback_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__source_traceback_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__cancel_message_DOCSTR)
#  define _asyncio_Future__cancel_message_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__CANCEL_MESSAGE_GETSETDEF)
#  undef _ASYNCIO_FUTURE__CANCEL_MESSAGE_GETSETDEF
#  define _ASYNCIO_FUTURE__CANCEL_MESSAGE_GETSETDEF {"_cancel_message", (getter)_asyncio_Future__cancel_message_get, (setter)_asyncio_Future__cancel_message_set, _asyncio_Future__cancel_message_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__CANCEL_MESSAGE_GETSETDEF {"_cancel_message", (getter)_asyncio_Future__cancel_message_get, NULL, _asyncio_Future__cancel_message_DOCSTR},
#endif

static PyObject *
_asyncio_Future__cancel_message_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__cancel_message_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__cancel_message_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__cancel_message_DOCSTR)
#  define _asyncio_Future__cancel_message_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__CANCEL_MESSAGE_GETSETDEF)
#  undef _ASYNCIO_FUTURE__CANCEL_MESSAGE_GETSETDEF
#  define _ASYNCIO_FUTURE__CANCEL_MESSAGE_GETSETDEF {"_cancel_message", (getter)_asyncio_Future__cancel_message_get, (setter)_asyncio_Future__cancel_message_set, _asyncio_Future__cancel_message_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__CANCEL_MESSAGE_GETSETDEF {"_cancel_message", NULL, (setter)_asyncio_Future__cancel_message_set, NULL},
#endif

static int
_asyncio_Future__cancel_message_set_impl(FutureObj *self, PyObject *value);

static int
_asyncio_Future__cancel_message_set(PyObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__cancel_message_set_impl((FutureObj *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Future__state_DOCSTR)
#  define _asyncio_Future__state_DOCSTR NULL
#endif
#if defined(_ASYNCIO_FUTURE__STATE_GETSETDEF)
#  undef _ASYNCIO_FUTURE__STATE_GETSETDEF
#  define _ASYNCIO_FUTURE__STATE_GETSETDEF {"_state", (getter)_asyncio_Future__state_get, (setter)_asyncio_Future__state_set, _asyncio_Future__state_DOCSTR},
#else
#  define _ASYNCIO_FUTURE__STATE_GETSETDEF {"_state", (getter)_asyncio_Future__state_get, NULL, _asyncio_Future__state_DOCSTR},
#endif

static PyObject *
_asyncio_Future__state_get_impl(FutureObj *self);

static PyObject *
_asyncio_Future__state_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__state_get_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Future__make_cancelled_error__doc__,
"_make_cancelled_error($self, /)\n"
"--\n"
"\n"
"Create the CancelledError to raise if the Future is cancelled.\n"
"\n"
"This should only be called once when handling a cancellation since\n"
"it erases the context exception value.");

#define _ASYNCIO_FUTURE__MAKE_CANCELLED_ERROR_METHODDEF    \
    {"_make_cancelled_error", (PyCFunction)_asyncio_Future__make_cancelled_error, METH_NOARGS, _asyncio_Future__make_cancelled_error__doc__},

static PyObject *
_asyncio_Future__make_cancelled_error_impl(FutureObj *self);

static PyObject *
_asyncio_Future__make_cancelled_error(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Future__make_cancelled_error_impl((FutureObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Task___init____doc__,
"Task(coro, *, loop=None, name=None, context=None, eager_start=False)\n"
"--\n"
"\n"
"A coroutine wrapped in a Future.");

static int
_asyncio_Task___init___impl(TaskObj *self, PyObject *coro, PyObject *loop,
                            PyObject *name, PyObject *context,
                            int eager_start);

static int
_asyncio_Task___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(coro), &_Py_ID(loop), &_Py_ID(name), &_Py_ID(context), &_Py_ID(eager_start), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"coro", "loop", "name", "context", "eager_start", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Task",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *coro;
    PyObject *loop = Py_None;
    PyObject *name = Py_None;
    PyObject *context = Py_None;
    int eager_start = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    coro = fastargs[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[1]) {
        loop = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[2]) {
        name = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[3]) {
        context = fastargs[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    eager_start = PyObject_IsTrue(fastargs[4]);
    if (eager_start < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _asyncio_Task___init___impl((TaskObj *)self, coro, loop, name, context, eager_start);

exit:
    return return_value;
}

#if !defined(_asyncio_Task__log_destroy_pending_DOCSTR)
#  define _asyncio_Task__log_destroy_pending_DOCSTR NULL
#endif
#if defined(_ASYNCIO_TASK__LOG_DESTROY_PENDING_GETSETDEF)
#  undef _ASYNCIO_TASK__LOG_DESTROY_PENDING_GETSETDEF
#  define _ASYNCIO_TASK__LOG_DESTROY_PENDING_GETSETDEF {"_log_destroy_pending", (getter)_asyncio_Task__log_destroy_pending_get, (setter)_asyncio_Task__log_destroy_pending_set, _asyncio_Task__log_destroy_pending_DOCSTR},
#else
#  define _ASYNCIO_TASK__LOG_DESTROY_PENDING_GETSETDEF {"_log_destroy_pending", (getter)_asyncio_Task__log_destroy_pending_get, NULL, _asyncio_Task__log_destroy_pending_DOCSTR},
#endif

static PyObject *
_asyncio_Task__log_destroy_pending_get_impl(TaskObj *self);

static PyObject *
_asyncio_Task__log_destroy_pending_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task__log_destroy_pending_get_impl((TaskObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Task__log_destroy_pending_DOCSTR)
#  define _asyncio_Task__log_destroy_pending_DOCSTR NULL
#endif
#if defined(_ASYNCIO_TASK__LOG_DESTROY_PENDING_GETSETDEF)
#  undef _ASYNCIO_TASK__LOG_DESTROY_PENDING_GETSETDEF
#  define _ASYNCIO_TASK__LOG_DESTROY_PENDING_GETSETDEF {"_log_destroy_pending", (getter)_asyncio_Task__log_destroy_pending_get, (setter)_asyncio_Task__log_destroy_pending_set, _asyncio_Task__log_destroy_pending_DOCSTR},
#else
#  define _ASYNCIO_TASK__LOG_DESTROY_PENDING_GETSETDEF {"_log_destroy_pending", NULL, (setter)_asyncio_Task__log_destroy_pending_set, NULL},
#endif

static int
_asyncio_Task__log_destroy_pending_set_impl(TaskObj *self, PyObject *value);

static int
_asyncio_Task__log_destroy_pending_set(PyObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task__log_destroy_pending_set_impl((TaskObj *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Task__must_cancel_DOCSTR)
#  define _asyncio_Task__must_cancel_DOCSTR NULL
#endif
#if defined(_ASYNCIO_TASK__MUST_CANCEL_GETSETDEF)
#  undef _ASYNCIO_TASK__MUST_CANCEL_GETSETDEF
#  define _ASYNCIO_TASK__MUST_CANCEL_GETSETDEF {"_must_cancel", (getter)_asyncio_Task__must_cancel_get, (setter)_asyncio_Task__must_cancel_set, _asyncio_Task__must_cancel_DOCSTR},
#else
#  define _ASYNCIO_TASK__MUST_CANCEL_GETSETDEF {"_must_cancel", (getter)_asyncio_Task__must_cancel_get, NULL, _asyncio_Task__must_cancel_DOCSTR},
#endif

static PyObject *
_asyncio_Task__must_cancel_get_impl(TaskObj *self);

static PyObject *
_asyncio_Task__must_cancel_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task__must_cancel_get_impl((TaskObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Task__coro_DOCSTR)
#  define _asyncio_Task__coro_DOCSTR NULL
#endif
#if defined(_ASYNCIO_TASK__CORO_GETSETDEF)
#  undef _ASYNCIO_TASK__CORO_GETSETDEF
#  define _ASYNCIO_TASK__CORO_GETSETDEF {"_coro", (getter)_asyncio_Task__coro_get, (setter)_asyncio_Task__coro_set, _asyncio_Task__coro_DOCSTR},
#else
#  define _ASYNCIO_TASK__CORO_GETSETDEF {"_coro", (getter)_asyncio_Task__coro_get, NULL, _asyncio_Task__coro_DOCSTR},
#endif

static PyObject *
_asyncio_Task__coro_get_impl(TaskObj *self);

static PyObject *
_asyncio_Task__coro_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task__coro_get_impl((TaskObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_asyncio_Task__fut_waiter_DOCSTR)
#  define _asyncio_Task__fut_waiter_DOCSTR NULL
#endif
#if defined(_ASYNCIO_TASK__FUT_WAITER_GETSETDEF)
#  undef _ASYNCIO_TASK__FUT_WAITER_GETSETDEF
#  define _ASYNCIO_TASK__FUT_WAITER_GETSETDEF {"_fut_waiter", (getter)_asyncio_Task__fut_waiter_get, (setter)_asyncio_Task__fut_waiter_set, _asyncio_Task__fut_waiter_DOCSTR},
#else
#  define _ASYNCIO_TASK__FUT_WAITER_GETSETDEF {"_fut_waiter", (getter)_asyncio_Task__fut_waiter_get, NULL, _asyncio_Task__fut_waiter_DOCSTR},
#endif

static PyObject *
_asyncio_Task__fut_waiter_get_impl(TaskObj *self);

static PyObject *
_asyncio_Task__fut_waiter_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task__fut_waiter_get_impl((TaskObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Task__make_cancelled_error__doc__,
"_make_cancelled_error($self, /)\n"
"--\n"
"\n"
"Create the CancelledError to raise if the Task is cancelled.\n"
"\n"
"This should only be called once when handling a cancellation since\n"
"it erases the context exception value.");

#define _ASYNCIO_TASK__MAKE_CANCELLED_ERROR_METHODDEF    \
    {"_make_cancelled_error", (PyCFunction)_asyncio_Task__make_cancelled_error, METH_NOARGS, _asyncio_Task__make_cancelled_error__doc__},

static PyObject *
_asyncio_Task__make_cancelled_error_impl(TaskObj *self);

static PyObject *
_asyncio_Task__make_cancelled_error(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task__make_cancelled_error_impl((TaskObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_cancel__doc__,
"cancel($self, /, msg=None)\n"
"--\n"
"\n"
"Request that this task cancel itself.\n"
"\n"
"This arranges for a CancelledError to be thrown into the\n"
"wrapped coroutine on the next cycle through the event loop.\n"
"The coroutine then has a chance to clean up or even deny\n"
"the request using try/except/finally.\n"
"\n"
"Unlike Future.cancel, this does not guarantee that the\n"
"task will be cancelled: the exception might be caught and\n"
"acted upon, delaying cancellation of the task or preventing\n"
"cancellation completely.  The task may also return a value or\n"
"raise a different exception.\n"
"\n"
"Immediately after this method is called, Task.cancelled() will\n"
"not return True (unless the task was already cancelled).  A\n"
"task will be marked as cancelled when the wrapped coroutine\n"
"terminates with a CancelledError exception (even if cancel()\n"
"was not called).\n"
"\n"
"This also increases the task\'s count of cancellation requests.");

#define _ASYNCIO_TASK_CANCEL_METHODDEF    \
    {"cancel", _PyCFunction_CAST(_asyncio_Task_cancel), METH_FASTCALL|METH_KEYWORDS, _asyncio_Task_cancel__doc__},

static PyObject *
_asyncio_Task_cancel_impl(TaskObj *self, PyObject *msg);

static PyObject *
_asyncio_Task_cancel(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(msg), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"msg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cancel",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *msg = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    msg = args[0];
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task_cancel_impl((TaskObj *)self, msg);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_cancelling__doc__,
"cancelling($self, /)\n"
"--\n"
"\n"
"Return the count of the task\'s cancellation requests.\n"
"\n"
"This count is incremented when .cancel() is called\n"
"and may be decremented using .uncancel().");

#define _ASYNCIO_TASK_CANCELLING_METHODDEF    \
    {"cancelling", (PyCFunction)_asyncio_Task_cancelling, METH_NOARGS, _asyncio_Task_cancelling__doc__},

static PyObject *
_asyncio_Task_cancelling_impl(TaskObj *self);

static PyObject *
_asyncio_Task_cancelling(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task_cancelling_impl((TaskObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_uncancel__doc__,
"uncancel($self, /)\n"
"--\n"
"\n"
"Decrement the task\'s count of cancellation requests.\n"
"\n"
"This should be used by tasks that catch CancelledError\n"
"and wish to continue indefinitely until they are cancelled again.\n"
"\n"
"Returns the remaining number of cancellation requests.");

#define _ASYNCIO_TASK_UNCANCEL_METHODDEF    \
    {"uncancel", (PyCFunction)_asyncio_Task_uncancel, METH_NOARGS, _asyncio_Task_uncancel__doc__},

static PyObject *
_asyncio_Task_uncancel_impl(TaskObj *self);

static PyObject *
_asyncio_Task_uncancel(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task_uncancel_impl((TaskObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_get_stack__doc__,
"get_stack($self, /, *, limit=None)\n"
"--\n"
"\n"
"Return the list of stack frames for this task\'s coroutine.\n"
"\n"
"If the coroutine is not done, this returns the stack where it is\n"
"suspended.  If the coroutine has completed successfully or was\n"
"cancelled, this returns an empty list.  If the coroutine was\n"
"terminated by an exception, this returns the list of traceback\n"
"frames.\n"
"\n"
"The frames are always ordered from oldest to newest.\n"
"\n"
"The optional limit gives the maximum number of frames to\n"
"return; by default all available frames are returned.  Its\n"
"meaning differs depending on whether a stack or a traceback is\n"
"returned: the newest frames of a stack are returned, but the\n"
"oldest frames of a traceback are returned.  (This matches the\n"
"behavior of the traceback module.)\n"
"\n"
"For reasons beyond our control, only one stack frame is\n"
"returned for a suspended coroutine.");

#define _ASYNCIO_TASK_GET_STACK_METHODDEF    \
    {"get_stack", _PyCFunction_CAST(_asyncio_Task_get_stack), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _asyncio_Task_get_stack__doc__},

static PyObject *
_asyncio_Task_get_stack_impl(TaskObj *self, PyTypeObject *cls,
                             PyObject *limit);

static PyObject *
_asyncio_Task_get_stack(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(limit), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"limit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_stack",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *limit = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    limit = args[0];
skip_optional_kwonly:
    return_value = _asyncio_Task_get_stack_impl((TaskObj *)self, cls, limit);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_print_stack__doc__,
"print_stack($self, /, *, limit=None, file=None)\n"
"--\n"
"\n"
"Print the stack or traceback for this task\'s coroutine.\n"
"\n"
"This produces output similar to that of the traceback module,\n"
"for the frames retrieved by get_stack().  The limit argument\n"
"is passed to get_stack().  The file argument is an I/O stream\n"
"to which the output is written; by default output is written\n"
"to sys.stderr.");

#define _ASYNCIO_TASK_PRINT_STACK_METHODDEF    \
    {"print_stack", _PyCFunction_CAST(_asyncio_Task_print_stack), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _asyncio_Task_print_stack__doc__},

static PyObject *
_asyncio_Task_print_stack_impl(TaskObj *self, PyTypeObject *cls,
                               PyObject *limit, PyObject *file);

static PyObject *
_asyncio_Task_print_stack(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(limit), &_Py_ID(file), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"limit", "file", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "print_stack",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *limit = Py_None;
    PyObject *file = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[0]) {
        limit = args[0];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    file = args[1];
skip_optional_kwonly:
    return_value = _asyncio_Task_print_stack_impl((TaskObj *)self, cls, limit, file);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_set_result__doc__,
"set_result($self, result, /)\n"
"--\n"
"\n");

#define _ASYNCIO_TASK_SET_RESULT_METHODDEF    \
    {"set_result", (PyCFunction)_asyncio_Task_set_result, METH_O, _asyncio_Task_set_result__doc__},

static PyObject *
_asyncio_Task_set_result_impl(TaskObj *self, PyObject *result);

static PyObject *
_asyncio_Task_set_result(PyObject *self, PyObject *result)
{
    PyObject *return_value = NULL;

    return_value = _asyncio_Task_set_result_impl((TaskObj *)self, result);

    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_set_exception__doc__,
"set_exception($self, exception, /)\n"
"--\n"
"\n");

#define _ASYNCIO_TASK_SET_EXCEPTION_METHODDEF    \
    {"set_exception", (PyCFunction)_asyncio_Task_set_exception, METH_O, _asyncio_Task_set_exception__doc__},

static PyObject *
_asyncio_Task_set_exception_impl(TaskObj *self, PyObject *exception);

static PyObject *
_asyncio_Task_set_exception(PyObject *self, PyObject *exception)
{
    PyObject *return_value = NULL;

    return_value = _asyncio_Task_set_exception_impl((TaskObj *)self, exception);

    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_get_coro__doc__,
"get_coro($self, /)\n"
"--\n"
"\n");

#define _ASYNCIO_TASK_GET_CORO_METHODDEF    \
    {"get_coro", (PyCFunction)_asyncio_Task_get_coro, METH_NOARGS, _asyncio_Task_get_coro__doc__},

static PyObject *
_asyncio_Task_get_coro_impl(TaskObj *self);

static PyObject *
_asyncio_Task_get_coro(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task_get_coro_impl((TaskObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_get_context__doc__,
"get_context($self, /)\n"
"--\n"
"\n");

#define _ASYNCIO_TASK_GET_CONTEXT_METHODDEF    \
    {"get_context", (PyCFunction)_asyncio_Task_get_context, METH_NOARGS, _asyncio_Task_get_context__doc__},

static PyObject *
_asyncio_Task_get_context_impl(TaskObj *self);

static PyObject *
_asyncio_Task_get_context(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_Task_get_context_impl((TaskObj *)self);
}

PyDoc_STRVAR(_asyncio_Task_get_name__doc__,
"get_name($self, /)\n"
"--\n"
"\n");

#define _ASYNCIO_TASK_GET_NAME_METHODDEF    \
    {"get_name", (PyCFunction)_asyncio_Task_get_name, METH_NOARGS, _asyncio_Task_get_name__doc__},

static PyObject *
_asyncio_Task_get_name_impl(TaskObj *self);

static PyObject *
_asyncio_Task_get_name(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task_get_name_impl((TaskObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio_Task_set_name__doc__,
"set_name($self, value, /)\n"
"--\n"
"\n");

#define _ASYNCIO_TASK_SET_NAME_METHODDEF    \
    {"set_name", (PyCFunction)_asyncio_Task_set_name, METH_O, _asyncio_Task_set_name__doc__},

static PyObject *
_asyncio_Task_set_name_impl(TaskObj *self, PyObject *value);

static PyObject *
_asyncio_Task_set_name(PyObject *self, PyObject *value)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _asyncio_Task_set_name_impl((TaskObj *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_asyncio__get_running_loop__doc__,
"_get_running_loop($module, /)\n"
"--\n"
"\n"
"Return the running event loop or None.\n"
"\n"
"This is a low-level function intended to be used by event loops.\n"
"This function is thread-specific.");

#define _ASYNCIO__GET_RUNNING_LOOP_METHODDEF    \
    {"_get_running_loop", (PyCFunction)_asyncio__get_running_loop, METH_NOARGS, _asyncio__get_running_loop__doc__},

static PyObject *
_asyncio__get_running_loop_impl(PyObject *module);

static PyObject *
_asyncio__get_running_loop(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _asyncio__get_running_loop_impl(module);
}

PyDoc_STRVAR(_asyncio__set_running_loop__doc__,
"_set_running_loop($module, loop, /)\n"
"--\n"
"\n"
"Set the running event loop.\n"
"\n"
"This is a low-level function intended to be used by event loops.\n"
"This function is thread-specific.");

#define _ASYNCIO__SET_RUNNING_LOOP_METHODDEF    \
    {"_set_running_loop", (PyCFunction)_asyncio__set_running_loop, METH_O, _asyncio__set_running_loop__doc__},

PyDoc_STRVAR(_asyncio_get_event_loop__doc__,
"get_event_loop($module, /)\n"
"--\n"
"\n"
"Return an asyncio event loop.\n"
"\n"
"When called from a coroutine or a callback (e.g. scheduled with\n"
"call_soon or similar API), this function will always return the\n"
"running event loop.\n"
"\n"
"If there is no running event loop set, the function will return\n"
"the result of `get_event_loop_policy().get_event_loop()` call.");

#define _ASYNCIO_GET_EVENT_LOOP_METHODDEF    \
    {"get_event_loop", (PyCFunction)_asyncio_get_event_loop, METH_NOARGS, _asyncio_get_event_loop__doc__},

static PyObject *
_asyncio_get_event_loop_impl(PyObject *module);

static PyObject *
_asyncio_get_event_loop(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_get_event_loop_impl(module);
}

PyDoc_STRVAR(_asyncio_get_running_loop__doc__,
"get_running_loop($module, /)\n"
"--\n"
"\n"
"Return the running event loop.  Raise a RuntimeError if there is none.\n"
"\n"
"This function is thread-specific.");

#define _ASYNCIO_GET_RUNNING_LOOP_METHODDEF    \
    {"get_running_loop", (PyCFunction)_asyncio_get_running_loop, METH_NOARGS, _asyncio_get_running_loop__doc__},

static PyObject *
_asyncio_get_running_loop_impl(PyObject *module);

static PyObject *
_asyncio_get_running_loop(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _asyncio_get_running_loop_impl(module);
}

PyDoc_STRVAR(_asyncio__register_task__doc__,
"_register_task($module, /, task)\n"
"--\n"
"\n"
"Register a new task in asyncio as executed by loop.\n"
"\n"
"Returns None.");

#define _ASYNCIO__REGISTER_TASK_METHODDEF    \
    {"_register_task", _PyCFunction_CAST(_asyncio__register_task), METH_FASTCALL|METH_KEYWORDS, _asyncio__register_task__doc__},

static PyObject *
_asyncio__register_task_impl(PyObject *module, PyObject *task);

static PyObject *
_asyncio__register_task(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(task), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"task", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_register_task",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *task;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    task = args[0];
    return_value = _asyncio__register_task_impl(module, task);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio__register_eager_task__doc__,
"_register_eager_task($module, /, task)\n"
"--\n"
"\n"
"Register a new task in asyncio as executed by loop.\n"
"\n"
"Returns None.");

#define _ASYNCIO__REGISTER_EAGER_TASK_METHODDEF    \
    {"_register_eager_task", _PyCFunction_CAST(_asyncio__register_eager_task), METH_FASTCALL|METH_KEYWORDS, _asyncio__register_eager_task__doc__},

static PyObject *
_asyncio__register_eager_task_impl(PyObject *module, PyObject *task);

static PyObject *
_asyncio__register_eager_task(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(task), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"task", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_register_eager_task",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *task;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    task = args[0];
    return_value = _asyncio__register_eager_task_impl(module, task);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio__unregister_task__doc__,
"_unregister_task($module, /, task)\n"
"--\n"
"\n"
"Unregister a task.\n"
"\n"
"Returns None.");

#define _ASYNCIO__UNREGISTER_TASK_METHODDEF    \
    {"_unregister_task", _PyCFunction_CAST(_asyncio__unregister_task), METH_FASTCALL|METH_KEYWORDS, _asyncio__unregister_task__doc__},

static PyObject *
_asyncio__unregister_task_impl(PyObject *module, PyObject *task);

static PyObject *
_asyncio__unregister_task(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(task), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"task", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_unregister_task",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *task;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    task = args[0];
    return_value = _asyncio__unregister_task_impl(module, task);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio__unregister_eager_task__doc__,
"_unregister_eager_task($module, /, task)\n"
"--\n"
"\n"
"Unregister a task.\n"
"\n"
"Returns None.");

#define _ASYNCIO__UNREGISTER_EAGER_TASK_METHODDEF    \
    {"_unregister_eager_task", _PyCFunction_CAST(_asyncio__unregister_eager_task), METH_FASTCALL|METH_KEYWORDS, _asyncio__unregister_eager_task__doc__},

static PyObject *
_asyncio__unregister_eager_task_impl(PyObject *module, PyObject *task);

static PyObject *
_asyncio__unregister_eager_task(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(task), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"task", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_unregister_eager_task",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *task;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    task = args[0];
    return_value = _asyncio__unregister_eager_task_impl(module, task);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio__enter_task__doc__,
"_enter_task($module, /, loop, task)\n"
"--\n"
"\n"
"Enter into task execution or resume suspended task.\n"
"\n"
"Task belongs to loop.\n"
"\n"
"Returns None.");

#define _ASYNCIO__ENTER_TASK_METHODDEF    \
    {"_enter_task", _PyCFunction_CAST(_asyncio__enter_task), METH_FASTCALL|METH_KEYWORDS, _asyncio__enter_task__doc__},

static PyObject *
_asyncio__enter_task_impl(PyObject *module, PyObject *loop, PyObject *task);

static PyObject *
_asyncio__enter_task(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(loop), &_Py_ID(task), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"loop", "task", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_enter_task",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *loop;
    PyObject *task;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    loop = args[0];
    task = args[1];
    return_value = _asyncio__enter_task_impl(module, loop, task);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio__leave_task__doc__,
"_leave_task($module, /, loop, task)\n"
"--\n"
"\n"
"Leave task execution or suspend a task.\n"
"\n"
"Task belongs to loop.\n"
"\n"
"Returns None.");

#define _ASYNCIO__LEAVE_TASK_METHODDEF    \
    {"_leave_task", _PyCFunction_CAST(_asyncio__leave_task), METH_FASTCALL|METH_KEYWORDS, _asyncio__leave_task__doc__},

static PyObject *
_asyncio__leave_task_impl(PyObject *module, PyObject *loop, PyObject *task);

static PyObject *
_asyncio__leave_task(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(loop), &_Py_ID(task), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"loop", "task", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_leave_task",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *loop;
    PyObject *task;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    loop = args[0];
    task = args[1];
    return_value = _asyncio__leave_task_impl(module, loop, task);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio__swap_current_task__doc__,
"_swap_current_task($module, /, loop, task)\n"
"--\n"
"\n"
"Temporarily swap in the supplied task and return the original one (or None).\n"
"\n"
"This is intended for use during eager coroutine execution.");

#define _ASYNCIO__SWAP_CURRENT_TASK_METHODDEF    \
    {"_swap_current_task", _PyCFunction_CAST(_asyncio__swap_current_task), METH_FASTCALL|METH_KEYWORDS, _asyncio__swap_current_task__doc__},

static PyObject *
_asyncio__swap_current_task_impl(PyObject *module, PyObject *loop,
                                 PyObject *task);

static PyObject *
_asyncio__swap_current_task(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(loop), &_Py_ID(task), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"loop", "task", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_swap_current_task",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *loop;
    PyObject *task;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    loop = args[0];
    task = args[1];
    return_value = _asyncio__swap_current_task_impl(module, loop, task);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_current_task__doc__,
"current_task($module, /, loop=None)\n"
"--\n"
"\n"
"Return a currently executed task.");

#define _ASYNCIO_CURRENT_TASK_METHODDEF    \
    {"current_task", _PyCFunction_CAST(_asyncio_current_task), METH_FASTCALL|METH_KEYWORDS, _asyncio_current_task__doc__},

static PyObject *
_asyncio_current_task_impl(PyObject *module, PyObject *loop);

static PyObject *
_asyncio_current_task(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(loop), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"loop", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "current_task",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *loop = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    loop = args[0];
skip_optional_pos:
    return_value = _asyncio_current_task_impl(module, loop);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_all_tasks__doc__,
"all_tasks($module, /, loop=None)\n"
"--\n"
"\n"
"Return a set of all tasks for the loop.");

#define _ASYNCIO_ALL_TASKS_METHODDEF    \
    {"all_tasks", _PyCFunction_CAST(_asyncio_all_tasks), METH_FASTCALL|METH_KEYWORDS, _asyncio_all_tasks__doc__},

static PyObject *
_asyncio_all_tasks_impl(PyObject *module, PyObject *loop);

static PyObject *
_asyncio_all_tasks(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(loop), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"loop", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "all_tasks",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *loop = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    loop = args[0];
skip_optional_pos:
    return_value = _asyncio_all_tasks_impl(module, loop);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_future_add_to_awaited_by__doc__,
"future_add_to_awaited_by($module, fut, waiter, /)\n"
"--\n"
"\n"
"Record that `fut` is awaited on by `waiter`.");

#define _ASYNCIO_FUTURE_ADD_TO_AWAITED_BY_METHODDEF    \
    {"future_add_to_awaited_by", _PyCFunction_CAST(_asyncio_future_add_to_awaited_by), METH_FASTCALL, _asyncio_future_add_to_awaited_by__doc__},

static PyObject *
_asyncio_future_add_to_awaited_by_impl(PyObject *module, PyObject *fut,
                                       PyObject *waiter);

static PyObject *
_asyncio_future_add_to_awaited_by(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *fut;
    PyObject *waiter;

    if (!_PyArg_CheckPositional("future_add_to_awaited_by", nargs, 2, 2)) {
        goto exit;
    }
    fut = args[0];
    waiter = args[1];
    return_value = _asyncio_future_add_to_awaited_by_impl(module, fut, waiter);

exit:
    return return_value;
}

PyDoc_STRVAR(_asyncio_future_discard_from_awaited_by__doc__,
"future_discard_from_awaited_by($module, fut, waiter, /)\n"
"--\n"
"\n");

#define _ASYNCIO_FUTURE_DISCARD_FROM_AWAITED_BY_METHODDEF    \
    {"future_discard_from_awaited_by", _PyCFunction_CAST(_asyncio_future_discard_from_awaited_by), METH_FASTCALL, _asyncio_future_discard_from_awaited_by__doc__},

static PyObject *
_asyncio_future_discard_from_awaited_by_impl(PyObject *module, PyObject *fut,
                                             PyObject *waiter);

static PyObject *
_asyncio_future_discard_from_awaited_by(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *fut;
    PyObject *waiter;

    if (!_PyArg_CheckPositional("future_discard_from_awaited_by", nargs, 2, 2)) {
        goto exit;
    }
    fut = args[0];
    waiter = args[1];
    return_value = _asyncio_future_discard_from_awaited_by_impl(module, fut, waiter);

exit:
    return return_value;
}
/*[clinic end generated code: output=b69948ed810591d9 input=a9049054013a1b77]*/
