/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(typevar_new__doc__,
"typevar(name, *constraints, *, bound=None, covariant=False,\n"
"        contravariant=False, infer_variance=False)\n"
"--\n"
"\n"
"Create a TypeVar.");

static PyObject *
typevar_new_impl(PyTypeObject *type, PyObject *name, PyObject *constraints,
                 PyObject *bound, int covariant, int contravariant,
                 int infer_variance);

static PyObject *
typevar_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(name), &_Py_ID(bound), &_Py_ID(covariant), &_Py_ID(contravariant), &_Py_ID(infer_variance), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "bound", "covariant", "contravariant", "infer_variance", NULL};
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
    int covariant = 0;
    int contravariant = 0;
    int infer_variance = 0;

    fastargs = _PyArg_UnpackKeywordsWithVararg(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("typevar", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    constraints = fastargs[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[2]) {
        bound = fastargs[2];
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
    return_value = typevar_new_impl(type, name, constraints, bound, covariant, contravariant, infer_variance);

exit:
    Py_XDECREF(constraints);
    return return_value;
}

PyDoc_STRVAR(typevar_typing_subst__doc__,
"__typing_subst__($self, /, arg)\n"
"--\n"
"\n");

#define TYPEVAR_TYPING_SUBST_METHODDEF    \
    {"__typing_subst__", _PyCFunction_CAST(typevar_typing_subst), METH_FASTCALL|METH_KEYWORDS, typevar_typing_subst__doc__},

static PyObject *
typevar_typing_subst_impl(typevarobject *self, PyObject *arg);

static PyObject *
typevar_typing_subst(typevarobject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(arg), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"arg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__typing_subst__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *arg;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    arg = args[0];
    return_value = typevar_typing_subst_impl(self, arg);

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
typevar_reduce(typevarobject *self, PyObject *Py_UNUSED(ignored))
{
    return typevar_reduce_impl(self);
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
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
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
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    origin = fastargs[0];
    return_value = paramspeckwargs_new_impl(type, origin);

exit:
    return return_value;
}

PyDoc_STRVAR(paramspec_new__doc__,
"paramspec(name, *, bound=None, covariant=False, contravariant=False,\n"
"          infer_variance=False)\n"
"--\n"
"\n"
"Create a ParamSpec object.");

static PyObject *
paramspec_new_impl(PyTypeObject *type, PyObject *name, PyObject *bound,
                   int covariant, int contravariant, int infer_variance);

static PyObject *
paramspec_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(name), &_Py_ID(bound), &_Py_ID(covariant), &_Py_ID(contravariant), &_Py_ID(infer_variance), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "bound", "covariant", "contravariant", "infer_variance", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "paramspec",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *name;
    PyObject *bound = Py_None;
    int covariant = 0;
    int contravariant = 0;
    int infer_variance = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
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
        covariant = PyObject_IsTrue(fastargs[2]);
        if (covariant < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[3]) {
        contravariant = PyObject_IsTrue(fastargs[3]);
        if (contravariant < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    infer_variance = PyObject_IsTrue(fastargs[4]);
    if (infer_variance < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = paramspec_new_impl(type, name, bound, covariant, contravariant, infer_variance);

exit:
    return return_value;
}

PyDoc_STRVAR(paramspec_typing_subst__doc__,
"__typing_subst__($self, /, arg)\n"
"--\n"
"\n");

#define PARAMSPEC_TYPING_SUBST_METHODDEF    \
    {"__typing_subst__", _PyCFunction_CAST(paramspec_typing_subst), METH_FASTCALL|METH_KEYWORDS, paramspec_typing_subst__doc__},

static PyObject *
paramspec_typing_subst_impl(paramspecobject *self, PyObject *arg);

static PyObject *
paramspec_typing_subst(paramspecobject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(arg), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"arg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__typing_subst__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *arg;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    arg = args[0];
    return_value = paramspec_typing_subst_impl(self, arg);

exit:
    return return_value;
}

PyDoc_STRVAR(paramspec_typing_prepare_subst__doc__,
"__typing_prepare_subst__($self, /, alias, args)\n"
"--\n"
"\n");

#define PARAMSPEC_TYPING_PREPARE_SUBST_METHODDEF    \
    {"__typing_prepare_subst__", _PyCFunction_CAST(paramspec_typing_prepare_subst), METH_FASTCALL|METH_KEYWORDS, paramspec_typing_prepare_subst__doc__},

static PyObject *
paramspec_typing_prepare_subst_impl(paramspecobject *self, PyObject *alias,
                                    PyObject *args);

static PyObject *
paramspec_typing_prepare_subst(paramspecobject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(alias), &_Py_ID(args), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"alias", "args", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__typing_prepare_subst__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *alias;
    PyObject *__clinic_args;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    alias = args[0];
    __clinic_args = args[1];
    return_value = paramspec_typing_prepare_subst_impl(self, alias, __clinic_args);

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
paramspec_reduce(paramspecobject *self, PyObject *Py_UNUSED(ignored))
{
    return paramspec_reduce_impl(self);
}

PyDoc_STRVAR(typevartuple__doc__,
"typevartuple(name)\n"
"--\n"
"\n"
"Create a new TypeVarTuple with the given name.");

static PyObject *
typevartuple_impl(PyTypeObject *type, PyObject *name);

static PyObject *
typevartuple(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "typevartuple",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *name;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("typevartuple", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    return_value = typevartuple_impl(type, name);

exit:
    return return_value;
}

PyDoc_STRVAR(typevartuple_typing_subst__doc__,
"__typing_subst__($self, /, arg)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_TYPING_SUBST_METHODDEF    \
    {"__typing_subst__", _PyCFunction_CAST(typevartuple_typing_subst), METH_FASTCALL|METH_KEYWORDS, typevartuple_typing_subst__doc__},

static PyObject *
typevartuple_typing_subst_impl(typevartupleobject *self, PyObject *arg);

static PyObject *
typevartuple_typing_subst(typevartupleobject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(arg), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"arg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__typing_subst__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *arg;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    arg = args[0];
    return_value = typevartuple_typing_subst_impl(self, arg);

exit:
    return return_value;
}

PyDoc_STRVAR(typevartuple_typing_prepare_subst__doc__,
"__typing_prepare_subst__($self, /, alias, args)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_TYPING_PREPARE_SUBST_METHODDEF    \
    {"__typing_prepare_subst__", _PyCFunction_CAST(typevartuple_typing_prepare_subst), METH_FASTCALL|METH_KEYWORDS, typevartuple_typing_prepare_subst__doc__},

static PyObject *
typevartuple_typing_prepare_subst_impl(typevartupleobject *self,
                                       PyObject *alias, PyObject *args);

static PyObject *
typevartuple_typing_prepare_subst(typevartupleobject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(alias), &_Py_ID(args), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"alias", "args", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__typing_prepare_subst__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *alias;
    PyObject *__clinic_args;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    alias = args[0];
    __clinic_args = args[1];
    return_value = typevartuple_typing_prepare_subst_impl(self, alias, __clinic_args);

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
typevartuple_reduce(typevartupleobject *self, PyObject *Py_UNUSED(ignored))
{
    return typevartuple_reduce_impl(self);
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
typealias_reduce(typealiasobject *self, PyObject *Py_UNUSED(ignored))
{
    return typealias_reduce_impl(self);
}

PyDoc_STRVAR(typealias_new__doc__,
"typealias(name, value, *, type_params=<unrepresentable>)\n"
"--\n"
"\n"
"Create a TypeAliasType.");

static PyObject *
typealias_new_impl(PyTypeObject *type, PyObject *name, PyObject *value,
                   PyObject *type_params);

static PyObject *
typealias_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(name), &_Py_ID(value), &_Py_ID(type_params), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "value", "type_params", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "typealias",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 2;
    PyObject *name;
    PyObject *value;
    PyObject *type_params = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 2, 2, 0, argsbuf);
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
    type_params = fastargs[2];
skip_optional_kwonly:
    return_value = typealias_new_impl(type, name, value, type_params);

exit:
    return return_value;
}
/*[clinic end generated code: output=807bcd30ebd10ac3 input=a9049054013a1b77]*/
