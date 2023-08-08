/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_VAR(deprecate_positional_pos0_len1__doc__);

PyDoc_STRVAR(deprecate_positional_pos0_len1__doc__,
"deprecate_positional_pos0_len1($module, /, a)\n"
"--\n"
"\n"
"Note: Passing positional arguments to deprecate_positional_pos0_len1()\n"
"is deprecated. Parameter \'a\' will become a keyword-only parameter in\n"
"Python 3.14.\n"
"");

#define DEPRECATE_POSITIONAL_POS0_LEN1_METHODDEF    \
    {"deprecate_positional_pos0_len1", _PyCFunction_CAST(deprecate_positional_pos0_len1), METH_FASTCALL|METH_KEYWORDS, deprecate_positional_pos0_len1__doc__},

static PyObject *
deprecate_positional_pos0_len1_impl(PyObject *module, PyObject *a);

static PyObject *
deprecate_positional_pos0_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

static PyObject *
deprecate_positional_pos0_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "deprecate_positional_pos0_len1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *a;

    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " 'deprecate_positional_pos0_len1' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " 'deprecate_positional_pos0_len1' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a' in the clinic input of" \
            " 'deprecate_positional_pos0_len1' to be keyword-only."
    #  endif
    #endif
    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to deprecate_positional_pos0_len1()"
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
    return_value = deprecate_positional_pos0_len1_impl(module, a);

exit:
    return return_value;
}

PyDoc_VAR(deprecate_positional_pos0_len2__doc__);

PyDoc_STRVAR(deprecate_positional_pos0_len2__doc__,
"deprecate_positional_pos0_len2($module, /, a, b)\n"
"--\n"
"\n"
"Note: Passing positional arguments to deprecate_positional_pos0_len2()\n"
"is deprecated. Parameters \'a\' and \'b\' will become keyword-only\n"
"parameters in Python 3.14.\n"
"");

#define DEPRECATE_POSITIONAL_POS0_LEN2_METHODDEF    \
    {"deprecate_positional_pos0_len2", _PyCFunction_CAST(deprecate_positional_pos0_len2), METH_FASTCALL|METH_KEYWORDS, deprecate_positional_pos0_len2__doc__},

static PyObject *
deprecate_positional_pos0_len2_impl(PyObject *module, PyObject *a,
                                    PyObject *b);

static PyObject *
deprecate_positional_pos0_len2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

static PyObject *
deprecate_positional_pos0_len2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "deprecate_positional_pos0_len2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *b;

    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a' and 'b' in the clinic " \
            "input of 'deprecate_positional_pos0_len2' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a' and 'b' in the clinic " \
            "input of 'deprecate_positional_pos0_len2' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a' and 'b' in the clinic " \
            "input of 'deprecate_positional_pos0_len2' to be keyword-only."
    #  endif
    #endif
    if (nargs > 0 && nargs <= 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to deprecate_positional_pos0_len2()"
                " is deprecated. Parameters 'a' and 'b' will become keyword-only "
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
    return_value = deprecate_positional_pos0_len2_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_VAR(deprecate_positional_pos0_len3_with_kwd__doc__);

PyDoc_STRVAR(deprecate_positional_pos0_len3_with_kwd__doc__,
"deprecate_positional_pos0_len3_with_kwd($module, /, a, b, c, *, d)\n"
"--\n"
"\n"
"Note: Passing positional arguments to\n"
"deprecate_positional_pos0_len3_with_kwd() is deprecated. Parameters\n"
"\'a\', \'b\' and \'c\' will become keyword-only parameters in Python 3.14.\n"
"");

#define DEPRECATE_POSITIONAL_POS0_LEN3_WITH_KWD_METHODDEF    \
    {"deprecate_positional_pos0_len3_with_kwd", _PyCFunction_CAST(deprecate_positional_pos0_len3_with_kwd), METH_FASTCALL|METH_KEYWORDS, deprecate_positional_pos0_len3_with_kwd__doc__},

static PyObject *
deprecate_positional_pos0_len3_with_kwd_impl(PyObject *module, PyObject *a,
                                             PyObject *b, PyObject *c,
                                             PyObject *d);

static PyObject *
deprecate_positional_pos0_len3_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

static PyObject *
deprecate_positional_pos0_len3_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "deprecate_positional_pos0_len3_with_kwd",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;

    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'a', 'b' and 'c' in the " \
            "clinic input of 'deprecate_positional_pos0_len3_with_kwd' to be " \
            "keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'a', 'b' and 'c' in the " \
            "clinic input of 'deprecate_positional_pos0_len3_with_kwd' to be " \
            "keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'a', 'b' and 'c' in the " \
            "clinic input of 'deprecate_positional_pos0_len3_with_kwd' to be " \
            "keyword-only."
    #  endif
    #endif
    if (nargs > 0 && nargs <= 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to "
                "deprecate_positional_pos0_len3_with_kwd() is deprecated. "
                "Parameters 'a', 'b' and 'c' will become keyword-only parameters "
                "in Python 3.14.", 1))
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
    return_value = deprecate_positional_pos0_len3_with_kwd_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_VAR(deprecate_positional_pos1_len1_optional__doc__);

