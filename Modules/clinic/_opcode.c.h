/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(_opcode_stack_effect__doc__,
"stack_effect($module, opcode, oparg=None, /, *, jump=None)\n"
"--\n"
"\n"
"Compute the stack effect of the opcode.");

#define _OPCODE_STACK_EFFECT_METHODDEF    \
    {"stack_effect", _PyCFunction_CAST(_opcode_stack_effect), METH_FASTCALL|METH_KEYWORDS, _opcode_stack_effect__doc__},

static int
_opcode_stack_effect_impl(PyObject *module, int opcode, PyObject *oparg,
                          PyObject *jump);

static PyObject *
_opcode_stack_effect(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(jump), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "jump", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "stack_effect",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    int opcode;
    PyObject *oparg = Py_None;
    PyObject *jump = Py_None;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    opcode = _PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    oparg = args[1];
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    jump = args[2];
skip_optional_kwonly:
    _return_value = _opcode_stack_effect_impl(module, opcode, oparg, jump);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_is_valid__doc__,
"is_valid($module, /, opcode)\n"
"--\n"
"\n"
"Return True if opcode is valid, False otherwise.");

#define _OPCODE_IS_VALID_METHODDEF    \
    {"is_valid", _PyCFunction_CAST(_opcode_is_valid), METH_FASTCALL|METH_KEYWORDS, _opcode_is_valid__doc__},

static int
_opcode_is_valid_impl(PyObject *module, int opcode);

static PyObject *
_opcode_is_valid(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(opcode), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_valid",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int opcode;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    opcode = _PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_is_valid_impl(module, opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_arg__doc__,
"has_arg($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode uses its oparg, False otherwise.");

#define _OPCODE_HAS_ARG_METHODDEF    \
    {"has_arg", _PyCFunction_CAST(_opcode_has_arg), METH_FASTCALL|METH_KEYWORDS, _opcode_has_arg__doc__},

static int
_opcode_has_arg_impl(PyObject *module, int opcode);

static PyObject *
_opcode_has_arg(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(opcode), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_arg",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int opcode;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    opcode = _PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_arg_impl(module, opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_const__doc__,
"has_const($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode accesses a constant, False otherwise.");

#define _OPCODE_HAS_CONST_METHODDEF    \
    {"has_const", _PyCFunction_CAST(_opcode_has_const), METH_FASTCALL|METH_KEYWORDS, _opcode_has_const__doc__},

static int
_opcode_has_const_impl(PyObject *module, int opcode);

static PyObject *
_opcode_has_const(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(opcode), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_const",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int opcode;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    opcode = _PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_const_impl(module, opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_name__doc__,
"has_name($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode accesses an attribute by name, False otherwise.");

#define _OPCODE_HAS_NAME_METHODDEF    \
    {"has_name", _PyCFunction_CAST(_opcode_has_name), METH_FASTCALL|METH_KEYWORDS, _opcode_has_name__doc__},

static int
_opcode_has_name_impl(PyObject *module, int opcode);

static PyObject *
_opcode_has_name(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(opcode), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_name",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int opcode;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    opcode = _PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_name_impl(module, opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_jump__doc__,
"has_jump($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode has a jump target, False otherwise.");

#define _OPCODE_HAS_JUMP_METHODDEF    \
    {"has_jump", _PyCFunction_CAST(_opcode_has_jump), METH_FASTCALL|METH_KEYWORDS, _opcode_has_jump__doc__},

static int
_opcode_has_jump_impl(PyObject *module, int opcode);

static PyObject *
_opcode_has_jump(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(opcode), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_jump",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int opcode;
    int _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    opcode = _PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_jump_impl(module, opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_get_specialization_stats__doc__,
"get_specialization_stats($module, /)\n"
"--\n"
"\n"
"Return the specialization stats");

#define _OPCODE_GET_SPECIALIZATION_STATS_METHODDEF    \
    {"get_specialization_stats", (PyCFunction)_opcode_get_specialization_stats, METH_NOARGS, _opcode_get_specialization_stats__doc__},

static PyObject *
_opcode_get_specialization_stats_impl(PyObject *module);

static PyObject *
_opcode_get_specialization_stats(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _opcode_get_specialization_stats_impl(module);
}
/*[clinic end generated code: output=ae2b2ef56d582180 input=a9049054013a1b77]*/
