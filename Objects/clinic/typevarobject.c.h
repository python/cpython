/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(typevar_new__doc__,
"typevar(name, *constraints, *, bound=None, covariant=False,\n"
"        contravariant=False, autovariance=False)\n"
"--\n"
"\n"
"Create a TypeVar.");

static PyObject *
typevar_new_impl(PyTypeObject *type, const char *name, PyObject *constraints,
                 PyObject *bound, int covariant, int contravariant,
                 int autovariance);

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
        .ob_item = { &_Py_ID(name), &_Py_ID(bound), &_Py_ID(covariant), &_Py_ID(contravariant), &_Py_ID(autovariance), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "bound", "covariant", "contravariant", "autovariance", NULL};
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
    const char *name;
    PyObject *constraints = NULL;
    PyObject *bound = Py_None;
    int covariant = 0;
    int contravariant = 0;
    int autovariance = 0;

    fastargs = _PyArg_UnpackKeywordsWithVararg(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("typevar", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(fastargs[0], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
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
    autovariance = PyObject_IsTrue(fastargs[5]);
    if (autovariance < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = typevar_new_impl(type, name, constraints, bound, covariant, contravariant, autovariance);

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
"          autovariance=False)\n"
"--\n"
"\n"
"Create a ParamSpec object.");

static PyObject *
paramspec_new_impl(PyTypeObject *type, const char *name, PyObject *bound,
                   int covariant, int contravariant, int autovariance);

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
        .ob_item = { &_Py_ID(name), &_Py_ID(bound), &_Py_ID(covariant), &_Py_ID(contravariant), &_Py_ID(autovariance), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "bound", "covariant", "contravariant", "autovariance", NULL};
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
    const char *name;
    PyObject *bound = Py_None;
    int covariant = 0;
    int contravariant = 0;
    int autovariance = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("paramspec", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(fastargs[0], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
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
    autovariance = PyObject_IsTrue(fastargs[4]);
    if (autovariance < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = paramspec_new_impl(type, name, bound, covariant, contravariant, autovariance);

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

PyDoc_STRVAR(typevartuple__doc__,
"typevartuple(name)\n"
"--\n"
"\n"
"Create a new TypeVarTuple with the given name.");

static PyObject *
typevartuple_impl(PyTypeObject *type, const char *name);

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
    const char *name;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("typevartuple", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(fastargs[0], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
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
/*[clinic end generated code: output=b2896de46c103443 input=a9049054013a1b77]*/