PyDoc_STRVAR(deprecate_positional_pos1_len1_optional__doc__,
"deprecate_positional_pos1_len1_optional($module, /, a, b=None)\n"
"--\n"
"\n"
"Note: Passing 2 positional arguments to\n"
"deprecate_positional_pos1_len1_optional() is deprecated. Parameter \'b\'\n"
"will become a keyword-only parameter in Python 3.14.\n"
"");

#define DEPRECATE_POSITIONAL_POS1_LEN1_OPTIONAL_METHODDEF    \
    {"deprecate_positional_pos1_len1_optional", _PyCFunction_CAST(deprecate_positional_pos1_len1_optional), METH_FASTCALL|METH_KEYWORDS, deprecate_positional_pos1_len1_optional__doc__},

static PyObject *
deprecate_positional_pos1_len1_optional_impl(PyObject *module, PyObject *a,
                                             PyObject *b);

static PyObject *
deprecate_positional_pos1_len1_optional(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

static PyObject *
deprecate_positional_pos1_len1_optional(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "deprecate_positional_pos1_len1_optional",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;

    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'deprecate_positional_pos1_len1_optional' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'deprecate_positional_pos1_len1_optional' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'deprecate_positional_pos1_len1_optional' to be keyword-only."
    #  endif
    #endif
    if (nargs == 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing 2 positional arguments to "
                "deprecate_positional_pos1_len1_optional() is deprecated. "
                "Parameter 'b' will become a keyword-only parameter in Python "
                "3.14.", 1))
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
    return_value = deprecate_positional_pos1_len1_optional_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_VAR(deprecate_positional_pos1_len1__doc__);

PyDoc_STRVAR(deprecate_positional_pos1_len1__doc__,
"deprecate_positional_pos1_len1($module, /, a, b)\n"
"--\n"
"\n"
"Note: Passing 2 positional arguments to\n"
"deprecate_positional_pos1_len1() is deprecated. Parameter \'b\' will\n"
"become a keyword-only parameter in Python 3.14.\n"
"");

#define DEPRECATE_POSITIONAL_POS1_LEN1_METHODDEF    \
    {"deprecate_positional_pos1_len1", _PyCFunction_CAST(deprecate_positional_pos1_len1), METH_FASTCALL|METH_KEYWORDS, deprecate_positional_pos1_len1__doc__},

static PyObject *
deprecate_positional_pos1_len1_impl(PyObject *module, PyObject *a,
                                    PyObject *b);

static PyObject *
deprecate_positional_pos1_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

static PyObject *
deprecate_positional_pos1_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "deprecate_positional_pos1_len1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *b;

    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'deprecate_positional_pos1_len1' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'deprecate_positional_pos1_len1' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'b' in the clinic input of" \
            " 'deprecate_positional_pos1_len1' to be keyword-only."
    #  endif
    #endif
    if (nargs == 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing 2 positional arguments to "
                "deprecate_positional_pos1_len1() is deprecated. Parameter 'b' "
                "will become a keyword-only parameter in Python 3.14.", 1))
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
    return_value = deprecate_positional_pos1_len1_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_VAR(deprecate_positional_pos1_len2_with_kwd__doc__);

