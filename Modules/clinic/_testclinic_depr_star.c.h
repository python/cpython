/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(depr_star_new__doc__,
"DeprStarNew(a)\n"
"--\n"
"\n"
"The deprecation message should use the class name instead of __new__.\n"
"\n"
"Note: Passing positional arguments to _testclinic.DeprStarNew() is\n"
"deprecated. Parameter \'a\' will become a keyword-only parameter in\n"
"Python 3.14.\n"
"");

static PyObject *
depr_star_new_impl(PyTypeObject *type, PyObject *a);

static PyObject *
depr_star_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(a), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "DeprStarNew",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *a;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarNew.__new__' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarNew.__new__' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarNew.__new__' to be keyword-only."
    #  endif
    #endif
    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to _testclinic.DeprStarNew() is "
                "deprecated. Parameter 'a' will become a keyword-only parameter "
                "in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = fastargs[0];
    return_value = depr_star_new_impl(type, a);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_new_clone__doc__,
"cloned($self, /, a)\n"
"--\n"
"\n"
"Note: Passing positional arguments to _testclinic.DeprStarNew.cloned()\n"
"is deprecated. Parameter \'a\' will become a keyword-only parameter in\n"
"Python 3.14.\n"
"");

#define DEPR_STAR_NEW_CLONE_METHODDEF    \
    {"cloned", _PyCFunction_CAST(depr_star_new_clone), METH_FASTCALL|METH_KEYWORDS, depr_star_new_clone__doc__},

static PyObject *
depr_star_new_clone_impl(PyObject *type, PyObject *a);

static PyObject *
depr_star_new_clone(PyObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(a), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cloned",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *a;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarNew.cloned' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarNew.cloned' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarNew.cloned' to be keyword-only."
    #  endif
    #endif
    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to _testclinic.DeprStarNew.cloned()"
                " is deprecated. Parameter 'a' will become a keyword-only "
                "parameter in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    return_value = depr_star_new_clone_impl(type, a);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_init__doc__,
"DeprStarInit(a)\n"
"--\n"
"\n"
"The deprecation message should use the class name instead of __init__.\n"
"\n"
"Note: Passing positional arguments to _testclinic.DeprStarInit() is\n"
"deprecated. Parameter \'a\' will become a keyword-only parameter in\n"
"Python 3.14.\n"
"");

static int
depr_star_init_impl(PyObject *self, PyObject *a);

static int
depr_star_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(a), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "DeprStarInit",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *a;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarInit.__init__' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarInit.__init__' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarInit.__init__' to be keyword-only."
    #  endif
    #endif
    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to _testclinic.DeprStarInit() is "
                "deprecated. Parameter 'a' will become a keyword-only parameter "
                "in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = fastargs[0];
    return_value = depr_star_init_impl(self, a);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_init_clone__doc__,
"cloned($self, /, a)\n"
"--\n"
"\n"
"Note: Passing positional arguments to\n"
"_testclinic.DeprStarInit.cloned() is deprecated. Parameter \'a\' will\n"
"become a keyword-only parameter in Python 3.14.\n"
"");

#define DEPR_STAR_INIT_CLONE_METHODDEF    \
    {"cloned", _PyCFunction_CAST(depr_star_init_clone), METH_FASTCALL|METH_KEYWORDS, depr_star_init_clone__doc__},

static PyObject *
depr_star_init_clone_impl(PyObject *self, PyObject *a);

static PyObject *
depr_star_init_clone(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(a), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cloned",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *a;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarInit.cloned' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarInit.cloned' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " '_testclinic.DeprStarInit.cloned' to be keyword-only."
    #  endif
    #endif
    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to "
                "_testclinic.DeprStarInit.cloned() is deprecated. Parameter 'a' "
                "will become a keyword-only parameter in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    return_value = depr_star_init_clone_impl(self, a);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos0_len1__doc__,
"depr_star_pos0_len1($module, /, a)\n"
"--\n"
"\n"
"Note: Passing positional arguments to depr_star_pos0_len1() is\n"
"deprecated. Parameter \'a\' will become a keyword-only parameter in\n"
"Python 3.14.\n"
"");

