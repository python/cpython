/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

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
    opcode = PyLong_AsInt(args[0]);
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
    opcode = PyLong_AsInt(args[0]);
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
    opcode = PyLong_AsInt(args[0]);
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
    opcode = PyLong_AsInt(args[0]);
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
    opcode = PyLong_AsInt(args[0]);
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
    opcode = PyLong_AsInt(args[0]);
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

PyDoc_STRVAR(_opcode_has_free__doc__,
"has_free($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode accesses a free variable, False otherwise.\n"
"\n"
"Note that \'free\' in this context refers to names in the current scope\n"
"that are referenced by inner scopes or names in outer scopes that are\n"
"referenced from this scope. It does not include references to global\n"
"or builtin scopes.");

#define _OPCODE_HAS_FREE_METHODDEF    \
    {"has_free", _PyCFunction_CAST(_opcode_has_free), METH_FASTCALL|METH_KEYWORDS, _opcode_has_free__doc__},

static int
_opcode_has_free_impl(PyObject *module, int opcode);

static PyObject *
_opcode_has_free(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "has_free",
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
    opcode = PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_free_impl(module, opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_local__doc__,
"has_local($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode accesses a local variable, False otherwise.");

#define _OPCODE_HAS_LOCAL_METHODDEF    \
    {"has_local", _PyCFunction_CAST(_opcode_has_local), METH_FASTCALL|METH_KEYWORDS, _opcode_has_local__doc__},

static int
_opcode_has_local_impl(PyObject *module, int opcode);

static PyObject *
_opcode_has_local(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "has_local",
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
    opcode = PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_local_impl(module, opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_exc__doc__,
"has_exc($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode sets an exception handler, False otherwise.");

#define _OPCODE_HAS_EXC_METHODDEF    \
    {"has_exc", _PyCFunction_CAST(_opcode_has_exc), METH_FASTCALL|METH_KEYWORDS, _opcode_has_exc__doc__},

static int
_opcode_has_exc_impl(PyObject *module, int opcode);

static PyObject *
_opcode_has_exc(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "has_exc",
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
    opcode = PyLong_AsInt(args[0]);
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_exc_impl(module, opcode);
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

PyDoc_STRVAR(_opcode_get_nb_ops__doc__,
"get_nb_ops($module, /)\n"
"--\n"
"\n"
"Return array of symbols of binary ops.\n"
"\n"
"Indexed by the BINARY_OP oparg value.");

#define _OPCODE_GET_NB_OPS_METHODDEF    \
    {"get_nb_ops", (PyCFunction)_opcode_get_nb_ops, METH_NOARGS, _opcode_get_nb_ops__doc__},

static PyObject *
_opcode_get_nb_ops_impl(PyObject *module);

static PyObject *
_opcode_get_nb_ops(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _opcode_get_nb_ops_impl(module);
}

PyDoc_STRVAR(_opcode_get_intrinsic1_descs__doc__,
"get_intrinsic1_descs($module, /)\n"
"--\n"
"\n"
"Return a list of names of the unary intrinsics.");

#define _OPCODE_GET_INTRINSIC1_DESCS_METHODDEF    \
    {"get_intrinsic1_descs", (PyCFunction)_opcode_get_intrinsic1_descs, METH_NOARGS, _opcode_get_intrinsic1_descs__doc__},

static PyObject *
_opcode_get_intrinsic1_descs_impl(PyObject *module);

static PyObject *
_opcode_get_intrinsic1_descs(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _opcode_get_intrinsic1_descs_impl(module);
}

PyDoc_STRVAR(_opcode_get_intrinsic2_descs__doc__,
"get_intrinsic2_descs($module, /)\n"
"--\n"
"\n"
"Return a list of names of the binary intrinsics.");

#define _OPCODE_GET_INTRINSIC2_DESCS_METHODDEF    \
    {"get_intrinsic2_descs", (PyCFunction)_opcode_get_intrinsic2_descs, METH_NOARGS, _opcode_get_intrinsic2_descs__doc__},

static PyObject *
_opcode_get_intrinsic2_descs_impl(PyObject *module);

static PyObject *
_opcode_get_intrinsic2_descs(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _opcode_get_intrinsic2_descs_impl(module);
}
/*[clinic end generated code: output=a1052bb1deffb7f2 input=a9049054013a1b77]*/