PyDoc_STRVAR(deprecate_positional_pos1_len2_with_kwd__doc__,
"deprecate_positional_pos1_len2_with_kwd($module, /, a, b, c, *, d)\n"
"--\n"
"\n"
"Note: Passing more than 1 positional argument to\n"
"deprecate_positional_pos1_len2_with_kwd() is deprecated. Parameters\n"
"\'b\' and \'c\' will become keyword-only parameters in Python 3.14.\n"
"");

#define DEPRECATE_POSITIONAL_POS1_LEN2_WITH_KWD_METHODDEF    \
    {"deprecate_positional_pos1_len2_with_kwd", _PyCFunction_CAST(deprecate_positional_pos1_len2_with_kwd), METH_FASTCALL|METH_KEYWORDS, deprecate_positional_pos1_len2_with_kwd__doc__},

static PyObject *
deprecate_positional_pos1_len2_with_kwd_impl(PyObject *module, PyObject *a,
                                             PyObject *b, PyObject *c,
                                             PyObject *d);

static PyObject *
deprecate_positional_pos1_len2_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

static PyObject *
deprecate_positional_pos1_len2_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "deprecate_positional_pos1_len2_with_kwd",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;

    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'b' and 'c' in the clinic " \
            "input of 'deprecate_positional_pos1_len2_with_kwd' to be " \
            "keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'b' and 'c' in the clinic " \
            "input of 'deprecate_positional_pos1_len2_with_kwd' to be " \
            "keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'b' and 'c' in the clinic " \
            "input of 'deprecate_positional_pos1_len2_with_kwd' to be " \
            "keyword-only."
    #  endif
    #endif
    if (nargs > 1 && nargs <= 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 1 positional argument to "
                "deprecate_positional_pos1_len2_with_kwd() is deprecated. "
                "Parameters 'b' and 'c' will become keyword-only parameters in "
                "Python 3.14.", 1))
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
    return_value = deprecate_positional_pos1_len2_with_kwd_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_VAR(deprecate_positional_pos2_len1__doc__);

PyDoc_STRVAR(deprecate_positional_pos2_len1__doc__,
"deprecate_positional_pos2_len1($module, /, a, b, c)\n"
"--\n"
"\n"
"Note: Passing 3 positional arguments to\n"
"deprecate_positional_pos2_len1() is deprecated. Parameter \'c\' will\n"
"become a keyword-only parameter in Python 3.14.\n"
"");

#define DEPRECATE_POSITIONAL_POS2_LEN1_METHODDEF    \
    {"deprecate_positional_pos2_len1", _PyCFunction_CAST(deprecate_positional_pos2_len1), METH_FASTCALL|METH_KEYWORDS, deprecate_positional_pos2_len1__doc__},

static PyObject *
deprecate_positional_pos2_len1_impl(PyObject *module, PyObject *a,
                                    PyObject *b, PyObject *c);

static PyObject *
deprecate_positional_pos2_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

static PyObject *
deprecate_positional_pos2_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "deprecate_positional_pos2_len1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject *a;
    PyObject *b;
    PyObject *c;

    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'c' in the clinic input of" \
            " 'deprecate_positional_pos2_len1' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'c' in the clinic input of" \
            " 'deprecate_positional_pos2_len1' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'c' in the clinic input of" \
            " 'deprecate_positional_pos2_len1' to be keyword-only."
    #  endif
    #endif
    if (nargs == 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing 3 positional arguments to "
                "deprecate_positional_pos2_len1() is deprecated. Parameter 'c' "
                "will become a keyword-only parameter in Python 3.14.", 1))
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
    return_value = deprecate_positional_pos2_len1_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_VAR(deprecate_positional_pos2_len2__doc__);