#define DEPR_STAR_POS0_LEN1_METHODDEF    \
    {"depr_star_pos0_len1", _PyCFunction_CAST(depr_star_pos0_len1), METH_FASTCALL|METH_KEYWORDS, depr_star_pos0_len1__doc__},

static PyObject *
depr_star_pos0_len1_impl(PyObject *module, PyObject *a);

static PyObject *
depr_star_pos0_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(a), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_pos0_len1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *a;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " 'depr_star_pos0_len1' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " 'depr_star_pos0_len1' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " 'depr_star_pos0_len1' to be keyword-only."
    #  endif
    #endif
    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to depr_star_pos0_len1() is "
                "deprecated. Parameter 'a' will become a keyword-only parameter "
                "in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    return_value = depr_star_pos0_len1_impl(module, a);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos0_len2__doc__,
"depr_star_pos0_len2($module, /, a, b)\n"
"--\n"
"\n"
"Note: Passing positional arguments to depr_star_pos0_len2() is\n"
"deprecated. Parameters \'a\' and \'b\' will become keyword-only parameters\n"
"in Python 3.14.\n"
"");

#define DEPR_STAR_POS0_LEN2_METHODDEF    \
    {"depr_star_pos0_len2", _PyCFunction_CAST(depr_star_pos0_len2), METH_FASTCALL|METH_KEYWORDS, depr_star_pos0_len2__doc__},

static PyObject *
depr_star_pos0_len2_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
depr_star_pos0_len2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(a), &_Py_ID(b), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_pos0_len2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *b;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a' and 'b' in the clinic " \
            "input of 'depr_star_pos0_len2' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a' and 'b' in the clinic " \
            "input of 'depr_star_pos0_len2' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a' and 'b' in the clinic " \
            "input of 'depr_star_pos0_len2' to be keyword-only."
    #  endif
    #endif
    if (nargs > 0 && nargs <= 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to depr_star_pos0_len2() is "
                "deprecated. Parameters 'a' and 'b' will become keyword-only "
                "parameters in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = depr_star_pos0_len2_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos0_len3_with_kwd__doc__,
"depr_star_pos0_len3_with_kwd($module, /, a, b, c, *, d)\n"
"--\n"
"\n"
"Note: Passing positional arguments to depr_star_pos0_len3_with_kwd()\n"
"is deprecated. Parameters \'a\', \'b\' and \'c\' will become keyword-only\n"
"parameters in Python 3.14.\n"
"");

#define DEPR_STAR_POS0_LEN3_WITH_KWD_METHODDEF    \
    {"depr_star_pos0_len3_with_kwd", _PyCFunction_CAST(depr_star_pos0_len3_with_kwd), METH_FASTCALL|METH_KEYWORDS, depr_star_pos0_len3_with_kwd__doc__},

static PyObject *
depr_star_pos0_len3_with_kwd_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject *c, PyObject *d);

static PyObject *
depr_star_pos0_len3_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(a), &_Py_ID(b), &_Py_ID(c), &_Py_ID(d), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_pos0_len3_with_kwd",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a', 'b' and 'c' in the " \
            "clinic input of 'depr_star_pos0_len3_with_kwd' to be " \
            "keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a', 'b' and 'c' in the " \
            "clinic input of 'depr_star_pos0_len3_with_kwd' to be " \
            "keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a', 'b' and 'c' in the " \
            "clinic input of 'depr_star_pos0_len3_with_kwd' to be " \
            "keyword-only."
    #  endif
    #endif
    if (nargs > 0 && nargs <= 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to depr_star_pos0_len3_with_kwd() "
                "is deprecated. Parameters 'a', 'b' and 'c' will become "
                "keyword-only parameters in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    c = args[2];
    d = args[3];
    return_value = depr_star_pos0_len3_with_kwd_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos1_len1_opt__doc__,
"depr_star_pos1_len1_opt($module, /, a, b=None)\n"
"--\n"
"\n"
"Note: Passing 2 positional arguments to depr_star_pos1_len1_opt() is\n"
"deprecated. Parameter \'b\' will become a keyword-only parameter in\n"
"Python 3.14.\n"
"");

