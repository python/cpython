/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(typevar_new__doc__,
"typevar(name, *constraints, bound=None, default=typing.NoDefault,\n"
"        covariant=False, contravariant=False, infer_variance=False)\n"
"--\n"
"\n"
"Create a TypeVar.");

static PyObject *
typevar_new_impl(PyTypeObject *type, PyObject *name, PyObject *constraints,
                 PyObject *bound, PyObject *default_value, int covariant,
                 int contravariant, int infer_variance);

static PyObject *
typevar_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(name), &_Py_ID(bound), &_Py_ID(default), &_Py_ID(covariant), &_Py_ID(contravariant), &_Py_ID(infer_variance), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "bound", "default", "covariant", "contravariant", "infer_variance", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "typevar",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = Py_MIN(nargs, 1) + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *name;
    PyObject *constraints = NULL;
    PyObject *bound = Py_None;
    PyObject *default_value = &_Py_NoDefaultStruct;
    int covariant = 0;
    int contravariant = 0;
    int infer_variance = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("typevar", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[1]) {
        bound = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[2]) {
        default_value = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[3]) {
        covariant = PyObject_IsTrue(fastargs[3]);
        if (covariant < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[4]) {
        contravariant = PyObject_IsTrue(fastargs[4]);
        if (contravariant < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    infer_variance = PyObject_IsTrue(fastargs[5]);
    if (infer_variance < 0) {
        goto exit;
    }
skip_optional_kwonly:
    constraints = PyTuple_GetSlice(args, 1, PY_SSIZE_T_MAX);
    if (!constraints) {
        goto exit;
    }
    return_value = typevar_new_impl(type, name, constraints, bound, default_value, covariant, contravariant, infer_variance);

exit:
    /* Cleanup for constraints */
    Py_XDECREF(constraints);

    return return_value;
}

PyDoc_STRVAR(typevar_typing_subst__doc__,
"__typing_subst__($self, arg, /)\n"
"--\n"
"\n");

#define TYPEVAR_TYPING_SUBST_METHODDEF    \
    {"__typing_subst__", (PyCFunction)typevar_typing_subst, METH_O, typevar_typing_subst__doc__},

static PyObject *
typevar_typing_subst_impl(typevarobject *self, PyObject *arg);

static PyObject *
typevar_typing_subst(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;

    return_value = typevar_typing_subst_impl((typevarobject *)self, arg);

    return return_value;
}

PyDoc_STRVAR(typevar_typing_prepare_subst__doc__,
"__typing_prepare_subst__($self, alias, args, /)\n"
"--\n"
"\n");

#define TYPEVAR_TYPING_PREPARE_SUBST_METHODDEF    \
    {"__typing_prepare_subst__", _PyCFunction_CAST(typevar_typing_prepare_subst), METH_FASTCALL, typevar_typing_prepare_subst__doc__},

static PyObject *
typevar_typing_prepare_subst_impl(typevarobject *self, PyObject *alias,
                                  PyObject *args);

static PyObject *
typevar_typing_prepare_subst(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *alias;
    PyObject *__clinic_args;

    if (!_PyArg_CheckPositional("__typing_prepare_subst__", nargs, 2, 2)) {
        goto exit;
    }
    alias = args[0];
    __clinic_args = args[1];
    return_value = typevar_typing_prepare_subst_impl((typevarobject *)self, alias, __clinic_args);

exit:
    return return_value;
}

PyDoc_STRVAR(typevar_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define TYPEVAR_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)typevar_reduce, METH_NOARGS, typevar_reduce__doc__},

static PyObject *
typevar_reduce_impl(typevarobject *self);

static PyObject *
typevar_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return typevar_reduce_impl((typevarobject *)self);
}

PyDoc_STRVAR(typevar_has_default__doc__,
"has_default($self, /)\n"
"--\n"
"\n");

#define TYPEVAR_HAS_DEFAULT_METHODDEF    \
    {"has_default", (PyCFunction)typevar_has_default, METH_NOARGS, typevar_has_default__doc__},

static PyObject *
typevar_has_default_impl(typevarobject *self);

static PyObject *
typevar_has_default(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return typevar_has_default_impl((typevarobject *)self);
}

PyDoc_STRVAR(paramspecargs_new__doc__,
"paramspecargs(origin)\n"
"--\n"
"\n"
"Create a ParamSpecArgs object.");

static PyObject *
paramspecargs_new_impl(PyTypeObject *type, PyObject *origin);

