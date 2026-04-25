/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_long.h"          // _PyLong_UnsignedInt_Converter()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(faulthandler_dump_traceback_py__doc__,
"dump_traceback($module, /, file=sys.stderr, all_threads=True)\n"
"--\n"
"\n"
"Dump the traceback of the current thread into file.\n"
"\n"
"Dump the traceback of all threads if all_threads is true.");

#define FAULTHANDLER_DUMP_TRACEBACK_PY_METHODDEF    \
    {"dump_traceback", _PyCFunction_CAST(faulthandler_dump_traceback_py), METH_FASTCALL|METH_KEYWORDS, faulthandler_dump_traceback_py__doc__},

static PyObject *
faulthandler_dump_traceback_py_impl(PyObject *module, PyObject *file,
                                    int all_threads);

static PyObject *
faulthandler_dump_traceback_py(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(file), &_Py_ID(all_threads), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", "all_threads", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "dump_traceback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *file = NULL;
    int all_threads = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        file = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    all_threads = PyObject_IsTrue(args[1]);
    if (all_threads < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = faulthandler_dump_traceback_py_impl(module, file, all_threads);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_dump_c_stack_py__doc__,
"dump_c_stack($module, /, file=sys.stderr)\n"
"--\n"
"\n"
"Dump the C stack of the current thread.");

#define FAULTHANDLER_DUMP_C_STACK_PY_METHODDEF    \
    {"dump_c_stack", _PyCFunction_CAST(faulthandler_dump_c_stack_py), METH_FASTCALL|METH_KEYWORDS, faulthandler_dump_c_stack_py__doc__},

static PyObject *
faulthandler_dump_c_stack_py_impl(PyObject *module, PyObject *file);

static PyObject *
faulthandler_dump_c_stack_py(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(file), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "dump_c_stack",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *file = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    file = args[0];
skip_optional_pos:
    return_value = faulthandler_dump_c_stack_py_impl(module, file);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_py_enable__doc__,
"enable($module, /, file=sys.stderr, all_threads=True, c_stack=True)\n"
"--\n"
"\n"
"Enable the fault handler.");

#define FAULTHANDLER_PY_ENABLE_METHODDEF    \
    {"enable", _PyCFunction_CAST(faulthandler_py_enable), METH_FASTCALL|METH_KEYWORDS, faulthandler_py_enable__doc__},

static PyObject *
faulthandler_py_enable_impl(PyObject *module, PyObject *file,
                            int all_threads, int c_stack);

static PyObject *
faulthandler_py_enable(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(file), &_Py_ID(all_threads), &_Py_ID(c_stack), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", "all_threads", "c_stack", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *file = NULL;
    int all_threads = 1;
    int c_stack = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        file = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        all_threads = PyObject_IsTrue(args[1]);
        if (all_threads < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    c_stack = PyObject_IsTrue(args[2]);
    if (c_stack < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = faulthandler_py_enable_impl(module, file, all_threads, c_stack);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_disable_py__doc__,
"disable($module, /)\n"
"--\n"
"\n"
"Disable the fault handler.");

#define FAULTHANDLER_DISABLE_PY_METHODDEF    \
    {"disable", (PyCFunction)faulthandler_disable_py, METH_NOARGS, faulthandler_disable_py__doc__},

static PyObject *
faulthandler_disable_py_impl(PyObject *module);

static PyObject *
faulthandler_disable_py(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return faulthandler_disable_py_impl(module);
}

PyDoc_STRVAR(faulthandler_is_enabled__doc__,
"is_enabled($module, /)\n"
"--\n"
"\n"
"Check if the handler is enabled.");

#define FAULTHANDLER_IS_ENABLED_METHODDEF    \
    {"is_enabled", (PyCFunction)faulthandler_is_enabled, METH_NOARGS, faulthandler_is_enabled__doc__},

static int
faulthandler_is_enabled_impl(PyObject *module);

static PyObject *
faulthandler_is_enabled(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = faulthandler_is_enabled_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_dump_traceback_later__doc__,
"dump_traceback_later($module, /, timeout, repeat=False,\n"
"                     file=sys.stderr, exit=False)\n"
"--\n"
"\n"
"Dump the traceback of all threads in timeout seconds.\n"
"\n"
"If repeat is true, the tracebacks of all threads are dumped every timeout\n"
"seconds. If exit is true, call _exit(1) which is not safe.");

#define FAULTHANDLER_DUMP_TRACEBACK_LATER_METHODDEF    \
    {"dump_traceback_later", _PyCFunction_CAST(faulthandler_dump_traceback_later), METH_FASTCALL|METH_KEYWORDS, faulthandler_dump_traceback_later__doc__},

static PyObject *
faulthandler_dump_traceback_later_impl(PyObject *module,
                                       PyObject *timeout_obj, int repeat,
                                       PyObject *file, int exit);

static PyObject *
faulthandler_dump_traceback_later(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
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
        .ob_item = { &_Py_ID(timeout), &_Py_ID(repeat), &_Py_ID(file), &_Py_ID(exit), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"timeout", "repeat", "file", "exit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "dump_traceback_later",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *timeout_obj;
    int repeat = 0;
    PyObject *file = NULL;
    int exit = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    timeout_obj = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        repeat = PyObject_IsTrue(args[1]);
        if (repeat < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        file = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    exit = PyObject_IsTrue(args[3]);
    if (exit < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = faulthandler_dump_traceback_later_impl(module, timeout_obj, repeat, file, exit);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_cancel_dump_traceback_later_py__doc__,
"cancel_dump_traceback_later($module, /)\n"
"--\n"
"\n"
"Cancel the previous call to dump_traceback_later().");

#define FAULTHANDLER_CANCEL_DUMP_TRACEBACK_LATER_PY_METHODDEF    \
    {"cancel_dump_traceback_later", (PyCFunction)faulthandler_cancel_dump_traceback_later_py, METH_NOARGS, faulthandler_cancel_dump_traceback_later_py__doc__},

static PyObject *
faulthandler_cancel_dump_traceback_later_py_impl(PyObject *module);

static PyObject *
faulthandler_cancel_dump_traceback_later_py(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return faulthandler_cancel_dump_traceback_later_py_impl(module);
}

#if defined(FAULTHANDLER_USER)

PyDoc_STRVAR(faulthandler_register_py__doc__,
"register($module, /, signum, file=sys.stderr, all_threads=True,\n"
"         chain=False)\n"
"--\n"
"\n"
"Register a handler for the signal \'signum\'.\n"
"\n"
"Dump the traceback of the current thread, or of all threads if\n"
"all_threads is True, into file.");

#define FAULTHANDLER_REGISTER_PY_METHODDEF    \
    {"register", _PyCFunction_CAST(faulthandler_register_py), METH_FASTCALL|METH_KEYWORDS, faulthandler_register_py__doc__},

static PyObject *
faulthandler_register_py_impl(PyObject *module, int signum, PyObject *file,
                              int all_threads, int chain);

static PyObject *
faulthandler_register_py(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
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
        .ob_item = { &_Py_ID(signum), &_Py_ID(file), &_Py_ID(all_threads), &_Py_ID(chain), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"signum", "file", "all_threads", "chain", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "register",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    int signum;
    PyObject *file = NULL;
    int all_threads = 1;
    int chain = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    signum = PyLong_AsInt(args[0]);
    if (signum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        file = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        all_threads = PyObject_IsTrue(args[2]);
        if (all_threads < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    chain = PyObject_IsTrue(args[3]);
    if (chain < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = faulthandler_register_py_impl(module, signum, file, all_threads, chain);

exit:
    return return_value;
}

#endif /* defined(FAULTHANDLER_USER) */

#if defined(FAULTHANDLER_USER)

PyDoc_STRVAR(faulthandler_unregister_py__doc__,
"unregister($module, signum, /)\n"
"--\n"
"\n"
"Unregister the handler of the signal \'signum\' registered by register().");

#define FAULTHANDLER_UNREGISTER_PY_METHODDEF    \
    {"unregister", (PyCFunction)faulthandler_unregister_py, METH_O, faulthandler_unregister_py__doc__},

static PyObject *
faulthandler_unregister_py_impl(PyObject *module, int signum);

static PyObject *
faulthandler_unregister_py(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int signum;

    signum = PyLong_AsInt(arg);
    if (signum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = faulthandler_unregister_py_impl(module, signum);

exit:
    return return_value;
}

#endif /* defined(FAULTHANDLER_USER) */

PyDoc_STRVAR(faulthandler__sigsegv__doc__,
"_sigsegv($module, release_gil=False, /)\n"
"--\n"
"\n"
"Raise a SIGSEGV signal.");

#define FAULTHANDLER__SIGSEGV_METHODDEF    \
    {"_sigsegv", _PyCFunction_CAST(faulthandler__sigsegv), METH_FASTCALL, faulthandler__sigsegv__doc__},

static PyObject *
faulthandler__sigsegv_impl(PyObject *module, int release_gil);

static PyObject *
faulthandler__sigsegv(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int release_gil = 0;

    if (!_PyArg_CheckPositional("_sigsegv", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    release_gil = PyObject_IsTrue(args[0]);
    if (release_gil < 0) {
        goto exit;
    }
skip_optional:
    return_value = faulthandler__sigsegv_impl(module, release_gil);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler__fatal_error_c_thread__doc__,
"_fatal_error_c_thread($module, /)\n"
"--\n"
"\n"
"Call Py_FatalError() in a new C thread.");

#define FAULTHANDLER__FATAL_ERROR_C_THREAD_METHODDEF    \
    {"_fatal_error_c_thread", (PyCFunction)faulthandler__fatal_error_c_thread, METH_NOARGS, faulthandler__fatal_error_c_thread__doc__},

static PyObject *
faulthandler__fatal_error_c_thread_impl(PyObject *module);

static PyObject *
faulthandler__fatal_error_c_thread(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return faulthandler__fatal_error_c_thread_impl(module);
}

PyDoc_STRVAR(faulthandler__sigfpe__doc__,
"_sigfpe($module, /)\n"
"--\n"
"\n"
"Raise a SIGFPE signal.");

#define FAULTHANDLER__SIGFPE_METHODDEF    \
    {"_sigfpe", (PyCFunction)faulthandler__sigfpe, METH_NOARGS, faulthandler__sigfpe__doc__},

static PyObject *
faulthandler__sigfpe_impl(PyObject *module);

static PyObject *
faulthandler__sigfpe(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return faulthandler__sigfpe_impl(module);
}

PyDoc_STRVAR(faulthandler__sigabrt__doc__,
"_sigabrt($module, /)\n"
"--\n"
"\n"
"Raise a SIGABRT signal.");

#define FAULTHANDLER__SIGABRT_METHODDEF    \
    {"_sigabrt", (PyCFunction)faulthandler__sigabrt, METH_NOARGS, faulthandler__sigabrt__doc__},

static PyObject *
faulthandler__sigabrt_impl(PyObject *module);

static PyObject *
faulthandler__sigabrt(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return faulthandler__sigabrt_impl(module);
}

#if defined(FAULTHANDLER_USE_ALT_STACK)

PyDoc_STRVAR(faulthandler__stack_overflow__doc__,
"_stack_overflow($module, /)\n"
"--\n"
"\n"
"Recursive call to raise a stack overflow.");

#define FAULTHANDLER__STACK_OVERFLOW_METHODDEF    \
    {"_stack_overflow", (PyCFunction)faulthandler__stack_overflow, METH_NOARGS, faulthandler__stack_overflow__doc__},

static PyObject *
faulthandler__stack_overflow_impl(PyObject *module);

static PyObject *
faulthandler__stack_overflow(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return faulthandler__stack_overflow_impl(module);
}

#endif /* defined(FAULTHANDLER_USE_ALT_STACK) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(faulthandler__raise_exception__doc__,
"_raise_exception($module, code, flags=0, /)\n"
"--\n"
"\n"
"Call RaiseException(code, flags).");

#define FAULTHANDLER__RAISE_EXCEPTION_METHODDEF    \
    {"_raise_exception", _PyCFunction_CAST(faulthandler__raise_exception), METH_FASTCALL, faulthandler__raise_exception__doc__},

static PyObject *
faulthandler__raise_exception_impl(PyObject *module, unsigned int code,
                                   unsigned int flags);

static PyObject *
faulthandler__raise_exception(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned int code;
    unsigned int flags = 0;

    if (!_PyArg_CheckPositional("_raise_exception", nargs, 1, 2)) {
        goto exit;
    }
    if (!_PyLong_UnsignedInt_Converter(args[0], &code)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedInt_Converter(args[1], &flags)) {
        goto exit;
    }
skip_optional:
    return_value = faulthandler__raise_exception_impl(module, code, flags);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#ifndef FAULTHANDLER_REGISTER_PY_METHODDEF
    #define FAULTHANDLER_REGISTER_PY_METHODDEF
#endif /* !defined(FAULTHANDLER_REGISTER_PY_METHODDEF) */

#ifndef FAULTHANDLER_UNREGISTER_PY_METHODDEF
    #define FAULTHANDLER_UNREGISTER_PY_METHODDEF
#endif /* !defined(FAULTHANDLER_UNREGISTER_PY_METHODDEF) */

#ifndef FAULTHANDLER__STACK_OVERFLOW_METHODDEF
    #define FAULTHANDLER__STACK_OVERFLOW_METHODDEF
#endif /* !defined(FAULTHANDLER__STACK_OVERFLOW_METHODDEF) */

#ifndef FAULTHANDLER__RAISE_EXCEPTION_METHODDEF
    #define FAULTHANDLER__RAISE_EXCEPTION_METHODDEF
#endif /* !defined(FAULTHANDLER__RAISE_EXCEPTION_METHODDEF) */
/*[clinic end generated code: output=31bf0149d0d02ccf input=a9049054013a1b77]*/
