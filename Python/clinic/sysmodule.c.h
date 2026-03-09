/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(sys_addaudithook__doc__,
"addaudithook($module, /, hook)\n"
"--\n"
"\n"
"Adds a new audit hook callback.");

#define SYS_ADDAUDITHOOK_METHODDEF    \
    {"addaudithook", _PyCFunction_CAST(sys_addaudithook), METH_FASTCALL|METH_KEYWORDS, sys_addaudithook__doc__},

static PyObject *
sys_addaudithook_impl(PyObject *module, PyObject *hook);

static PyObject *
sys_addaudithook(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(hook), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"hook", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "addaudithook",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *hook;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    hook = args[0];
    return_value = sys_addaudithook_impl(module, hook);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_audit__doc__,
"audit($module, event, /, *args)\n"
"--\n"
"\n"
"Passes the event to any audit hooks that are attached.");

#define SYS_AUDIT_METHODDEF    \
    {"audit", _PyCFunction_CAST(sys_audit), METH_FASTCALL, sys_audit__doc__},

static PyObject *
sys_audit_impl(PyObject *module, const char *event, PyObject *args);

static PyObject *
sys_audit(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *event;
    PyObject *__clinic_args = NULL;

    if (!_PyArg_CheckPositional("audit", nargs, 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("audit", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t event_length;
    event = PyUnicode_AsUTF8AndSize(args[0], &event_length);
    if (event == NULL) {
        goto exit;
    }
    if (strlen(event) != (size_t)event_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    __clinic_args = PyTuple_FromArray(args + 1, nargs - 1);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = sys_audit_impl(module, event, __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(sys_displayhook__doc__,
"displayhook($module, object, /)\n"
"--\n"
"\n"
"Print an object to sys.stdout and also save it in builtins._");

#define SYS_DISPLAYHOOK_METHODDEF    \
    {"displayhook", (PyCFunction)sys_displayhook, METH_O, sys_displayhook__doc__},

PyDoc_STRVAR(sys_excepthook__doc__,
"excepthook($module, exctype, value, traceback, /)\n"
"--\n"
"\n"
"Handle an exception by displaying it with a traceback on sys.stderr.");

#define SYS_EXCEPTHOOK_METHODDEF    \
    {"excepthook", _PyCFunction_CAST(sys_excepthook), METH_FASTCALL, sys_excepthook__doc__},

static PyObject *
sys_excepthook_impl(PyObject *module, PyObject *exctype, PyObject *value,
                    PyObject *traceback);

static PyObject *
sys_excepthook(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exctype;
    PyObject *value;
    PyObject *traceback;

    if (!_PyArg_CheckPositional("excepthook", nargs, 3, 3)) {
        goto exit;
    }
    exctype = args[0];
    value = args[1];
    traceback = args[2];
    return_value = sys_excepthook_impl(module, exctype, value, traceback);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_exception__doc__,
"exception($module, /)\n"
"--\n"
"\n"
"Return the current exception.\n"
"\n"
"Return the most recent exception caught by an except clause\n"
"in the current stack frame or in an older stack frame, or None\n"
"if no such exception exists.");

#define SYS_EXCEPTION_METHODDEF    \
    {"exception", (PyCFunction)sys_exception, METH_NOARGS, sys_exception__doc__},

static PyObject *
sys_exception_impl(PyObject *module);

static PyObject *
sys_exception(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_exception_impl(module);
}

PyDoc_STRVAR(sys_exc_info__doc__,
"exc_info($module, /)\n"
"--\n"
"\n"
"Return current exception information: (type, value, traceback).\n"
"\n"
"Return information about the most recent exception caught by an except\n"
"clause in the current stack frame or in an older stack frame.");

#define SYS_EXC_INFO_METHODDEF    \
    {"exc_info", (PyCFunction)sys_exc_info, METH_NOARGS, sys_exc_info__doc__},

static PyObject *
sys_exc_info_impl(PyObject *module);

static PyObject *
sys_exc_info(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_exc_info_impl(module);
}

PyDoc_STRVAR(sys_unraisablehook__doc__,
"unraisablehook($module, unraisable, /)\n"
"--\n"
"\n"
"Handle an unraisable exception.\n"
"\n"
"The unraisable argument has the following attributes:\n"
"\n"
"* exc_type: Exception type.\n"
"* exc_value: Exception value, can be None.\n"
"* exc_traceback: Exception traceback, can be None.\n"
"* err_msg: Error message, can be None.\n"
"* object: Object causing the exception, can be None.");

#define SYS_UNRAISABLEHOOK_METHODDEF    \
    {"unraisablehook", (PyCFunction)sys_unraisablehook, METH_O, sys_unraisablehook__doc__},

PyDoc_STRVAR(sys_exit__doc__,
"exit($module, status=None, /)\n"
"--\n"
"\n"
"Exit the interpreter by raising SystemExit(status).\n"
"\n"
"If the status is omitted or None, it defaults to zero (i.e., success).\n"
"If the status is an integer, it will be used as the system exit status.\n"
"If it is another kind of object, it will be printed and the system\n"
"exit status will be one (i.e., failure).");

#define SYS_EXIT_METHODDEF    \
    {"exit", _PyCFunction_CAST(sys_exit), METH_FASTCALL, sys_exit__doc__},

static PyObject *
sys_exit_impl(PyObject *module, PyObject *status);

static PyObject *
sys_exit(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *status = Py_None;

    if (!_PyArg_CheckPositional("exit", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    status = args[0];
skip_optional:
    return_value = sys_exit_impl(module, status);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getdefaultencoding__doc__,
"getdefaultencoding($module, /)\n"
"--\n"
"\n"
"Return the current default encoding used by the Unicode implementation.");

#define SYS_GETDEFAULTENCODING_METHODDEF    \
    {"getdefaultencoding", (PyCFunction)sys_getdefaultencoding, METH_NOARGS, sys_getdefaultencoding__doc__},

static PyObject *
sys_getdefaultencoding_impl(PyObject *module);

static PyObject *
sys_getdefaultencoding(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getdefaultencoding_impl(module);
}

PyDoc_STRVAR(sys_getfilesystemencoding__doc__,
"getfilesystemencoding($module, /)\n"
"--\n"
"\n"
"Return the encoding used to convert Unicode filenames to OS filenames.");

#define SYS_GETFILESYSTEMENCODING_METHODDEF    \
    {"getfilesystemencoding", (PyCFunction)sys_getfilesystemencoding, METH_NOARGS, sys_getfilesystemencoding__doc__},

static PyObject *
sys_getfilesystemencoding_impl(PyObject *module);

static PyObject *
sys_getfilesystemencoding(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getfilesystemencoding_impl(module);
}

PyDoc_STRVAR(sys_getfilesystemencodeerrors__doc__,
"getfilesystemencodeerrors($module, /)\n"
"--\n"
"\n"
"Return the error mode used Unicode to OS filename conversion.");

#define SYS_GETFILESYSTEMENCODEERRORS_METHODDEF    \
    {"getfilesystemencodeerrors", (PyCFunction)sys_getfilesystemencodeerrors, METH_NOARGS, sys_getfilesystemencodeerrors__doc__},

static PyObject *
sys_getfilesystemencodeerrors_impl(PyObject *module);

static PyObject *
sys_getfilesystemencodeerrors(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getfilesystemencodeerrors_impl(module);
}

PyDoc_STRVAR(sys_intern__doc__,
"intern($module, string, /)\n"
"--\n"
"\n"
"``Intern\'\' the given string.\n"
"\n"
"This enters the string in the (global) table of interned strings whose\n"
"purpose is to speed up dictionary lookups. Return the string itself or\n"
"the previously interned string object with the same value.");

#define SYS_INTERN_METHODDEF    \
    {"intern", (PyCFunction)sys_intern, METH_O, sys_intern__doc__},

static PyObject *
sys_intern_impl(PyObject *module, PyObject *s);

static PyObject *
sys_intern(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *s;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("intern", "argument", "str", arg);
        goto exit;
    }
    s = arg;
    return_value = sys_intern_impl(module, s);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__is_interned__doc__,
"_is_interned($module, string, /)\n"
"--\n"
"\n"
"Return True if the given string is \"interned\".");

#define SYS__IS_INTERNED_METHODDEF    \
    {"_is_interned", (PyCFunction)sys__is_interned, METH_O, sys__is_interned__doc__},

static int
sys__is_interned_impl(PyObject *module, PyObject *string);

static PyObject *
sys__is_interned(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *string;
    int _return_value;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("_is_interned", "argument", "str", arg);
        goto exit;
    }
    string = arg;
    _return_value = sys__is_interned_impl(module, string);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__is_immortal__doc__,
"_is_immortal($module, op, /)\n"
"--\n"
"\n"
"Return True if the given object is \"immortal\" per PEP 683.\n"
"\n"
"This function should be used for specialized purposes only.");

#define SYS__IS_IMMORTAL_METHODDEF    \
    {"_is_immortal", (PyCFunction)sys__is_immortal, METH_O, sys__is_immortal__doc__},

static int
sys__is_immortal_impl(PyObject *module, PyObject *op);

static PyObject *
sys__is_immortal(PyObject *module, PyObject *op)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = sys__is_immortal_impl(module, op);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_settrace__doc__,
"settrace($module, function, /)\n"
"--\n"
"\n"
"Set the global debug tracing function.\n"
"\n"
"It will be called on each function call.  See the debugger chapter\n"
"in the library manual.");

#define SYS_SETTRACE_METHODDEF    \
    {"settrace", (PyCFunction)sys_settrace, METH_O, sys_settrace__doc__},

PyDoc_STRVAR(sys__settraceallthreads__doc__,
"_settraceallthreads($module, function, /)\n"
"--\n"
"\n"
"Set the global debug tracing function in all running threads belonging to the current interpreter.\n"
"\n"
"It will be called on each function call. See the debugger chapter\n"
"in the library manual.");

#define SYS__SETTRACEALLTHREADS_METHODDEF    \
    {"_settraceallthreads", (PyCFunction)sys__settraceallthreads, METH_O, sys__settraceallthreads__doc__},

PyDoc_STRVAR(sys_gettrace__doc__,
"gettrace($module, /)\n"
"--\n"
"\n"
"Return the global debug tracing function set with sys.settrace.\n"
"\n"
"See the debugger chapter in the library manual.");

#define SYS_GETTRACE_METHODDEF    \
    {"gettrace", (PyCFunction)sys_gettrace, METH_NOARGS, sys_gettrace__doc__},

static PyObject *
sys_gettrace_impl(PyObject *module);

static PyObject *
sys_gettrace(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_gettrace_impl(module);
}

PyDoc_STRVAR(sys_setprofile__doc__,
"setprofile($module, function, /)\n"
"--\n"
"\n"
"Set the profiling function.\n"
"\n"
"It will be called on each function call and return.  See the profiler\n"
"chapter in the library manual.");

#define SYS_SETPROFILE_METHODDEF    \
    {"setprofile", (PyCFunction)sys_setprofile, METH_O, sys_setprofile__doc__},

PyDoc_STRVAR(sys__setprofileallthreads__doc__,
"_setprofileallthreads($module, function, /)\n"
"--\n"
"\n"
"Set the profiling function in all running threads belonging to the current interpreter.\n"
"\n"
"It will be called on each function call and return.  See the profiler\n"
"chapter in the library manual.");

#define SYS__SETPROFILEALLTHREADS_METHODDEF    \
    {"_setprofileallthreads", (PyCFunction)sys__setprofileallthreads, METH_O, sys__setprofileallthreads__doc__},

PyDoc_STRVAR(sys_getprofile__doc__,
"getprofile($module, /)\n"
"--\n"
"\n"
"Return the profiling function set with sys.setprofile.\n"
"\n"
"See the profiler chapter in the library manual.");

#define SYS_GETPROFILE_METHODDEF    \
    {"getprofile", (PyCFunction)sys_getprofile, METH_NOARGS, sys_getprofile__doc__},

static PyObject *
sys_getprofile_impl(PyObject *module);

static PyObject *
sys_getprofile(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getprofile_impl(module);
}

PyDoc_STRVAR(sys_setswitchinterval__doc__,
"setswitchinterval($module, interval, /)\n"
"--\n"
"\n"
"Set the ideal thread switching delay inside the Python interpreter.\n"
"\n"
"The actual frequency of switching threads can be lower if the\n"
"interpreter executes long sequences of uninterruptible code\n"
"(this is implementation-specific and workload-dependent).\n"
"\n"
"The parameter must represent the desired switching delay in seconds\n"
"A typical value is 0.005 (5 milliseconds).");

#define SYS_SETSWITCHINTERVAL_METHODDEF    \
    {"setswitchinterval", (PyCFunction)sys_setswitchinterval, METH_O, sys_setswitchinterval__doc__},

static PyObject *
sys_setswitchinterval_impl(PyObject *module, double interval);

static PyObject *
sys_setswitchinterval(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    double interval;

    if (PyFloat_CheckExact(arg)) {
        interval = PyFloat_AS_DOUBLE(arg);
    }
    else
    {
        interval = PyFloat_AsDouble(arg);
        if (interval == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = sys_setswitchinterval_impl(module, interval);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getswitchinterval__doc__,
"getswitchinterval($module, /)\n"
"--\n"
"\n"
"Return the current thread switch interval; see sys.setswitchinterval().");

#define SYS_GETSWITCHINTERVAL_METHODDEF    \
    {"getswitchinterval", (PyCFunction)sys_getswitchinterval, METH_NOARGS, sys_getswitchinterval__doc__},

static double
sys_getswitchinterval_impl(PyObject *module);

static PyObject *
sys_getswitchinterval(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    double _return_value;

    _return_value = sys_getswitchinterval_impl(module);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_setrecursionlimit__doc__,
"setrecursionlimit($module, limit, /)\n"
"--\n"
"\n"
"Set the maximum depth of the Python interpreter stack to n.\n"
"\n"
"This limit prevents infinite recursion from causing an overflow of the C\n"
"stack and crashing Python.  The highest possible limit is platform-\n"
"dependent.");

#define SYS_SETRECURSIONLIMIT_METHODDEF    \
    {"setrecursionlimit", (PyCFunction)sys_setrecursionlimit, METH_O, sys_setrecursionlimit__doc__},

static PyObject *
sys_setrecursionlimit_impl(PyObject *module, int new_limit);

static PyObject *
sys_setrecursionlimit(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int new_limit;

    new_limit = PyLong_AsInt(arg);
    if (new_limit == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_setrecursionlimit_impl(module, new_limit);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_set_coroutine_origin_tracking_depth__doc__,
"set_coroutine_origin_tracking_depth($module, /, depth)\n"
"--\n"
"\n"
"Enable or disable origin tracking for coroutine objects in this thread.\n"
"\n"
"Coroutine objects will track \'depth\' frames of traceback information\n"
"about where they came from, available in their cr_origin attribute.\n"
"\n"
"Set a depth of 0 to disable.");

#define SYS_SET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF    \
    {"set_coroutine_origin_tracking_depth", _PyCFunction_CAST(sys_set_coroutine_origin_tracking_depth), METH_FASTCALL|METH_KEYWORDS, sys_set_coroutine_origin_tracking_depth__doc__},

static PyObject *
sys_set_coroutine_origin_tracking_depth_impl(PyObject *module, int depth);

static PyObject *
sys_set_coroutine_origin_tracking_depth(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(depth), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"depth", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_coroutine_origin_tracking_depth",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int depth;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    depth = PyLong_AsInt(args[0]);
    if (depth == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_set_coroutine_origin_tracking_depth_impl(module, depth);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_coroutine_origin_tracking_depth__doc__,
"get_coroutine_origin_tracking_depth($module, /)\n"
"--\n"
"\n"
"Check status of origin tracking for coroutine objects in this thread.");

#define SYS_GET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF    \
    {"get_coroutine_origin_tracking_depth", (PyCFunction)sys_get_coroutine_origin_tracking_depth, METH_NOARGS, sys_get_coroutine_origin_tracking_depth__doc__},

static int
sys_get_coroutine_origin_tracking_depth_impl(PyObject *module);

static PyObject *
sys_get_coroutine_origin_tracking_depth(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = sys_get_coroutine_origin_tracking_depth_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_asyncgen_hooks__doc__,
"get_asyncgen_hooks($module, /)\n"
"--\n"
"\n"
"Return the installed asynchronous generators hooks.\n"
"\n"
"This returns a namedtuple of the form (firstiter, finalizer).");

#define SYS_GET_ASYNCGEN_HOOKS_METHODDEF    \
    {"get_asyncgen_hooks", (PyCFunction)sys_get_asyncgen_hooks, METH_NOARGS, sys_get_asyncgen_hooks__doc__},

static PyObject *
sys_get_asyncgen_hooks_impl(PyObject *module);

static PyObject *
sys_get_asyncgen_hooks(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_get_asyncgen_hooks_impl(module);
}

PyDoc_STRVAR(sys_getrecursionlimit__doc__,
"getrecursionlimit($module, /)\n"
"--\n"
"\n"
"Return the current value of the recursion limit.\n"
"\n"
"The recursion limit is the maximum depth of the Python interpreter\n"
"stack.  This limit prevents infinite recursion from causing an overflow\n"
"of the C stack and crashing Python.");

#define SYS_GETRECURSIONLIMIT_METHODDEF    \
    {"getrecursionlimit", (PyCFunction)sys_getrecursionlimit, METH_NOARGS, sys_getrecursionlimit__doc__},

static PyObject *
sys_getrecursionlimit_impl(PyObject *module);

static PyObject *
sys_getrecursionlimit(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getrecursionlimit_impl(module);
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(sys_getwindowsversion__doc__,
"getwindowsversion($module, /)\n"
"--\n"
"\n"
"Return info about the running version of Windows as a named tuple.\n"
"\n"
"The members are named: major, minor, build, platform, service_pack,\n"
"service_pack_major, service_pack_minor, suite_mask, product_type and\n"
"platform_version. For backward compatibility, only the first 5 items\n"
"are available by indexing. All elements are numbers, except\n"
"service_pack and platform_type which are strings, and platform_version\n"
"which is a 3-tuple. Platform is always 2. Product_type may be 1 for a\n"
"workstation, 2 for a domain controller, 3 for a server.\n"
"Platform_version is a 3-tuple containing a version number that is\n"
"intended for identifying the OS rather than feature detection.");

#define SYS_GETWINDOWSVERSION_METHODDEF    \
    {"getwindowsversion", (PyCFunction)sys_getwindowsversion, METH_NOARGS, sys_getwindowsversion__doc__},

static PyObject *
sys_getwindowsversion_impl(PyObject *module);

static PyObject *
sys_getwindowsversion(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getwindowsversion_impl(module);
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(sys__enablelegacywindowsfsencoding__doc__,
"_enablelegacywindowsfsencoding($module, /)\n"
"--\n"
"\n"
"Changes the default filesystem encoding to mbcs:replace.\n"
"\n"
"This is done for consistency with earlier versions of Python. See PEP\n"
"529 for more information.\n"
"\n"
"This is equivalent to defining the PYTHONLEGACYWINDOWSFSENCODING\n"
"environment variable before launching Python.");

#define SYS__ENABLELEGACYWINDOWSFSENCODING_METHODDEF    \
    {"_enablelegacywindowsfsencoding", (PyCFunction)sys__enablelegacywindowsfsencoding, METH_NOARGS, sys__enablelegacywindowsfsencoding__doc__},

static PyObject *
sys__enablelegacywindowsfsencoding_impl(PyObject *module);

static PyObject *
sys__enablelegacywindowsfsencoding(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__enablelegacywindowsfsencoding_impl(module);
}

#endif /* defined(MS_WINDOWS) */

#if defined(HAVE_DLOPEN)

PyDoc_STRVAR(sys_setdlopenflags__doc__,
"setdlopenflags($module, flags, /)\n"
"--\n"
"\n"
"Set the flags used by the interpreter for dlopen calls.\n"
"\n"
"This is used, for example, when the interpreter loads extension\n"
"modules. Among other things, this will enable a lazy resolving of\n"
"symbols when importing a module, if called as sys.setdlopenflags(0).\n"
"To share symbols across extension modules, call as\n"
"sys.setdlopenflags(os.RTLD_GLOBAL).  Symbolic names for the flag\n"
"modules can be found in the os module (RTLD_xxx constants, e.g.\n"
"os.RTLD_LAZY).");

#define SYS_SETDLOPENFLAGS_METHODDEF    \
    {"setdlopenflags", (PyCFunction)sys_setdlopenflags, METH_O, sys_setdlopenflags__doc__},

static PyObject *
sys_setdlopenflags_impl(PyObject *module, int new_val);

static PyObject *
sys_setdlopenflags(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int new_val;

    new_val = PyLong_AsInt(arg);
    if (new_val == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_setdlopenflags_impl(module, new_val);

exit:
    return return_value;
}

#endif /* defined(HAVE_DLOPEN) */

#if defined(HAVE_DLOPEN)

PyDoc_STRVAR(sys_getdlopenflags__doc__,
"getdlopenflags($module, /)\n"
"--\n"
"\n"
"Return the current value of the flags that are used for dlopen calls.\n"
"\n"
"The flag constants are defined in the os module.");

#define SYS_GETDLOPENFLAGS_METHODDEF    \
    {"getdlopenflags", (PyCFunction)sys_getdlopenflags, METH_NOARGS, sys_getdlopenflags__doc__},

static PyObject *
sys_getdlopenflags_impl(PyObject *module);

static PyObject *
sys_getdlopenflags(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getdlopenflags_impl(module);
}

#endif /* defined(HAVE_DLOPEN) */

#if defined(USE_MALLOPT)

PyDoc_STRVAR(sys_mdebug__doc__,
"mdebug($module, flag, /)\n"
"--\n"
"\n");

#define SYS_MDEBUG_METHODDEF    \
    {"mdebug", (PyCFunction)sys_mdebug, METH_O, sys_mdebug__doc__},

static PyObject *
sys_mdebug_impl(PyObject *module, int flag);

static PyObject *
sys_mdebug(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int flag;

    flag = PyLong_AsInt(arg);
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_mdebug_impl(module, flag);

exit:
    return return_value;
}

#endif /* defined(USE_MALLOPT) */

PyDoc_STRVAR(sys_get_int_max_str_digits__doc__,
"get_int_max_str_digits($module, /)\n"
"--\n"
"\n"
"Return the maximum string digits limit for non-binary int<->str conversions.");

#define SYS_GET_INT_MAX_STR_DIGITS_METHODDEF    \
    {"get_int_max_str_digits", (PyCFunction)sys_get_int_max_str_digits, METH_NOARGS, sys_get_int_max_str_digits__doc__},

static PyObject *
sys_get_int_max_str_digits_impl(PyObject *module);

static PyObject *
sys_get_int_max_str_digits(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_get_int_max_str_digits_impl(module);
}

PyDoc_STRVAR(sys_set_int_max_str_digits__doc__,
"set_int_max_str_digits($module, /, maxdigits)\n"
"--\n"
"\n"
"Set the maximum string digits limit for non-binary int<->str conversions.");

#define SYS_SET_INT_MAX_STR_DIGITS_METHODDEF    \
    {"set_int_max_str_digits", _PyCFunction_CAST(sys_set_int_max_str_digits), METH_FASTCALL|METH_KEYWORDS, sys_set_int_max_str_digits__doc__},

static PyObject *
sys_set_int_max_str_digits_impl(PyObject *module, int maxdigits);

static PyObject *
sys_set_int_max_str_digits(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(maxdigits), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"maxdigits", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_int_max_str_digits",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int maxdigits;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    maxdigits = PyLong_AsInt(args[0]);
    if (maxdigits == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_set_int_max_str_digits_impl(module, maxdigits);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getrefcount__doc__,
"getrefcount($module, object, /)\n"
"--\n"
"\n"
"Return the reference count of object.\n"
"\n"
"The count returned is generally one higher than you might expect,\n"
"because it includes the (temporary) reference as an argument to\n"
"getrefcount().");

#define SYS_GETREFCOUNT_METHODDEF    \
    {"getrefcount", (PyCFunction)sys_getrefcount, METH_O, sys_getrefcount__doc__},

static Py_ssize_t
sys_getrefcount_impl(PyObject *module, PyObject *object);

static PyObject *
sys_getrefcount(PyObject *module, PyObject *object)
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = sys_getrefcount_impl(module, object);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

#if defined(Py_REF_DEBUG)

PyDoc_STRVAR(sys_gettotalrefcount__doc__,
"gettotalrefcount($module, /)\n"
"--\n"
"\n");

#define SYS_GETTOTALREFCOUNT_METHODDEF    \
    {"gettotalrefcount", (PyCFunction)sys_gettotalrefcount, METH_NOARGS, sys_gettotalrefcount__doc__},

static Py_ssize_t
sys_gettotalrefcount_impl(PyObject *module);

static PyObject *
sys_gettotalrefcount(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = sys_gettotalrefcount_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

#endif /* defined(Py_REF_DEBUG) */

PyDoc_STRVAR(sys_getallocatedblocks__doc__,
"getallocatedblocks($module, /)\n"
"--\n"
"\n"
"Return the number of memory blocks currently allocated.");

#define SYS_GETALLOCATEDBLOCKS_METHODDEF    \
    {"getallocatedblocks", (PyCFunction)sys_getallocatedblocks, METH_NOARGS, sys_getallocatedblocks__doc__},

static Py_ssize_t
sys_getallocatedblocks_impl(PyObject *module);

static PyObject *
sys_getallocatedblocks(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = sys_getallocatedblocks_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getunicodeinternedsize__doc__,
"getunicodeinternedsize($module, /, *, _only_immortal=False)\n"
"--\n"
"\n"
"Return the number of elements of the unicode interned dictionary");

#define SYS_GETUNICODEINTERNEDSIZE_METHODDEF    \
    {"getunicodeinternedsize", _PyCFunction_CAST(sys_getunicodeinternedsize), METH_FASTCALL|METH_KEYWORDS, sys_getunicodeinternedsize__doc__},

static Py_ssize_t
sys_getunicodeinternedsize_impl(PyObject *module, int _only_immortal);

static PyObject *
sys_getunicodeinternedsize(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(_only_immortal), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"_only_immortal", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "getunicodeinternedsize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int _only_immortal = 0;
    Py_ssize_t _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    _only_immortal = PyObject_IsTrue(args[0]);
    if (_only_immortal < 0) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = sys_getunicodeinternedsize_impl(module, _only_immortal);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__getframe__doc__,
"_getframe($module, depth=0, /)\n"
"--\n"
"\n"
"Return a frame object from the call stack.\n"
"\n"
"If optional integer depth is given, return the frame object that many\n"
"calls below the top of the stack.  If that is deeper than the call\n"
"stack, ValueError is raised.  The default for depth is zero, returning\n"
"the frame at the top of the call stack.\n"
"\n"
"This function should be used for internal and specialized purposes\n"
"only.");

#define SYS__GETFRAME_METHODDEF    \
    {"_getframe", _PyCFunction_CAST(sys__getframe), METH_FASTCALL, sys__getframe__doc__},

static PyObject *
sys__getframe_impl(PyObject *module, int depth);

static PyObject *
sys__getframe(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int depth = 0;

    if (!_PyArg_CheckPositional("_getframe", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    depth = PyLong_AsInt(args[0]);
    if (depth == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = sys__getframe_impl(module, depth);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__current_frames__doc__,
"_current_frames($module, /)\n"
"--\n"
"\n"
"Return a dict mapping each thread\'s thread id to its current stack frame.\n"
"\n"
"This function should be used for specialized purposes only.");

#define SYS__CURRENT_FRAMES_METHODDEF    \
    {"_current_frames", (PyCFunction)sys__current_frames, METH_NOARGS, sys__current_frames__doc__},

static PyObject *
sys__current_frames_impl(PyObject *module);

static PyObject *
sys__current_frames(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__current_frames_impl(module);
}

PyDoc_STRVAR(sys__current_exceptions__doc__,
"_current_exceptions($module, /)\n"
"--\n"
"\n"
"Return a dict mapping each thread\'s identifier to its current raised exception.\n"
"\n"
"This function should be used for specialized purposes only.");

#define SYS__CURRENT_EXCEPTIONS_METHODDEF    \
    {"_current_exceptions", (PyCFunction)sys__current_exceptions, METH_NOARGS, sys__current_exceptions__doc__},

static PyObject *
sys__current_exceptions_impl(PyObject *module);

static PyObject *
sys__current_exceptions(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__current_exceptions_impl(module);
}

PyDoc_STRVAR(sys_call_tracing__doc__,
"call_tracing($module, func, args, /)\n"
"--\n"
"\n"
"Call func(*args), while tracing is enabled.\n"
"\n"
"The tracing state is saved, and restored afterwards.  This is intended\n"
"to be called from a debugger from a checkpoint, to recursively debug\n"
"some other code.");

#define SYS_CALL_TRACING_METHODDEF    \
    {"call_tracing", _PyCFunction_CAST(sys_call_tracing), METH_FASTCALL, sys_call_tracing__doc__},

static PyObject *
sys_call_tracing_impl(PyObject *module, PyObject *func, PyObject *funcargs);

static PyObject *
sys_call_tracing(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *funcargs;

    if (!_PyArg_CheckPositional("call_tracing", nargs, 2, 2)) {
        goto exit;
    }
    func = args[0];
    if (!PyTuple_Check(args[1])) {
        _PyArg_BadArgument("call_tracing", "argument 2", "tuple", args[1]);
        goto exit;
    }
    funcargs = args[1];
    return_value = sys_call_tracing_impl(module, func, funcargs);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__debugmallocstats__doc__,
"_debugmallocstats($module, /)\n"
"--\n"
"\n"
"Print summary info to stderr about the state of pymalloc\'s structures.\n"
"\n"
"In Py_DEBUG mode, also perform some expensive internal consistency\n"
"checks.");

#define SYS__DEBUGMALLOCSTATS_METHODDEF    \
    {"_debugmallocstats", (PyCFunction)sys__debugmallocstats, METH_NOARGS, sys__debugmallocstats__doc__},

static PyObject *
sys__debugmallocstats_impl(PyObject *module);

static PyObject *
sys__debugmallocstats(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__debugmallocstats_impl(module);
}

PyDoc_STRVAR(sys__clear_type_cache__doc__,
"_clear_type_cache($module, /)\n"
"--\n"
"\n"
"Clear the internal type lookup cache.");

#define SYS__CLEAR_TYPE_CACHE_METHODDEF    \
    {"_clear_type_cache", (PyCFunction)sys__clear_type_cache, METH_NOARGS, sys__clear_type_cache__doc__},

static PyObject *
sys__clear_type_cache_impl(PyObject *module);

static PyObject *
sys__clear_type_cache(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__clear_type_cache_impl(module);
}

PyDoc_STRVAR(sys__clear_internal_caches__doc__,
"_clear_internal_caches($module, /)\n"
"--\n"
"\n"
"Clear all internal performance-related caches.");

#define SYS__CLEAR_INTERNAL_CACHES_METHODDEF    \
    {"_clear_internal_caches", (PyCFunction)sys__clear_internal_caches, METH_NOARGS, sys__clear_internal_caches__doc__},

static PyObject *
sys__clear_internal_caches_impl(PyObject *module);

static PyObject *
sys__clear_internal_caches(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__clear_internal_caches_impl(module);
}

PyDoc_STRVAR(sys_is_finalizing__doc__,
"is_finalizing($module, /)\n"
"--\n"
"\n"
"Return True if Python is exiting.");

#define SYS_IS_FINALIZING_METHODDEF    \
    {"is_finalizing", (PyCFunction)sys_is_finalizing, METH_NOARGS, sys_is_finalizing__doc__},

static PyObject *
sys_is_finalizing_impl(PyObject *module);

static PyObject *
sys_is_finalizing(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_is_finalizing_impl(module);
}

#if defined(Py_STATS)

PyDoc_STRVAR(sys__stats_on__doc__,
"_stats_on($module, /)\n"
"--\n"
"\n"
"Turns on stats gathering (stats gathering is off by default).");

#define SYS__STATS_ON_METHODDEF    \
    {"_stats_on", (PyCFunction)sys__stats_on, METH_NOARGS, sys__stats_on__doc__},

static PyObject *
sys__stats_on_impl(PyObject *module);

static PyObject *
sys__stats_on(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__stats_on_impl(module);
}

#endif /* defined(Py_STATS) */

#if defined(Py_STATS)

PyDoc_STRVAR(sys__stats_off__doc__,
"_stats_off($module, /)\n"
"--\n"
"\n"
"Turns off stats gathering (stats gathering is off by default).");

#define SYS__STATS_OFF_METHODDEF    \
    {"_stats_off", (PyCFunction)sys__stats_off, METH_NOARGS, sys__stats_off__doc__},

static PyObject *
sys__stats_off_impl(PyObject *module);

static PyObject *
sys__stats_off(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__stats_off_impl(module);
}

#endif /* defined(Py_STATS) */

#if defined(Py_STATS)

PyDoc_STRVAR(sys__stats_clear__doc__,
"_stats_clear($module, /)\n"
"--\n"
"\n"
"Clears the stats.");

#define SYS__STATS_CLEAR_METHODDEF    \
    {"_stats_clear", (PyCFunction)sys__stats_clear, METH_NOARGS, sys__stats_clear__doc__},

static PyObject *
sys__stats_clear_impl(PyObject *module);

static PyObject *
sys__stats_clear(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__stats_clear_impl(module);
}

#endif /* defined(Py_STATS) */

#if defined(Py_STATS)

PyDoc_STRVAR(sys__stats_dump__doc__,
"_stats_dump($module, /)\n"
"--\n"
"\n"
"Dump stats to file, and clears the stats.\n"
"\n"
"Return False if no statistics were not dumped because stats gathering was off.");

#define SYS__STATS_DUMP_METHODDEF    \
    {"_stats_dump", (PyCFunction)sys__stats_dump, METH_NOARGS, sys__stats_dump__doc__},

static int
sys__stats_dump_impl(PyObject *module);

static PyObject *
sys__stats_dump(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = sys__stats_dump_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(Py_STATS) */

#if defined(ANDROID_API_LEVEL)

PyDoc_STRVAR(sys_getandroidapilevel__doc__,
"getandroidapilevel($module, /)\n"
"--\n"
"\n"
"Return the build time API version of Android as an integer.");

#define SYS_GETANDROIDAPILEVEL_METHODDEF    \
    {"getandroidapilevel", (PyCFunction)sys_getandroidapilevel, METH_NOARGS, sys_getandroidapilevel__doc__},

static PyObject *
sys_getandroidapilevel_impl(PyObject *module);

static PyObject *
sys_getandroidapilevel(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getandroidapilevel_impl(module);
}

#endif /* defined(ANDROID_API_LEVEL) */

PyDoc_STRVAR(sys_activate_stack_trampoline__doc__,
"activate_stack_trampoline($module, backend, /)\n"
"--\n"
"\n"
"Activate stack profiler trampoline *backend*.");

#define SYS_ACTIVATE_STACK_TRAMPOLINE_METHODDEF    \
    {"activate_stack_trampoline", (PyCFunction)sys_activate_stack_trampoline, METH_O, sys_activate_stack_trampoline__doc__},

static PyObject *
sys_activate_stack_trampoline_impl(PyObject *module, const char *backend);

static PyObject *
sys_activate_stack_trampoline(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *backend;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("activate_stack_trampoline", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t backend_length;
    backend = PyUnicode_AsUTF8AndSize(arg, &backend_length);
    if (backend == NULL) {
        goto exit;
    }
    if (strlen(backend) != (size_t)backend_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = sys_activate_stack_trampoline_impl(module, backend);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_deactivate_stack_trampoline__doc__,
"deactivate_stack_trampoline($module, /)\n"
"--\n"
"\n"
"Deactivate the current stack profiler trampoline backend.\n"
"\n"
"If no stack profiler is activated, this function has no effect.");

#define SYS_DEACTIVATE_STACK_TRAMPOLINE_METHODDEF    \
    {"deactivate_stack_trampoline", (PyCFunction)sys_deactivate_stack_trampoline, METH_NOARGS, sys_deactivate_stack_trampoline__doc__},

static PyObject *
sys_deactivate_stack_trampoline_impl(PyObject *module);

static PyObject *
sys_deactivate_stack_trampoline(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_deactivate_stack_trampoline_impl(module);
}

PyDoc_STRVAR(sys_is_stack_trampoline_active__doc__,
"is_stack_trampoline_active($module, /)\n"
"--\n"
"\n"
"Return *True* if a stack profiler trampoline is active.");

#define SYS_IS_STACK_TRAMPOLINE_ACTIVE_METHODDEF    \
    {"is_stack_trampoline_active", (PyCFunction)sys_is_stack_trampoline_active, METH_NOARGS, sys_is_stack_trampoline_active__doc__},

static PyObject *
sys_is_stack_trampoline_active_impl(PyObject *module);

static PyObject *
sys_is_stack_trampoline_active(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_is_stack_trampoline_active_impl(module);
}

PyDoc_STRVAR(sys_is_remote_debug_enabled__doc__,
"is_remote_debug_enabled($module, /)\n"
"--\n"
"\n"
"Return True if remote debugging is enabled, False otherwise.");

#define SYS_IS_REMOTE_DEBUG_ENABLED_METHODDEF    \
    {"is_remote_debug_enabled", (PyCFunction)sys_is_remote_debug_enabled, METH_NOARGS, sys_is_remote_debug_enabled__doc__},

static PyObject *
sys_is_remote_debug_enabled_impl(PyObject *module);

static PyObject *
sys_is_remote_debug_enabled(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_is_remote_debug_enabled_impl(module);
}

PyDoc_STRVAR(sys_remote_exec__doc__,
"remote_exec($module, /, pid, script)\n"
"--\n"
"\n"
"Executes a file containing Python code in a given remote Python process.\n"
"\n"
"This function returns immediately, and the code will be executed by the\n"
"target process\'s main thread at the next available opportunity, similarly\n"
"to how signals are handled. There is no interface to determine when the\n"
"code has been executed. The caller is responsible for making sure that\n"
"the file still exists whenever the remote process tries to read it and that\n"
"it hasn\'t been overwritten.\n"
"\n"
"The remote process must be running a CPython interpreter of the same major\n"
"and minor version as the local process. If either the local or remote\n"
"interpreter is pre-release (alpha, beta, or release candidate) then the\n"
"local and remote interpreters must be the same exact version.\n"
"\n"
"Args:\n"
"     pid (int): The process ID of the target Python process.\n"
"     script (str|bytes): The path to a file containing\n"
"         the Python code to be executed.");

#define SYS_REMOTE_EXEC_METHODDEF    \
    {"remote_exec", _PyCFunction_CAST(sys_remote_exec), METH_FASTCALL|METH_KEYWORDS, sys_remote_exec__doc__},

static PyObject *
sys_remote_exec_impl(PyObject *module, int pid, PyObject *script);

static PyObject *
sys_remote_exec(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(pid), &_Py_ID(script), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "script", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "remote_exec",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    int pid;
    PyObject *script;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    pid = PyLong_AsInt(args[0]);
    if (pid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    script = args[1];
    return_value = sys_remote_exec_impl(module, pid, script);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__dump_tracelets__doc__,
"_dump_tracelets($module, /, outpath)\n"
"--\n"
"\n"
"Dump the graph of tracelets in graphviz format");

#define SYS__DUMP_TRACELETS_METHODDEF    \
    {"_dump_tracelets", _PyCFunction_CAST(sys__dump_tracelets), METH_FASTCALL|METH_KEYWORDS, sys__dump_tracelets__doc__},

static PyObject *
sys__dump_tracelets_impl(PyObject *module, PyObject *outpath);

static PyObject *
sys__dump_tracelets(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(outpath), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"outpath", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_dump_tracelets",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *outpath;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    outpath = args[0];
    return_value = sys__dump_tracelets_impl(module, outpath);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__getframemodulename__doc__,
"_getframemodulename($module, /, depth=0)\n"
"--\n"
"\n"
"Return the name of the module for a calling frame.\n"
"\n"
"The default depth returns the module containing the call to this API.\n"
"A more typical use in a library will pass a depth of 1 to get the user\'s\n"
"module rather than the library module.\n"
"\n"
"If no frame, module, or name can be found, returns None.");

#define SYS__GETFRAMEMODULENAME_METHODDEF    \
    {"_getframemodulename", _PyCFunction_CAST(sys__getframemodulename), METH_FASTCALL|METH_KEYWORDS, sys__getframemodulename__doc__},

static PyObject *
sys__getframemodulename_impl(PyObject *module, int depth);

static PyObject *
sys__getframemodulename(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(depth), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"depth", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_getframemodulename",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int depth = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    depth = PyLong_AsInt(args[0]);
    if (depth == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = sys__getframemodulename_impl(module, depth);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__get_cpu_count_config__doc__,
"_get_cpu_count_config($module, /)\n"
"--\n"
"\n"
"Private function for getting PyConfig.cpu_count");

#define SYS__GET_CPU_COUNT_CONFIG_METHODDEF    \
    {"_get_cpu_count_config", (PyCFunction)sys__get_cpu_count_config, METH_NOARGS, sys__get_cpu_count_config__doc__},

static int
sys__get_cpu_count_config_impl(PyObject *module);

static PyObject *
sys__get_cpu_count_config(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = sys__get_cpu_count_config_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__baserepl__doc__,
"_baserepl($module, /)\n"
"--\n"
"\n"
"Private function for getting the base REPL");

#define SYS__BASEREPL_METHODDEF    \
    {"_baserepl", (PyCFunction)sys__baserepl, METH_NOARGS, sys__baserepl__doc__},

static PyObject *
sys__baserepl_impl(PyObject *module);

static PyObject *
sys__baserepl(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys__baserepl_impl(module);
}

PyDoc_STRVAR(sys__is_gil_enabled__doc__,
"_is_gil_enabled($module, /)\n"
"--\n"
"\n"
"Return True if the GIL is currently enabled and False otherwise.");

#define SYS__IS_GIL_ENABLED_METHODDEF    \
    {"_is_gil_enabled", (PyCFunction)sys__is_gil_enabled, METH_NOARGS, sys__is_gil_enabled__doc__},

static int
sys__is_gil_enabled_impl(PyObject *module);

static PyObject *
sys__is_gil_enabled(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = sys__is_gil_enabled_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_set_lazy_imports_filter__doc__,
"set_lazy_imports_filter($module, /, filter)\n"
"--\n"
"\n"
"Set the lazy imports filter callback.\n"
"\n"
"The filter is a callable which disables lazy imports when they\n"
"would otherwise be enabled. Returns True if the import is still enabled\n"
"or False to disable it. The callable is called with:\n"
"\n"
"(importing_module_name, imported_module_name, [fromlist])\n"
"\n"
"Pass None to clear the filter.");

#define SYS_SET_LAZY_IMPORTS_FILTER_METHODDEF    \
    {"set_lazy_imports_filter", _PyCFunction_CAST(sys_set_lazy_imports_filter), METH_FASTCALL|METH_KEYWORDS, sys_set_lazy_imports_filter__doc__},

static PyObject *
sys_set_lazy_imports_filter_impl(PyObject *module, PyObject *filter);

static PyObject *
sys_set_lazy_imports_filter(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(filter), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"filter", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_lazy_imports_filter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *filter;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    filter = args[0];
    return_value = sys_set_lazy_imports_filter_impl(module, filter);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_lazy_imports_filter__doc__,
"get_lazy_imports_filter($module, /)\n"
"--\n"
"\n"
"Get the current lazy imports filter callback.\n"
"\n"
"Returns the filter callable or None if no filter is set.");

#define SYS_GET_LAZY_IMPORTS_FILTER_METHODDEF    \
    {"get_lazy_imports_filter", (PyCFunction)sys_get_lazy_imports_filter, METH_NOARGS, sys_get_lazy_imports_filter__doc__},

static PyObject *
sys_get_lazy_imports_filter_impl(PyObject *module);

static PyObject *
sys_get_lazy_imports_filter(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_get_lazy_imports_filter_impl(module);
}

PyDoc_STRVAR(sys_set_lazy_imports__doc__,
"set_lazy_imports($module, /, mode)\n"
"--\n"
"\n"
"Sets the global lazy imports mode.\n"
"\n"
"The mode parameter must be one of the following strings:\n"
"- \"all\": All top-level imports become potentially lazy\n"
"- \"none\": All lazy imports are suppressed (even explicitly marked ones)\n"
"- \"normal\": Only explicitly marked imports (with \'lazy\' keyword) are lazy\n"
"\n"
"In addition to the mode, lazy imports can be controlled via the filter\n"
"provided to sys.set_lazy_imports_filter");

#define SYS_SET_LAZY_IMPORTS_METHODDEF    \
    {"set_lazy_imports", _PyCFunction_CAST(sys_set_lazy_imports), METH_FASTCALL|METH_KEYWORDS, sys_set_lazy_imports__doc__},

static PyObject *
sys_set_lazy_imports_impl(PyObject *module, PyObject *mode);

static PyObject *
sys_set_lazy_imports(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(mode), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"mode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_lazy_imports",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *mode;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    mode = args[0];
    return_value = sys_set_lazy_imports_impl(module, mode);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_lazy_imports__doc__,
"get_lazy_imports($module, /)\n"
"--\n"
"\n"
"Gets the global lazy imports mode.\n"
"\n"
"Returns \"all\" if all top level imports are potentially lazy.\n"
"Returns \"none\" if all explicitly marked lazy imports are suppressed.\n"
"Returns \"normal\" if only explicitly marked imports are lazy.");

#define SYS_GET_LAZY_IMPORTS_METHODDEF    \
    {"get_lazy_imports", (PyCFunction)sys_get_lazy_imports, METH_NOARGS, sys_get_lazy_imports__doc__},

static PyObject *
sys_get_lazy_imports_impl(PyObject *module);

static PyObject *
sys_get_lazy_imports(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_get_lazy_imports_impl(module);
}

PyDoc_STRVAR(_jit_is_available__doc__,
"is_available($module, /)\n"
"--\n"
"\n"
"Return True if the current Python executable supports JIT compilation, and False otherwise.");

#define _JIT_IS_AVAILABLE_METHODDEF    \
    {"is_available", (PyCFunction)_jit_is_available, METH_NOARGS, _jit_is_available__doc__},

static int
_jit_is_available_impl(PyObject *module);

static PyObject *
_jit_is_available(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _jit_is_available_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_jit_is_enabled__doc__,
"is_enabled($module, /)\n"
"--\n"
"\n"
"Return True if JIT compilation is enabled for the current Python process (implies sys._jit.is_available()), and False otherwise.");

#define _JIT_IS_ENABLED_METHODDEF    \
    {"is_enabled", (PyCFunction)_jit_is_enabled, METH_NOARGS, _jit_is_enabled__doc__},

static int
_jit_is_enabled_impl(PyObject *module);

static PyObject *
_jit_is_enabled(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _jit_is_enabled_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_jit_is_active__doc__,
"is_active($module, /)\n"
"--\n"
"\n"
"Return True if the topmost Python frame is currently executing JIT code (implies sys._jit.is_enabled()), and False otherwise.");

#define _JIT_IS_ACTIVE_METHODDEF    \
    {"is_active", (PyCFunction)_jit_is_active, METH_NOARGS, _jit_is_active__doc__},

static int
_jit_is_active_impl(PyObject *module);

static PyObject *
_jit_is_active(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _jit_is_active_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

#ifndef SYS_GETWINDOWSVERSION_METHODDEF
    #define SYS_GETWINDOWSVERSION_METHODDEF
#endif /* !defined(SYS_GETWINDOWSVERSION_METHODDEF) */

#ifndef SYS__ENABLELEGACYWINDOWSFSENCODING_METHODDEF
    #define SYS__ENABLELEGACYWINDOWSFSENCODING_METHODDEF
#endif /* !defined(SYS__ENABLELEGACYWINDOWSFSENCODING_METHODDEF) */

#ifndef SYS_SETDLOPENFLAGS_METHODDEF
    #define SYS_SETDLOPENFLAGS_METHODDEF
#endif /* !defined(SYS_SETDLOPENFLAGS_METHODDEF) */

#ifndef SYS_GETDLOPENFLAGS_METHODDEF
    #define SYS_GETDLOPENFLAGS_METHODDEF
#endif /* !defined(SYS_GETDLOPENFLAGS_METHODDEF) */

#ifndef SYS_MDEBUG_METHODDEF
    #define SYS_MDEBUG_METHODDEF
#endif /* !defined(SYS_MDEBUG_METHODDEF) */

#ifndef SYS_GETTOTALREFCOUNT_METHODDEF
    #define SYS_GETTOTALREFCOUNT_METHODDEF
#endif /* !defined(SYS_GETTOTALREFCOUNT_METHODDEF) */

#ifndef SYS__STATS_ON_METHODDEF
    #define SYS__STATS_ON_METHODDEF
#endif /* !defined(SYS__STATS_ON_METHODDEF) */

#ifndef SYS__STATS_OFF_METHODDEF
    #define SYS__STATS_OFF_METHODDEF
#endif /* !defined(SYS__STATS_OFF_METHODDEF) */

#ifndef SYS__STATS_CLEAR_METHODDEF
    #define SYS__STATS_CLEAR_METHODDEF
#endif /* !defined(SYS__STATS_CLEAR_METHODDEF) */

#ifndef SYS__STATS_DUMP_METHODDEF
    #define SYS__STATS_DUMP_METHODDEF
#endif /* !defined(SYS__STATS_DUMP_METHODDEF) */

#ifndef SYS_GETANDROIDAPILEVEL_METHODDEF
    #define SYS_GETANDROIDAPILEVEL_METHODDEF
#endif /* !defined(SYS_GETANDROIDAPILEVEL_METHODDEF) */
/*[clinic end generated code: output=adbadb629b98eabf input=a9049054013a1b77]*/
