/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(sys_addaudithook__doc__,
"addaudithook($module, /, hook)\n"
"--\n"
"\n"
"Adds a new audit hook callback.");

#define SYS_ADDAUDITHOOK_METHODDEF    \
    {"addaudithook", (PyCFunction)(void(*)(void))sys_addaudithook, METH_FASTCALL|METH_KEYWORDS, sys_addaudithook__doc__},

static PyObject *
sys_addaudithook_impl(PyObject *module, PyObject *hook);

static PyObject *
sys_addaudithook(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"hook", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "addaudithook", 0};
    PyObject *argsbuf[1];
    PyObject *hook;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    hook = args[0];
    return_value = sys_addaudithook_impl(module, hook);

exit:
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
    {"excepthook", (PyCFunction)(void(*)(void))sys_excepthook, METH_FASTCALL, sys_excepthook__doc__},

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
    {"exit", (PyCFunction)(void(*)(void))sys_exit, METH_FASTCALL, sys_exit__doc__},

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
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    s = arg;
    return_value = sys_intern_impl(module, s);

exit:
    return return_value;
}

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

PyDoc_STRVAR(sys_setcheckinterval__doc__,
"setcheckinterval($module, n, /)\n"
"--\n"
"\n"
"Set the async event check interval to n instructions.\n"
"\n"
"This tells the Python interpreter to check for asynchronous events\n"
"every n instructions.\n"
"\n"
"This also affects how often thread switches occur.");

#define SYS_SETCHECKINTERVAL_METHODDEF    \
    {"setcheckinterval", (PyCFunction)sys_setcheckinterval, METH_O, sys_setcheckinterval__doc__},

static PyObject *
sys_setcheckinterval_impl(PyObject *module, int n);

static PyObject *
sys_setcheckinterval(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int n;

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    n = _PyLong_AsInt(arg);
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_setcheckinterval_impl(module, n);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getcheckinterval__doc__,
"getcheckinterval($module, /)\n"
"--\n"
"\n"
"Return the current check interval; see sys.setcheckinterval().");

#define SYS_GETCHECKINTERVAL_METHODDEF    \
    {"getcheckinterval", (PyCFunction)sys_getcheckinterval, METH_NOARGS, sys_getcheckinterval__doc__},

static PyObject *
sys_getcheckinterval_impl(PyObject *module);

static PyObject *
sys_getcheckinterval(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getcheckinterval_impl(module);
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

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    new_limit = _PyLong_AsInt(arg);
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
    {"set_coroutine_origin_tracking_depth", (PyCFunction)(void(*)(void))sys_set_coroutine_origin_tracking_depth, METH_FASTCALL|METH_KEYWORDS, sys_set_coroutine_origin_tracking_depth__doc__},

static PyObject *
sys_set_coroutine_origin_tracking_depth_impl(PyObject *module, int depth);

static PyObject *
sys_set_coroutine_origin_tracking_depth(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"depth", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "set_coroutine_origin_tracking_depth", 0};
    PyObject *argsbuf[1];
    int depth;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    depth = _PyLong_AsInt(args[0]);
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

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    new_val = _PyLong_AsInt(arg);
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

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    flag = _PyLong_AsInt(arg);
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
"Set the maximum string digits limit for non-binary int<->str conversions.");

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
    {"set_int_max_str_digits", (PyCFunction)(void(*)(void))sys_set_int_max_str_digits, METH_FASTCALL|METH_KEYWORDS, sys_set_int_max_str_digits__doc__},

static PyObject *
sys_set_int_max_str_digits_impl(PyObject *module, int maxdigits);

static PyObject *
sys_set_int_max_str_digits(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"maxdigits", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "set_int_max_str_digits", 0};
    PyObject *argsbuf[1];
    int maxdigits;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    maxdigits = _PyLong_AsInt(args[0]);
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

#if defined(COUNT_ALLOCS)

PyDoc_STRVAR(sys_getcounts__doc__,
"getcounts($module, /)\n"
"--\n"
"\n");

#define SYS_GETCOUNTS_METHODDEF    \
    {"getcounts", (PyCFunction)sys_getcounts, METH_NOARGS, sys_getcounts__doc__},

static PyObject *
sys_getcounts_impl(PyObject *module);

static PyObject *
sys_getcounts(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_getcounts_impl(module);
}

#endif /* defined(COUNT_ALLOCS) */

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
    {"_getframe", (PyCFunction)(void(*)(void))sys__getframe, METH_FASTCALL, sys__getframe__doc__},

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
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    depth = _PyLong_AsInt(args[0]);
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
    {"call_tracing", (PyCFunction)(void(*)(void))sys_call_tracing, METH_FASTCALL, sys_call_tracing__doc__},

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

PyDoc_STRVAR(sys_callstats__doc__,
"callstats($module, /)\n"
"--\n"
"\n"
"Return a tuple of function call statistics.\n"
"\n"
"A tuple is returned only if CALL_PROFILE was defined when Python was\n"
"built.  Otherwise, this returns None.\n"
"\n"
"When enabled, this function returns detailed, implementation-specific\n"
"details about the number of function calls executed. The return value\n"
"is a 11-tuple where the entries in the tuple are counts of:\n"
"0. all function calls\n"
"1. calls to PyFunction_Type objects\n"
"2. PyFunction calls that do not create an argument tuple\n"
"3. PyFunction calls that do not create an argument tuple\n"
"   and bypass PyEval_EvalCodeEx()\n"
"4. PyMethod calls\n"
"5. PyMethod calls on bound methods\n"
"6. PyType calls\n"
"7. PyCFunction calls\n"
"8. generator calls\n"
"9. All other calls\n"
"10. Number of stack pops performed by call_function()");

#define SYS_CALLSTATS_METHODDEF    \
    {"callstats", (PyCFunction)sys_callstats, METH_NOARGS, sys_callstats__doc__},

static PyObject *
sys_callstats_impl(PyObject *module);

static PyObject *
sys_callstats(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return sys_callstats_impl(module);
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

#ifndef SYS_GETCOUNTS_METHODDEF
    #define SYS_GETCOUNTS_METHODDEF
#endif /* !defined(SYS_GETCOUNTS_METHODDEF) */

#ifndef SYS_GETANDROIDAPILEVEL_METHODDEF
    #define SYS_GETANDROIDAPILEVEL_METHODDEF
#endif /* !defined(SYS_GETANDROIDAPILEVEL_METHODDEF) */
/*[clinic end generated code: output=c41f7fa36ead9409 input=a9049054013a1b77]*/