PyDoc_STRVAR(deprecate_positional_pos2_len2__doc__,
"deprecate_positional_pos2_len2($module, /, a, b, c, d)\n"
"--\n"
"\n"
"Note: Passing more than 2 positional arguments to\n"
"deprecate_positional_pos2_len2() is deprecated. Parameters \'c\' and \'d\'\n"
"will become keyword-only parameters in Python 3.14.\n"
"");

#define DEPRECATE_POSITIONAL_POS2_LEN2_METHODDEF    \
    {"deprecate_positional_pos2_len2", _PyCFunction_CAST(deprecate_positional_pos2_len2), METH_FASTCALL|METH_KEYWORDS, deprecate_positional_pos2_len2__doc__},

static PyObject *
deprecate_positional_pos2_len2_impl(PyObject *module, PyObject *a,
                                    PyObject *b, PyObject *c, PyObject *d);

static PyObject *
deprecate_positional_pos2_len2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

static PyObject *
deprecate_positional_pos2_len2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "deprecate_positional_pos2_len2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;

    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'deprecate_positional_pos2_len2' to be keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'deprecate_positional_pos2_len2' to be keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'deprecate_positional_pos2_len2' to be keyword-only."
    #  endif
    #endif
    if (nargs > 2 && nargs <= 4) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 2 positional arguments to "
                "deprecate_positional_pos2_len2() is deprecated. Parameters 'c' "
                "and 'd' will become keyword-only parameters in Python 3.14.", 1))
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
    return_value = deprecate_positional_pos2_len2_impl(module, a, b, c, d);

exit:
    return return_value;
}

PyDoc_VAR(deprecate_positional_pos2_len3_with_kwd__doc__);

PyDoc_STRVAR(deprecate_positional_pos2_len3_with_kwd__doc__,
"deprecate_positional_pos2_len3_with_kwd($module, /, a, b, c, d, *, e)\n"
"--\n"
"\n"
"Note: Passing more than 2 positional arguments to\n"
"deprecate_positional_pos2_len3_with_kwd() is deprecated. Parameters\n"
"\'c\' and \'d\' will become keyword-only parameters in Python 3.14.\n"
"");

#define DEPRECATE_POSITIONAL_POS2_LEN3_WITH_KWD_METHODDEF    \
    {"deprecate_positional_pos2_len3_with_kwd", _PyCFunction_CAST(deprecate_positional_pos2_len3_with_kwd), METH_FASTCALL|METH_KEYWORDS, deprecate_positional_pos2_len3_with_kwd__doc__},

static PyObject *
deprecate_positional_pos2_len3_with_kwd_impl(PyObject *module, PyObject *a,
                                             PyObject *b, PyObject *c,
                                             PyObject *d, PyObject *e);

static PyObject *
deprecate_positional_pos2_len3_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames);

static PyObject *
deprecate_positional_pos2_len3_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "deprecate_positional_pos2_len3_with_kwd",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;
    PyObject *e;

    #if PY_VERSION_HEX >= 0x030e00C0
    #  error \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'deprecate_positional_pos2_len3_with_kwd' to be " \
            "keyword-only."
    #elif PY_VERSION_HEX >= 0x030e00A0
    #  ifdef _MSC_VER
    #    pragma message ( \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'deprecate_positional_pos2_len3_with_kwd' to be " \
            "keyword-only.")
    #  else
    #    warning \
            "In _testclinic.c, update parameter(s) 'c' and 'd' in the clinic " \
            "input of 'deprecate_positional_pos2_len3_with_kwd' to be " \
            "keyword-only."
    #  endif
    #endif
    if (nargs > 2 && nargs <= 4) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 2 positional arguments to "
                "deprecate_positional_pos2_len3_with_kwd() is deprecated. "
                "Parameters 'c' and 'd' will become keyword-only parameters in "
                "Python 3.14.", 1))
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
    return_value = deprecate_positional_pos2_len3_with_kwd_impl(module, a, b, c, d, e);

exit:
    return return_value;
}
/*[clinic end generated code: output=8a26bcc46d538167 input=a9049054013a1b77]*/