static PyObject *
paramspecargs_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(origin), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"origin", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "paramspecargs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *origin;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    origin = fastargs[0];
    return_value = paramspecargs_new_impl(type, origin);

exit:
    return return_value;
}

PyDoc_STRVAR(paramspeckwargs_new__doc__,
"paramspeckwargs(origin)\n"
"--\n"
"\n"
"Create a ParamSpecKwargs object.");

static PyObject *
paramspeckwargs_new_impl(PyTypeObject *type, PyObject *origin);

static PyObject *
paramspeckwargs_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(origin), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"origin", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "paramspeckwargs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *origin;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    origin = fastargs[0];
    return_value = paramspeckwargs_new_impl(type, origin);

exit:
    return return_value;
}

PyDoc_STRVAR(paramspec_new__doc__,
"paramspec(name, *, bound=None, default=typing.NoDefault,\n"
"          covariant=False, contravariant=False, infer_variance=False)\n"
"--\n"
"\n"
"Create a ParamSpec object.");

static PyObject *
paramspec_new_impl(PyTypeObject *type, PyObject *name, PyObject *bound,
                   PyObject *default_value, int covariant, int contravariant,
                   int infer_variance);

static PyObject *
paramspec_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(name), &_Py_ID(bound), &_Py_ID(default), &_Py_ID(covariant), &_Py_ID(contravariant), &_Py_ID(infer_variance), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "bound", "default", "covariant", "contravariant", "infer_variance", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "paramspec",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *name;
    PyObject *bound = Py_None;
    PyObject *default_value = &_Py_NoDefaultStruct;
    int covariant = 0;
    int contravariant = 0;
    int infer_variance = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("paramspec", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[1]) {
        bound = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[2]) {
        default_value = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[3]) {
        covariant = PyObject_IsTrue(fastargs[3]);
        if (covariant < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[4]) {
        contravariant = PyObject_IsTrue(fastargs[4]);
        if (contravariant < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    infer_variance = PyObject_IsTrue(fastargs[5]);
    if (infer_variance < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = paramspec_new_impl(type, name, bound, default_value, covariant, contravariant, infer_variance);

exit:
    return return_value;
}

PyDoc_STRVAR(paramspec_typing_subst__doc__,
"__typing_subst__($self, arg, /)\n"
"--\n"
"\n");

#define PARAMSPEC_TYPING_SUBST_METHODDEF    \
    {"__typing_subst__", (PyCFunction)paramspec_typing_subst, METH_O, paramspec_typing_subst__doc__},

static PyObject *
paramspec_typing_subst_impl(paramspecobject *self, PyObject *arg);

static PyObject *
paramspec_typing_subst(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;

    return_value = paramspec_typing_subst_impl((paramspecobject *)self, arg);

    return return_value;
}

PyDoc_STRVAR(paramspec_typing_prepare_subst__doc__,
"__typing_prepare_subst__($self, alias, args, /)\n"
"--\n"
"\n");

#define PARAMSPEC_TYPING_PREPARE_SUBST_METHODDEF    \
    {"__typing_prepare_subst__", _PyCFunction_CAST(paramspec_typing_prepare_subst), METH_FASTCALL, paramspec_typing_prepare_subst__doc__},

static PyObject *
paramspec_typing_prepare_subst_impl(paramspecobject *self, PyObject *alias,
                                    PyObject *args);

static PyObject *
paramspec_typing_prepare_subst(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *alias;
    PyObject *__clinic_args;

    if (!_PyArg_CheckPositional("__typing_prepare_subst__", nargs, 2, 2)) {
        goto exit;
    }
    alias = args[0];
    __clinic_args = args[1];
    return_value = paramspec_typing_prepare_subst_impl((paramspecobject *)self, alias, __clinic_args);

exit:
    return return_value;
}

PyDoc_STRVAR(paramspec_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define PARAMSPEC_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)paramspec_reduce, METH_NOARGS, paramspec_reduce__doc__},

static PyObject *
paramspec_reduce_impl(paramspecobject *self);

static PyObject *
paramspec_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return paramspec_reduce_impl((paramspecobject *)self);
}

PyDoc_STRVAR(paramspec_has_default__doc__,
"has_default($self, /)\n"
"--\n"
"\n");

#define PARAMSPEC_HAS_DEFAULT_METHODDEF    \
    {"has_default", (PyCFunction)paramspec_has_default, METH_NOARGS, paramspec_has_default__doc__},

static PyObject *
paramspec_has_default_impl(paramspecobject *self);

static PyObject *
paramspec_has_default(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return paramspec_has_default_impl((paramspecobject *)self);
}

PyDoc_STRVAR(typevartuple__doc__,
"typevartuple(name, *, default=typing.NoDefault)\n"
"--\n"
"\n"
"Create a new TypeVarTuple with the given name.");

static PyObject *
typevartuple_impl(PyTypeObject *type, PyObject *name,
                  PyObject *default_value);

static PyObject *
typevartuple(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(name), &_Py_ID(default), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "typevartuple",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *name;
    PyObject *default_value = &_Py_NoDefaultStruct;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("typevartuple", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    default_value = fastargs[1];
skip_optional_kwonly:
    return_value = typevartuple_impl(type, name, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(typevartuple_typing_subst__doc__,
"__typing_subst__($self, arg, /)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_TYPING_SUBST_METHODDEF    \
    {"__typing_subst__", (PyCFunction)typevartuple_typing_subst, METH_O, typevartuple_typing_subst__doc__},

static PyObject *
typevartuple_typing_subst_impl(typevartupleobject *self, PyObject *arg);

static PyObject *
typevartuple_typing_subst(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;

    return_value = typevartuple_typing_subst_impl((typevartupleobject *)self, arg);

    return return_value;
}

PyDoc_STRVAR(typevartuple_typing_prepare_subst__doc__,
"__typing_prepare_subst__($self, alias, args, /)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_TYPING_PREPARE_SUBST_METHODDEF    \
    {"__typing_prepare_subst__", _PyCFunction_CAST(typevartuple_typing_prepare_subst), METH_FASTCALL, typevartuple_typing_prepare_subst__doc__},

static PyObject *
typevartuple_typing_prepare_subst_impl(typevartupleobject *self,
                                       PyObject *alias, PyObject *args);

static PyObject *
typevartuple_typing_prepare_subst(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *alias;
    PyObject *__clinic_args;

    if (!_PyArg_CheckPositional("__typing_prepare_subst__", nargs, 2, 2)) {
        goto exit;
    }
    alias = args[0];
    __clinic_args = args[1];
    return_value = typevartuple_typing_prepare_subst_impl((typevartupleobject *)self, alias, __clinic_args);

exit:
    return return_value;
}

PyDoc_STRVAR(typevartuple_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)typevartuple_reduce, METH_NOARGS, typevartuple_reduce__doc__},

static PyObject *
typevartuple_reduce_impl(typevartupleobject *self);

static PyObject *
typevartuple_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return typevartuple_reduce_impl((typevartupleobject *)self);
}

PyDoc_STRVAR(typevartuple_has_default__doc__,
"has_default($self, /)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_HAS_DEFAULT_METHODDEF    \
    {"has_default", (PyCFunction)typevartuple_has_default, METH_NOARGS, typevartuple_has_default__doc__},

static PyObject *
typevartuple_has_default_impl(typevartupleobject *self);

static PyObject *
typevartuple_has_default(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return typevartuple_has_default_impl((typevartupleobject *)self);
}

PyDoc_STRVAR(typealias_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define TYPEALIAS_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)typealias_reduce, METH_NOARGS, typealias_reduce__doc__},

static PyObject *
typealias_reduce_impl(typealiasobject *self);

static PyObject *
typealias_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return typealias_reduce_impl((typealiasobject *)self);
}

PyDoc_STRVAR(typealias_new__doc__,
"typealias(name, value, *, type_params=<unrepresentable>, qualname=None)\n"
"--\n"
"\n"
"Create a TypeAliasType.");

static PyObject *
typealias_new_impl(PyTypeObject *type, PyObject *name, PyObject *value,
                   PyObject *type_params, PyObject *qualname);

static PyObject *
typealias_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(name), &_Py_ID(value), &_Py_ID(type_params), &_Py_ID(qualname), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "value", "type_params", "qualname", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "typealias",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 2;
    PyObject *name;
    PyObject *value;
    PyObject *type_params = NULL;
    PyObject *qualname = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("typealias", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    value = fastargs[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[2]) {
        type_params = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    qualname = fastargs[3];
skip_optional_kwonly:
    return_value = typealias_new_impl(type, name, value, type_params, qualname);

exit:
    return return_value;
}
/*[clinic end generated code: output=67ab9a5d1869f2c9 input=a9049054013a1b77]*/
