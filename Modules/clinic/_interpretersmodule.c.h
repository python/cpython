/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_interpreters_create__doc__,
"create($module, /, config=\'isolated\', *, reqrefs=False)\n"
"--\n"
"\n"
"Create a new interpreter and return a unique generated ID.\n"
"\n"
"The caller is responsible for destroying the interpreter before exiting,\n"
"typically by using _interpreters.destroy().  This can be managed\n"
"automatically by passing \"reqrefs=True\" and then using _incref() and\n"
"_decref() appropriately.\n"
"\n"
"\"config\" must be a valid interpreter config or the name of a\n"
"predefined config (\'isolated\' or \'legacy\').  The default\n"
"is \'isolated\'.");

#define _INTERPRETERS_CREATE_METHODDEF    \
    {"create", _PyCFunction_CAST(_interpreters_create), METH_FASTCALL|METH_KEYWORDS, _interpreters_create__doc__},

static PyObject *
_interpreters_create_impl(PyObject *module, PyObject *configobj, int reqrefs);

static PyObject *
_interpreters_create(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(config), &_Py_ID(reqrefs), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"config", "reqrefs", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *configobj = NULL;
    int reqrefs = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        configobj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    reqrefs = PyObject_IsTrue(args[1]);
    if (reqrefs < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_create_impl(module, configobj, reqrefs);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_destroy__doc__,
"destroy($module, /, id, *, restrict=False)\n"
"--\n"
"\n"
"Destroy the identified interpreter.\n"
"\n"
"Attempting to destroy the current interpreter raises InterpreterError.\n"
"So does an unrecognized ID.");

#define _INTERPRETERS_DESTROY_METHODDEF    \
    {"destroy", _PyCFunction_CAST(_interpreters_destroy), METH_FASTCALL|METH_KEYWORDS, _interpreters_destroy__doc__},

static PyObject *
_interpreters_destroy_impl(PyObject *module, PyObject *id, int restricted);

static PyObject *
_interpreters_destroy(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "destroy",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *id;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    restricted = PyObject_IsTrue(args[1]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_destroy_impl(module, id, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_list_all__doc__,
"list_all($module, /, *, require_ready=False)\n"
"--\n"
"\n"
"Return a list containing the ID of every existing interpreter.");

#define _INTERPRETERS_LIST_ALL_METHODDEF    \
    {"list_all", _PyCFunction_CAST(_interpreters_list_all), METH_FASTCALL|METH_KEYWORDS, _interpreters_list_all__doc__},

static PyObject *
_interpreters_list_all_impl(PyObject *module, int reqready);

static PyObject *
_interpreters_list_all(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(require_ready), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"require_ready", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "list_all",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int reqready = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    reqready = PyObject_IsTrue(args[0]);
    if (reqready < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_list_all_impl(module, reqready);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_get_current__doc__,
"get_current($module, /)\n"
"--\n"
"\n"
"Return (ID, whence) of the current interpreter.");

#define _INTERPRETERS_GET_CURRENT_METHODDEF    \
    {"get_current", (PyCFunction)_interpreters_get_current, METH_NOARGS, _interpreters_get_current__doc__},

static PyObject *
_interpreters_get_current_impl(PyObject *module);

static PyObject *
_interpreters_get_current(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _interpreters_get_current_impl(module);
}

PyDoc_STRVAR(_interpreters_get_main__doc__,
"get_main($module, /)\n"
"--\n"
"\n"
"Return (ID, whence) of the main interpreter.");

#define _INTERPRETERS_GET_MAIN_METHODDEF    \
    {"get_main", (PyCFunction)_interpreters_get_main, METH_NOARGS, _interpreters_get_main__doc__},

static PyObject *
_interpreters_get_main_impl(PyObject *module);

static PyObject *
_interpreters_get_main(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _interpreters_get_main_impl(module);
}

PyDoc_STRVAR(_interpreters_set___main___attrs__doc__,
"set___main___attrs($module, /, id, updates, *, restrict=False)\n"
"--\n"
"\n"
"Bind the given attributes in the interpreter\'s __main__ module.");

#define _INTERPRETERS_SET___MAIN___ATTRS_METHODDEF    \
    {"set___main___attrs", _PyCFunction_CAST(_interpreters_set___main___attrs), METH_FASTCALL|METH_KEYWORDS, _interpreters_set___main___attrs__doc__},

static PyObject *
_interpreters_set___main___attrs_impl(PyObject *module, PyObject *id,
                                      PyObject *updates, int restricted);

static PyObject *
_interpreters_set___main___attrs(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(updates), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "updates", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set___main___attrs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *id;
    PyObject *updates;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    if (!PyDict_Check(args[1])) {
        _PyArg_BadArgument("set___main___attrs", "argument 'updates'", "dict", args[1]);
        goto exit;
    }
    updates = args[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    restricted = PyObject_IsTrue(args[2]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_set___main___attrs_impl(module, id, updates, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_exec__doc__,
"exec($module, /, id, code, shared={}, *, restrict=False)\n"
"--\n"
"\n"
"Execute the provided code in the identified interpreter.\n"
"\n"
"This is equivalent to running the builtin exec() under the target\n"
"interpreter, using the __dict__ of its __main__ module as both\n"
"globals and locals.\n"
"\n"
"\"code\" may be a string containing the text of a Python script.\n"
"\n"
"Functions (and code objects) are also supported, with some restrictions.\n"
"The code/function must not take any arguments or be a closure\n"
"(i.e. have cell vars).  Methods and other callables are not supported.\n"
"\n"
"If a function is provided, its code object is used and all its state\n"
"is ignored, including its __globals__ dict.");

#define _INTERPRETERS_EXEC_METHODDEF    \
    {"exec", _PyCFunction_CAST(_interpreters_exec), METH_FASTCALL|METH_KEYWORDS, _interpreters_exec__doc__},

static PyObject *
_interpreters_exec_impl(PyObject *module, PyObject *id, PyObject *code,
                        PyObject *shared, int restricted);

static PyObject *
_interpreters_exec(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(code), &_Py_ID(shared), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "code", "shared", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "exec",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *id;
    PyObject *code;
    PyObject *shared = NULL;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    code = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        if (!PyDict_Check(args[2])) {
            _PyArg_BadArgument("exec", "argument 'shared'", "dict", args[2]);
            goto exit;
        }
        shared = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    restricted = PyObject_IsTrue(args[3]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_exec_impl(module, id, code, shared, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_run_string__doc__,
"run_string($module, /, id, script, shared={}, *, restrict=False)\n"
"--\n"
"\n"
"Execute the provided string in the identified interpreter.\n"
"\n"
"(See _interpreters.exec().)");

#define _INTERPRETERS_RUN_STRING_METHODDEF    \
    {"run_string", _PyCFunction_CAST(_interpreters_run_string), METH_FASTCALL|METH_KEYWORDS, _interpreters_run_string__doc__},

static PyObject *
_interpreters_run_string_impl(PyObject *module, PyObject *id,
                              PyObject *script, PyObject *shared,
                              int restricted);

static PyObject *
_interpreters_run_string(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(script), &_Py_ID(shared), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "script", "shared", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "run_string",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *id;
    PyObject *script;
    PyObject *shared = NULL;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("run_string", "argument 'script'", "str", args[1]);
        goto exit;
    }
    script = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        if (!PyDict_Check(args[2])) {
            _PyArg_BadArgument("run_string", "argument 'shared'", "dict", args[2]);
            goto exit;
        }
        shared = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    restricted = PyObject_IsTrue(args[3]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_run_string_impl(module, id, script, shared, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_run_func__doc__,
"run_func($module, /, id, func, shared={}, *, restrict=False)\n"
"--\n"
"\n"
"Execute the body of the provided function in the identified interpreter.\n"
"\n"
"Code objects are also supported.  In both cases, closures and args\n"
"are not supported.  Methods and other callables are not supported either.\n"
"\n"
"(See _interpreters.exec().)");

#define _INTERPRETERS_RUN_FUNC_METHODDEF    \
    {"run_func", _PyCFunction_CAST(_interpreters_run_func), METH_FASTCALL|METH_KEYWORDS, _interpreters_run_func__doc__},

static PyObject *
_interpreters_run_func_impl(PyObject *module, PyObject *id, PyObject *func,
                            PyObject *shared, int restricted);

static PyObject *
_interpreters_run_func(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(func), &_Py_ID(shared), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "func", "shared", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "run_func",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *id;
    PyObject *func;
    PyObject *shared = NULL;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    func = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        if (!PyDict_Check(args[2])) {
            _PyArg_BadArgument("run_func", "argument 'shared'", "dict", args[2]);
            goto exit;
        }
        shared = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    restricted = PyObject_IsTrue(args[3]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_run_func_impl(module, id, func, shared, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_call__doc__,
"call($module, /, id, callable, args=(), kwargs={}, *,\n"
"     preserve_exc=False, restrict=False)\n"
"--\n"
"\n"
"Call the provided object in the identified interpreter.\n"
"\n"
"Pass the given args and kwargs, if possible.");

#define _INTERPRETERS_CALL_METHODDEF    \
    {"call", _PyCFunction_CAST(_interpreters_call), METH_FASTCALL|METH_KEYWORDS, _interpreters_call__doc__},

static PyObject *
_interpreters_call_impl(PyObject *module, PyObject *id, PyObject *callable,
                        PyObject *args, PyObject *kwargs, int preserve_exc,
                        int restricted);

static PyObject *
_interpreters_call(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(id), &_Py_ID(callable), &_Py_ID(args), &_Py_ID(kwargs), &_Py_ID(preserve_exc), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "callable", "args", "kwargs", "preserve_exc", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "call",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *id;
    PyObject *callable;
    PyObject *__clinic_args = NULL;
    PyObject *__clinic_kwargs = NULL;
    int preserve_exc = 0;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    callable = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        if (!PyTuple_Check(args[2])) {
            _PyArg_BadArgument("call", "argument 'args'", "tuple", args[2]);
            goto exit;
        }
        __clinic_args = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        if (!PyDict_Check(args[3])) {
            _PyArg_BadArgument("call", "argument 'kwargs'", "dict", args[3]);
            goto exit;
        }
        __clinic_kwargs = args[3];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[4]) {
        preserve_exc = PyObject_IsTrue(args[4]);
        if (preserve_exc < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    restricted = PyObject_IsTrue(args[5]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_call_impl(module, id, callable, __clinic_args, __clinic_kwargs, preserve_exc, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_is_shareable__doc__,
"is_shareable($module, /, obj)\n"
"--\n"
"\n"
"Return True if the object\'s data may be shared between interpreters and False otherwise.");

#define _INTERPRETERS_IS_SHAREABLE_METHODDEF    \
    {"is_shareable", _PyCFunction_CAST(_interpreters_is_shareable), METH_FASTCALL|METH_KEYWORDS, _interpreters_is_shareable__doc__},

static PyObject *
_interpreters_is_shareable_impl(PyObject *module, PyObject *obj);

static PyObject *
_interpreters_is_shareable(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(obj), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"obj", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_shareable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *obj;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    obj = args[0];
    return_value = _interpreters_is_shareable_impl(module, obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_is_running__doc__,
"is_running($module, /, id, *, restrict=False)\n"
"--\n"
"\n"
"Return whether or not the identified interpreter is running.");

#define _INTERPRETERS_IS_RUNNING_METHODDEF    \
    {"is_running", _PyCFunction_CAST(_interpreters_is_running), METH_FASTCALL|METH_KEYWORDS, _interpreters_is_running__doc__},

static PyObject *
_interpreters_is_running_impl(PyObject *module, PyObject *id, int restricted);

static PyObject *
_interpreters_is_running(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_running",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *id;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    restricted = PyObject_IsTrue(args[1]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_is_running_impl(module, id, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_get_config__doc__,
"get_config($module, /, id, *, restrict=False)\n"
"--\n"
"\n"
"Return a representation of the config used to initialize the interpreter.");

#define _INTERPRETERS_GET_CONFIG_METHODDEF    \
    {"get_config", _PyCFunction_CAST(_interpreters_get_config), METH_FASTCALL|METH_KEYWORDS, _interpreters_get_config__doc__},

static PyObject *
_interpreters_get_config_impl(PyObject *module, PyObject *id, int restricted);

static PyObject *
_interpreters_get_config(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_config",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *id;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    restricted = PyObject_IsTrue(args[1]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_get_config_impl(module, id, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_whence__doc__,
"whence($module, /, id)\n"
"--\n"
"\n"
"Return an identifier for where the interpreter was created.");

#define _INTERPRETERS_WHENCE_METHODDEF    \
    {"whence", _PyCFunction_CAST(_interpreters_whence), METH_FASTCALL|METH_KEYWORDS, _interpreters_whence__doc__},

static PyObject *
_interpreters_whence_impl(PyObject *module, PyObject *id);

static PyObject *
_interpreters_whence(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "whence",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *id;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    return_value = _interpreters_whence_impl(module, id);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_incref__doc__,
"incref($module, /, id, *, implieslink=False, restrict=False)\n"
"--\n"
"\n");

#define _INTERPRETERS_INCREF_METHODDEF    \
    {"incref", _PyCFunction_CAST(_interpreters_incref), METH_FASTCALL|METH_KEYWORDS, _interpreters_incref__doc__},

static PyObject *
_interpreters_incref_impl(PyObject *module, PyObject *id, int implieslink,
                          int restricted);

static PyObject *
_interpreters_incref(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(implieslink), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "implieslink", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "incref",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *id;
    int implieslink = 0;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        implieslink = PyObject_IsTrue(args[1]);
        if (implieslink < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    restricted = PyObject_IsTrue(args[2]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_incref_impl(module, id, implieslink, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_decref__doc__,
"decref($module, /, id, *, restrict=False)\n"
"--\n"
"\n");

#define _INTERPRETERS_DECREF_METHODDEF    \
    {"decref", _PyCFunction_CAST(_interpreters_decref), METH_FASTCALL|METH_KEYWORDS, _interpreters_decref__doc__},

static PyObject *
_interpreters_decref_impl(PyObject *module, PyObject *id, int restricted);

static PyObject *
_interpreters_decref(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decref",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *id;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    restricted = PyObject_IsTrue(args[1]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_decref_impl(module, id, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_capture_exception__doc__,
"capture_exception($module, /, exc=None)\n"
"--\n"
"\n"
"Return a snapshot of an exception.\n"
"\n"
"If \"exc\" is None then the current exception, if any, is used (but not cleared).\n"
"The returned snapshot is the same as what _interpreters.exec() returns.");

#define _INTERPRETERS_CAPTURE_EXCEPTION_METHODDEF    \
    {"capture_exception", _PyCFunction_CAST(_interpreters_capture_exception), METH_FASTCALL|METH_KEYWORDS, _interpreters_capture_exception__doc__},

static PyObject *
_interpreters_capture_exception_impl(PyObject *module, PyObject *exc_arg);

static PyObject *
_interpreters_capture_exception(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(exc), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"exc", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "capture_exception",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *exc_arg = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    exc_arg = args[0];
skip_optional_pos:
    return_value = _interpreters_capture_exception_impl(module, exc_arg);

exit:
    return return_value;
}
/*[clinic end generated code: output=c80f73761f860f6c input=a9049054013a1b77]*/