#define DEPR_STAR_POS1_LEN1_OPT_METHODDEF    \
    {"depr_star_pos1_len1_opt", _PyCFunction_CAST(depr_star_pos1_len1_opt), METH_FASTCALL|METH_KEYWORDS, depr_star_pos1_len1_opt__doc__},

static PyObject *
depr_star_pos1_len1_opt_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
depr_star_pos1_len1_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(a), &_Py_ID(b), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_pos1_len1_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'depr_star_pos1_len1_opt' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'depr_star_pos1_len1_opt' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'depr_star_pos1_len1_opt' to be keyword-only."
    #  endif
    #endif
    if (nargs == 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing 2 positional arguments to depr_star_pos1_len1_opt() is "
                "deprecated. Parameter 'b' will become a keyword-only parameter "
                "in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    b = args[1];
skip_optional_pos:
    return_value = depr_star_pos1_len1_opt_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos1_len1__doc__,
"depr_star_pos1_len1($module, /, a, b)\n"
"--\n"
"\n"
"Note: Passing 2 positional arguments to depr_star_pos1_len1() is\n"
"deprecated. Parameter \'b\' will become a keyword-only parameter in\n"
"Python 3.14.\n"
"");

#define DEPR_STAR_POS1_LEN1_METHODDEF    \
    {"depr_star_pos1_len1", _PyCFunction_CAST(depr_star_pos1_len1), METH_FASTCALL|METH_KEYWORDS, depr_star_pos1_len1__doc__},

static PyObject *
depr_star_pos1_len1_impl(PyObject *module, PyObject *a, PyObject *b);

static PyObject *
depr_star_pos1_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(a), &_Py_ID(b), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_pos1_len1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *b;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'depr_star_pos1_len1' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'depr_star_pos1_len1' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'depr_star_pos1_len1' to be keyword-only."
    #  endif
    #endif
    if (nargs == 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing 2 positional arguments to depr_star_pos1_len1() is "
                "deprecated. Parameter 'b' will become a keyword-only parameter "
                "in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = depr_star_pos1_len1_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos1_len2_with_kwd__doc__,
"depr_star_pos1_len2_with_kwd($module, /, a, b, c, *, d)\n"
"--\n"
"\n"
"Note: Passing more than 1 positional argument to\n"
"depr_star_pos1_len2_with_kwd() is deprecated. Parameters \'b\' and \'c\'\n"
"will become keyword-only parameters in Python 3.14.\n"
"");

#define DEPR_STAR_POS1_LEN2_WITH_KWD_METHODDEF    \
    {"depr_star_pos1_len2_with_kwd", _PyCFunction_CAST(depr_star_pos1_len2_with_kwd), METH_FASTCALL|METH_KEYWORDS, depr_star_pos1_len2_with_kwd__doc__},

static PyObject *
depr_star_pos1_len2_with_kwd_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject *c, PyObject *d);

static PyObject *
depr_star_pos1_len2_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(a), &_Py_ID(b), &_Py_ID(c), &_Py_ID(d), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_pos1_len2_with_kwd",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'b' and 'c' in the clinic " \
            "input of 'depr_star_pos1_len2_with_kwd' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'b' and 'c' in the clinic " \
            "input of 'depr_star_pos1_len2_with_kwd' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'b' and 'c' in the clinic " \
            "input of 'depr_star_pos1_len2_with_kwd' to be keyword-only."
    #  endif
    #endif
    if (nargs > 1 && nargs <= 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 1 positional argument to "
                "depr_star_pos1_len2_with_kwd() is deprecated. Parameters 'b' and"
                " 'c' will become keyword-only parameters in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    c = args[2];
    d = args[3];
    return_value = depr_star_pos1_len2_with_kwd_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos2_len1__doc__,
"depr_star_pos2_len1($module, /, a, b, c)\n"
"--\n"
"\n"
"Note: Passing 3 positional arguments to depr_star_pos2_len1() is\n"
"deprecated. Parameter \'c\' will become a keyword-only parameter in\n"
"Python 3.14.\n"
"");

#define DEPR_STAR_POS2_LEN1_METHODDEF    \
    {"depr_star_pos2_len1", _PyCFunction_CAST(depr_star_pos2_len1), METH_FASTCALL|METH_KEYWORDS, depr_star_pos2_len1__doc__},

static PyObject *
depr_star_pos2_len1_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c);

static PyObject *
depr_star_pos2_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(a), &_Py_ID(b), &_Py_ID(c), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_pos2_len1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject *a;
    PyObject *b;
    PyObject *c;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'c' in the clinic input of" \
            " 'depr_star_pos2_len1' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'c' in the clinic input of" \
            " 'depr_star_pos2_len1' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'c' in the clinic input of" \
            " 'depr_star_pos2_len1' to be keyword-only."
    #  endif
    #endif
    if (nargs == 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing 3 positional arguments to depr_star_pos2_len1() is "
                "deprecated. Parameter 'c' will become a keyword-only parameter "
                "in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    c = args[2];
    return_value = depr_star_pos2_len1_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos2_len2__doc__,
"depr_star_pos2_len2($module, /, a, b, c, d)\n"
"--\n"
"\n"
"Note: Passing more than 2 positional arguments to\n"
"depr_star_pos2_len2() is deprecated. Parameters \'c\' and \'d\' will\n"
"become keyword-only parameters in Python 3.14.\n"
"");

#define DEPR_STAR_POS2_LEN2_METHODDEF    \
    {"depr_star_pos2_len2", _PyCFunction_CAST(depr_star_pos2_len2), METH_FASTCALL|METH_KEYWORDS, depr_star_pos2_len2__doc__},

static PyObject *
depr_star_pos2_len2_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c, PyObject *d);

static PyObject *
depr_star_pos2_len2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(a), &_Py_ID(b), &_Py_ID(c), &_Py_ID(d), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_pos2_len2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'depr_star_pos2_len2' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'depr_star_pos2_len2' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'depr_star_pos2_len2' to be keyword-only."
    #  endif
    #endif
    if (nargs > 2 && nargs <= 4) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 2 positional arguments to "
                "depr_star_pos2_len2() is deprecated. Parameters 'c' and 'd' will"
                " become keyword-only parameters in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 4, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    c = args[2];
    d = args[3];
    return_value = depr_star_pos2_len2_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos2_len2_with_kwd__doc__,
"depr_star_pos2_len2_with_kwd($module, /, a, b, c, d, *, e)\n"
"--\n"
"\n"
"Note: Passing more than 2 positional arguments to\n"
"depr_star_pos2_len2_with_kwd() is deprecated. Parameters \'c\' and \'d\'\n"
"will become keyword-only parameters in Python 3.14.\n"
"");

#define DEPR_STAR_POS2_LEN2_WITH_KWD_METHODDEF    \
    {"depr_star_pos2_len2_with_kwd", _PyCFunction_CAST(depr_star_pos2_len2_with_kwd), METH_FASTCALL|METH_KEYWORDS, depr_star_pos2_len2_with_kwd__doc__},

static PyObject *
depr_star_pos2_len2_with_kwd_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject *c, PyObject *d, PyObject *e);

static PyObject *
depr_star_pos2_len2_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(a), &_Py_ID(b), &_Py_ID(c), &_Py_ID(d), &_Py_ID(e), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", "d", "e", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_pos2_len2_with_kwd",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;
    PyObject *e;

    // Emit compiler warnings when we get to Python 3.14.
    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'depr_star_pos2_len2_with_kwd' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'depr_star_pos2_len2_with_kwd' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'depr_star_pos2_len2_with_kwd' to be keyword-only."
    #  endif
    #endif
    if (nargs > 2 && nargs <= 4) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 2 positional arguments to "
                "depr_star_pos2_len2_with_kwd() is deprecated. Parameters 'c' and"
                " 'd' will become keyword-only parameters in Python 3.14.", 1))
        {
                goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 4, 4, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    c = args[2];
    d = args[3];
    e = args[4];
    return_value = depr_star_pos2_len2_with_kwd_impl(module, a, b, c, d, e);

exit:
    return return_value;
}
/*[clinic end generated code: output=7a16fee4d6742d54 input=a9049054013a1b77]*/
