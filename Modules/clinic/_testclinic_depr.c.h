/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_long.h"          // _PyLong_UnsignedShort_Converter()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_tuple.h"         // _PyTuple_FromArray()

PyDoc_STRVAR(depr_star_new__doc__,
"DeprStarNew(a=None)\n"
"--\n"
"\n"
"The deprecation message should use the class name instead of __new__.\n"
"\n"
"Note: Passing positional arguments to _testclinic.DeprStarNew() is\n"
"deprecated. Parameter \'a\' will become a keyword-only parameter in\n"
"Python 3.37.\n"
"");

static PyObject *
depr_star_new_impl(PyTypeObject *type, PyObject *a);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of '_testclinic.DeprStarNew.__new__'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of '_testclinic.DeprStarNew.__new__'.")
#  else
#    warning "Update the clinic input of '_testclinic.DeprStarNew.__new__'."
#  endif
#endif

static PyObject *
depr_star_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { _Py_LATIN1_CHR('a'), },
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
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *a = Py_None;

    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to _testclinic.DeprStarNew() is "
                "deprecated. Parameter 'a' will become a keyword-only parameter "
                "in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    a = fastargs[0];
skip_optional_pos:
    return_value = depr_star_new_impl(type, a);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_new_clone__doc__,
"cloned($self, /, a=None)\n"
"--\n"
"\n"
"Note: Passing positional arguments to _testclinic.DeprStarNew.cloned()\n"
"is deprecated. Parameter \'a\' will become a keyword-only parameter in\n"
"Python 3.37.\n"
"");

#define DEPR_STAR_NEW_CLONE_METHODDEF    \
    {"cloned", _PyCFunction_CAST(depr_star_new_clone), METH_FASTCALL|METH_KEYWORDS, depr_star_new_clone__doc__},

static PyObject *
depr_star_new_clone_impl(PyObject *type, PyObject *a);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of '_testclinic.DeprStarNew.cloned'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of '_testclinic.DeprStarNew.cloned'.")
#  else
#    warning "Update the clinic input of '_testclinic.DeprStarNew.cloned'."
#  endif
#endif

static PyObject *
depr_star_new_clone(PyObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), },
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
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *a = Py_None;

    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to _testclinic.DeprStarNew.cloned()"
                " is deprecated. Parameter 'a' will become a keyword-only "
                "parameter in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    a = args[0];
skip_optional_pos:
    return_value = depr_star_new_clone_impl(type, a);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_init__doc__,
"DeprStarInit(a=None)\n"
"--\n"
"\n"
"The deprecation message should use the class name instead of __init__.\n"
"\n"
"Note: Passing positional arguments to _testclinic.DeprStarInit() is\n"
"deprecated. Parameter \'a\' will become a keyword-only parameter in\n"
"Python 3.37.\n"
"");

static int
depr_star_init_impl(PyObject *self, PyObject *a);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of '_testclinic.DeprStarInit.__init__'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of '_testclinic.DeprStarInit.__init__'.")
#  else
#    warning "Update the clinic input of '_testclinic.DeprStarInit.__init__'."
#  endif
#endif

static int
depr_star_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
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
        .ob_item = { _Py_LATIN1_CHR('a'), },
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
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *a = Py_None;

    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to _testclinic.DeprStarInit() is "
                "deprecated. Parameter 'a' will become a keyword-only parameter "
                "in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    a = fastargs[0];
skip_optional_pos:
    return_value = depr_star_init_impl(self, a);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_init_clone__doc__,
"cloned($self, /, a=None)\n"
"--\n"
"\n"
"Note: Passing positional arguments to\n"
"_testclinic.DeprStarInit.cloned() is deprecated. Parameter \'a\' will\n"
"become a keyword-only parameter in Python 3.37.\n"
"");

#define DEPR_STAR_INIT_CLONE_METHODDEF    \
    {"cloned", _PyCFunction_CAST(depr_star_init_clone), METH_FASTCALL|METH_KEYWORDS, depr_star_init_clone__doc__},

static PyObject *
depr_star_init_clone_impl(PyObject *self, PyObject *a);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of '_testclinic.DeprStarInit.cloned'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of '_testclinic.DeprStarInit.cloned'.")
#  else
#    warning "Update the clinic input of '_testclinic.DeprStarInit.cloned'."
#  endif
#endif

static PyObject *
depr_star_init_clone(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), },
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
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *a = Py_None;

    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to "
                "_testclinic.DeprStarInit.cloned() is deprecated. Parameter 'a' "
                "will become a keyword-only parameter in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    a = args[0];
skip_optional_pos:
    return_value = depr_star_init_clone_impl(self, a);

exit:
    return return_value;
}

static int
depr_star_init_noinline_impl(PyObject *self, PyObject *a, PyObject *b,
                             PyObject *c, const char *d, Py_ssize_t d_length);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of '_testclinic.DeprStarInitNoInline.__init__'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of '_testclinic.DeprStarInitNoInline.__init__'.")
#  else
#    warning "Update the clinic input of '_testclinic.DeprStarInitNoInline.__init__'."
#  endif
#endif

static int
depr_star_init_noinline(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "OO|O$s#:DeprStarInitNoInline",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    const char *d = "";
    Py_ssize_t d_length;

    if (nargs > 1 && nargs <= 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 1 positional argument to "
                "_testclinic.DeprStarInitNoInline() is deprecated. Parameters 'b'"
                " and 'c' will become keyword-only parameters in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &a, &b, &c, &d, &d_length)) {
        goto exit;
    }
    return_value = depr_star_init_noinline_impl(self, a, b, c, d, d_length);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_new__doc__,
"DeprKwdNew(a=None)\n"
"--\n"
"\n"
"The deprecation message should use the class name instead of __new__.\n"
"\n"
"Note: Passing keyword argument \'a\' to _testclinic.DeprKwdNew() is\n"
"deprecated. Parameter \'a\' will become positional-only in Python 3.37.\n"
"");

static PyObject *
depr_kwd_new_impl(PyTypeObject *type, PyObject *a);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of '_testclinic.DeprKwdNew.__new__'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of '_testclinic.DeprKwdNew.__new__'.")
#  else
#    warning "Update the clinic input of '_testclinic.DeprKwdNew.__new__'."
#  endif
#endif

static PyObject *
depr_kwd_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { _Py_LATIN1_CHR('a'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "DeprKwdNew",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *a = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (kwargs && PyDict_GET_SIZE(kwargs) && nargs < 1 && fastargs[0]) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword argument 'a' to _testclinic.DeprKwdNew() is "
                "deprecated. Parameter 'a' will become positional-only in Python "
                "3.37.", 1))
        {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    a = fastargs[0];
skip_optional_pos:
    return_value = depr_kwd_new_impl(type, a);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_init__doc__,
"DeprKwdInit(a=None)\n"
"--\n"
"\n"
"The deprecation message should use the class name instead of __init__.\n"
"\n"
"Note: Passing keyword argument \'a\' to _testclinic.DeprKwdInit() is\n"
"deprecated. Parameter \'a\' will become positional-only in Python 3.37.\n"
"");

static int
depr_kwd_init_impl(PyObject *self, PyObject *a);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of '_testclinic.DeprKwdInit.__init__'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of '_testclinic.DeprKwdInit.__init__'.")
#  else
#    warning "Update the clinic input of '_testclinic.DeprKwdInit.__init__'."
#  endif
#endif

static int
depr_kwd_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
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
        .ob_item = { _Py_LATIN1_CHR('a'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "DeprKwdInit",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *a = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (kwargs && PyDict_GET_SIZE(kwargs) && nargs < 1 && fastargs[0]) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword argument 'a' to _testclinic.DeprKwdInit() is "
                "deprecated. Parameter 'a' will become positional-only in Python "
                "3.37.", 1))
        {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    a = fastargs[0];
skip_optional_pos:
    return_value = depr_kwd_init_impl(self, a);

exit:
    return return_value;
}

static int
depr_kwd_init_noinline_impl(PyObject *self, PyObject *a, PyObject *b,
                            PyObject *c, const char *d, Py_ssize_t d_length);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of '_testclinic.DeprKwdInitNoInline.__init__'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of '_testclinic.DeprKwdInitNoInline.__init__'.")
#  else
#    warning "Update the clinic input of '_testclinic.DeprKwdInitNoInline.__init__'."
#  endif
#endif

static int
depr_kwd_init_noinline(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
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
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "OO|Os#:DeprKwdInitNoInline",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    const char *d = "";
    Py_ssize_t d_length;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &a, &b, &c, &d, &d_length)) {
        goto exit;
    }
    if (kwargs && PyDict_GET_SIZE(kwargs) && ((nargs < 2) || (nargs < 3 && PyDict_Contains(kwargs, _Py_LATIN1_CHR('c'))))) {
        if (PyErr_Occurred()) { // PyDict_Contains() above can fail
            goto exit;
        }
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword arguments 'b' and 'c' to "
                "_testclinic.DeprKwdInitNoInline() is deprecated. Parameters 'b' "
                "and 'c' will become positional-only in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    return_value = depr_kwd_init_noinline_impl(self, a, b, c, d, d_length);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_pos0_len1__doc__,
"depr_star_pos0_len1($module, /, a)\n"
"--\n"
"\n"
"Note: Passing positional arguments to depr_star_pos0_len1() is\n"
"deprecated. Parameter \'a\' will become a keyword-only parameter in\n"
"Python 3.37.\n"
"");

#define DEPR_STAR_POS0_LEN1_METHODDEF    \
    {"depr_star_pos0_len1", _PyCFunction_CAST(depr_star_pos0_len1), METH_FASTCALL|METH_KEYWORDS, depr_star_pos0_len1__doc__},

static PyObject *
depr_star_pos0_len1_impl(PyObject *module, PyObject *a);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_pos0_len1'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_pos0_len1'.")
#  else
#    warning "Update the clinic input of 'depr_star_pos0_len1'."
#  endif
#endif

static PyObject *
depr_star_pos0_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), },
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

    if (nargs == 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to depr_star_pos0_len1() is "
                "deprecated. Parameter 'a' will become a keyword-only parameter "
                "in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
"in Python 3.37.\n"
"");

#define DEPR_STAR_POS0_LEN2_METHODDEF    \
    {"depr_star_pos0_len2", _PyCFunction_CAST(depr_star_pos0_len2), METH_FASTCALL|METH_KEYWORDS, depr_star_pos0_len2__doc__},

static PyObject *
depr_star_pos0_len2_impl(PyObject *module, PyObject *a, PyObject *b);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_pos0_len2'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_pos0_len2'.")
#  else
#    warning "Update the clinic input of 'depr_star_pos0_len2'."
#  endif
#endif

static PyObject *
depr_star_pos0_len2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), },
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

    if (nargs > 0 && nargs <= 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to depr_star_pos0_len2() is "
                "deprecated. Parameters 'a' and 'b' will become keyword-only "
                "parameters in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
"parameters in Python 3.37.\n"
"");

#define DEPR_STAR_POS0_LEN3_WITH_KWD_METHODDEF    \
    {"depr_star_pos0_len3_with_kwd", _PyCFunction_CAST(depr_star_pos0_len3_with_kwd), METH_FASTCALL|METH_KEYWORDS, depr_star_pos0_len3_with_kwd__doc__},

static PyObject *
depr_star_pos0_len3_with_kwd_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject *c, PyObject *d);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_pos0_len3_with_kwd'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_pos0_len3_with_kwd'.")
#  else
#    warning "Update the clinic input of 'depr_star_pos0_len3_with_kwd'."
#  endif
#endif

static PyObject *
depr_star_pos0_len3_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
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

    if (nargs > 0 && nargs <= 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing positional arguments to depr_star_pos0_len3_with_kwd() "
                "is deprecated. Parameters 'a', 'b' and 'c' will become "
                "keyword-only parameters in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
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
"Python 3.37.\n"
"");

#define DEPR_STAR_POS1_LEN1_OPT_METHODDEF    \
    {"depr_star_pos1_len1_opt", _PyCFunction_CAST(depr_star_pos1_len1_opt), METH_FASTCALL|METH_KEYWORDS, depr_star_pos1_len1_opt__doc__},

static PyObject *
depr_star_pos1_len1_opt_impl(PyObject *module, PyObject *a, PyObject *b);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_pos1_len1_opt'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_pos1_len1_opt'.")
#  else
#    warning "Update the clinic input of 'depr_star_pos1_len1_opt'."
#  endif
#endif

static PyObject *
depr_star_pos1_len1_opt(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), },
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

    if (nargs == 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing 2 positional arguments to depr_star_pos1_len1_opt() is "
                "deprecated. Parameter 'b' will become a keyword-only parameter "
                "in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
"Python 3.37.\n"
"");

#define DEPR_STAR_POS1_LEN1_METHODDEF    \
    {"depr_star_pos1_len1", _PyCFunction_CAST(depr_star_pos1_len1), METH_FASTCALL|METH_KEYWORDS, depr_star_pos1_len1__doc__},

static PyObject *
depr_star_pos1_len1_impl(PyObject *module, PyObject *a, PyObject *b);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_pos1_len1'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_pos1_len1'.")
#  else
#    warning "Update the clinic input of 'depr_star_pos1_len1'."
#  endif
#endif

static PyObject *
depr_star_pos1_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), },
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

    if (nargs == 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing 2 positional arguments to depr_star_pos1_len1() is "
                "deprecated. Parameter 'b' will become a keyword-only parameter "
                "in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
"will become keyword-only parameters in Python 3.37.\n"
"");

#define DEPR_STAR_POS1_LEN2_WITH_KWD_METHODDEF    \
    {"depr_star_pos1_len2_with_kwd", _PyCFunction_CAST(depr_star_pos1_len2_with_kwd), METH_FASTCALL|METH_KEYWORDS, depr_star_pos1_len2_with_kwd__doc__},

static PyObject *
depr_star_pos1_len2_with_kwd_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject *c, PyObject *d);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_pos1_len2_with_kwd'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_pos1_len2_with_kwd'.")
#  else
#    warning "Update the clinic input of 'depr_star_pos1_len2_with_kwd'."
#  endif
#endif

static PyObject *
depr_star_pos1_len2_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
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

    if (nargs > 1 && nargs <= 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 1 positional argument to "
                "depr_star_pos1_len2_with_kwd() is deprecated. Parameters 'b' and"
                " 'c' will become keyword-only parameters in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
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
"Python 3.37.\n"
"");

#define DEPR_STAR_POS2_LEN1_METHODDEF    \
    {"depr_star_pos2_len1", _PyCFunction_CAST(depr_star_pos2_len1), METH_FASTCALL|METH_KEYWORDS, depr_star_pos2_len1__doc__},

static PyObject *
depr_star_pos2_len1_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_pos2_len1'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_pos2_len1'.")
#  else
#    warning "Update the clinic input of 'depr_star_pos2_len1'."
#  endif
#endif

static PyObject *
depr_star_pos2_len1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
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

    if (nargs == 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing 3 positional arguments to depr_star_pos2_len1() is "
                "deprecated. Parameter 'c' will become a keyword-only parameter "
                "in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
"become keyword-only parameters in Python 3.37.\n"
"");

#define DEPR_STAR_POS2_LEN2_METHODDEF    \
    {"depr_star_pos2_len2", _PyCFunction_CAST(depr_star_pos2_len2), METH_FASTCALL|METH_KEYWORDS, depr_star_pos2_len2__doc__},

static PyObject *
depr_star_pos2_len2_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c, PyObject *d);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_pos2_len2'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_pos2_len2'.")
#  else
#    warning "Update the clinic input of 'depr_star_pos2_len2'."
#  endif
#endif

static PyObject *
depr_star_pos2_len2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
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

    if (nargs > 2 && nargs <= 4) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 2 positional arguments to "
                "depr_star_pos2_len2() is deprecated. Parameters 'c' and 'd' will"
                " become keyword-only parameters in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 4, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
"will become keyword-only parameters in Python 3.37.\n"
"");

#define DEPR_STAR_POS2_LEN2_WITH_KWD_METHODDEF    \
    {"depr_star_pos2_len2_with_kwd", _PyCFunction_CAST(depr_star_pos2_len2_with_kwd), METH_FASTCALL|METH_KEYWORDS, depr_star_pos2_len2_with_kwd__doc__},

static PyObject *
depr_star_pos2_len2_with_kwd_impl(PyObject *module, PyObject *a, PyObject *b,
                                  PyObject *c, PyObject *d, PyObject *e);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_pos2_len2_with_kwd'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_pos2_len2_with_kwd'.")
#  else
#    warning "Update the clinic input of 'depr_star_pos2_len2_with_kwd'."
#  endif
#endif

static PyObject *
depr_star_pos2_len2_with_kwd(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), _Py_LATIN1_CHR('e'), },
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

    if (nargs > 2 && nargs <= 4) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 2 positional arguments to "
                "depr_star_pos2_len2_with_kwd() is deprecated. Parameters 'c' and"
                " 'd' will become keyword-only parameters in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 4, /*maxpos*/ 4, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
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

PyDoc_STRVAR(depr_star_noinline__doc__,
"depr_star_noinline($module, /, a, b, c=None, *, d=\'\')\n"
"--\n"
"\n"
"Note: Passing more than 1 positional argument to depr_star_noinline()\n"
"is deprecated. Parameters \'b\' and \'c\' will become keyword-only\n"
"parameters in Python 3.37.\n"
"");

#define DEPR_STAR_NOINLINE_METHODDEF    \
    {"depr_star_noinline", _PyCFunction_CAST(depr_star_noinline), METH_FASTCALL|METH_KEYWORDS, depr_star_noinline__doc__},

static PyObject *
depr_star_noinline_impl(PyObject *module, PyObject *a, PyObject *b,
                        PyObject *c, const char *d, Py_ssize_t d_length);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_noinline'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_noinline'.")
#  else
#    warning "Update the clinic input of 'depr_star_noinline'."
#  endif
#endif

static PyObject *
depr_star_noinline(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "OO|O$s#:depr_star_noinline",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    const char *d = "";
    Py_ssize_t d_length;

    if (nargs > 1 && nargs <= 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 1 positional argument to depr_star_noinline() "
                "is deprecated. Parameters 'b' and 'c' will become keyword-only "
                "parameters in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &a, &b, &c, &d, &d_length)) {
        goto exit;
    }
    return_value = depr_star_noinline_impl(module, a, b, c, d, d_length);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_star_multi__doc__,
"depr_star_multi($module, /, a, b, c, d, e, f, g, *, h)\n"
"--\n"
"\n"
"Note: Passing more than 1 positional argument to depr_star_multi() is\n"
"deprecated. Parameter \'b\' will become a keyword-only parameter in\n"
"Python 3.39. Parameters \'c\' and \'d\' will become keyword-only\n"
"parameters in Python 3.38. Parameters \'e\', \'f\' and \'g\' will become\n"
"keyword-only parameters in Python 3.37.\n"
"");

#define DEPR_STAR_MULTI_METHODDEF    \
    {"depr_star_multi", _PyCFunction_CAST(depr_star_multi), METH_FASTCALL|METH_KEYWORDS, depr_star_multi__doc__},

static PyObject *
depr_star_multi_impl(PyObject *module, PyObject *a, PyObject *b, PyObject *c,
                     PyObject *d, PyObject *e, PyObject *f, PyObject *g,
                     PyObject *h);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_star_multi'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_star_multi'.")
#  else
#    warning "Update the clinic input of 'depr_star_multi'."
#  endif
#endif

static PyObject *
depr_star_multi(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 8
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), _Py_LATIN1_CHR('e'), _Py_LATIN1_CHR('f'), _Py_LATIN1_CHR('g'), _Py_LATIN1_CHR('h'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", "d", "e", "f", "g", "h", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_star_multi",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[8];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;
    PyObject *e;
    PyObject *f;
    PyObject *g;
    PyObject *h;

    if (nargs > 1 && nargs <= 7) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 1 positional argument to depr_star_multi() is "
                "deprecated. Parameter 'b' will become a keyword-only parameter "
                "in Python 3.39. Parameters 'c' and 'd' will become keyword-only "
                "parameters in Python 3.38. Parameters 'e', 'f' and 'g' will "
                "become keyword-only parameters in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 7, /*maxpos*/ 7, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    c = args[2];
    d = args[3];
    e = args[4];
    f = args[5];
    g = args[6];
    h = args[7];
    return_value = depr_star_multi_impl(module, a, b, c, d, e, f, g, h);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_required_1__doc__,
"depr_kwd_required_1($module, a, /, b)\n"
"--\n"
"\n"
"Note: Passing keyword argument \'b\' to depr_kwd_required_1() is\n"
"deprecated. Parameter \'b\' will become positional-only in Python 3.37.\n"
"");

#define DEPR_KWD_REQUIRED_1_METHODDEF    \
    {"depr_kwd_required_1", _PyCFunction_CAST(depr_kwd_required_1), METH_FASTCALL|METH_KEYWORDS, depr_kwd_required_1__doc__},

static PyObject *
depr_kwd_required_1_impl(PyObject *module, PyObject *a, PyObject *b);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_kwd_required_1'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_kwd_required_1'.")
#  else
#    warning "Update the clinic input of 'depr_kwd_required_1'."
#  endif
#endif

static PyObject *
depr_kwd_required_1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_kwd_required_1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *a;
    PyObject *b;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 2) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword argument 'b' to depr_kwd_required_1() is "
                "deprecated. Parameter 'b' will become positional-only in Python "
                "3.37.", 1))
        {
            goto exit;
        }
    }
    a = args[0];
    b = args[1];
    return_value = depr_kwd_required_1_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_required_2__doc__,
"depr_kwd_required_2($module, a, /, b, c)\n"
"--\n"
"\n"
"Note: Passing keyword arguments \'b\' and \'c\' to depr_kwd_required_2()\n"
"is deprecated. Parameters \'b\' and \'c\' will become positional-only in\n"
"Python 3.37.\n"
"");

#define DEPR_KWD_REQUIRED_2_METHODDEF    \
    {"depr_kwd_required_2", _PyCFunction_CAST(depr_kwd_required_2), METH_FASTCALL|METH_KEYWORDS, depr_kwd_required_2__doc__},

static PyObject *
depr_kwd_required_2_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_kwd_required_2'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_kwd_required_2'.")
#  else
#    warning "Update the clinic input of 'depr_kwd_required_2'."
#  endif
#endif

static PyObject *
depr_kwd_required_2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_kwd_required_2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject *a;
    PyObject *b;
    PyObject *c;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword arguments 'b' and 'c' to depr_kwd_required_2() "
                "is deprecated. Parameters 'b' and 'c' will become "
                "positional-only in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    a = args[0];
    b = args[1];
    c = args[2];
    return_value = depr_kwd_required_2_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_optional_1__doc__,
"depr_kwd_optional_1($module, a, /, b=None)\n"
"--\n"
"\n"
"Note: Passing keyword argument \'b\' to depr_kwd_optional_1() is\n"
"deprecated. Parameter \'b\' will become positional-only in Python 3.37.\n"
"");

#define DEPR_KWD_OPTIONAL_1_METHODDEF    \
    {"depr_kwd_optional_1", _PyCFunction_CAST(depr_kwd_optional_1), METH_FASTCALL|METH_KEYWORDS, depr_kwd_optional_1__doc__},

static PyObject *
depr_kwd_optional_1_impl(PyObject *module, PyObject *a, PyObject *b);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_kwd_optional_1'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_kwd_optional_1'.")
#  else
#    warning "Update the clinic input of 'depr_kwd_optional_1'."
#  endif
#endif

static PyObject *
depr_kwd_optional_1(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_kwd_optional_1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (kwnames && PyTuple_GET_SIZE(kwnames) && nargs < 2 && args[1]) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword argument 'b' to depr_kwd_optional_1() is "
                "deprecated. Parameter 'b' will become positional-only in Python "
                "3.37.", 1))
        {
            goto exit;
        }
    }
    a = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    b = args[1];
skip_optional_pos:
    return_value = depr_kwd_optional_1_impl(module, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_optional_2__doc__,
"depr_kwd_optional_2($module, a, /, b=None, c=None)\n"
"--\n"
"\n"
"Note: Passing keyword arguments \'b\' and \'c\' to depr_kwd_optional_2()\n"
"is deprecated. Parameters \'b\' and \'c\' will become positional-only in\n"
"Python 3.37.\n"
"");

#define DEPR_KWD_OPTIONAL_2_METHODDEF    \
    {"depr_kwd_optional_2", _PyCFunction_CAST(depr_kwd_optional_2), METH_FASTCALL|METH_KEYWORDS, depr_kwd_optional_2__doc__},

static PyObject *
depr_kwd_optional_2_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_kwd_optional_2'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_kwd_optional_2'.")
#  else
#    warning "Update the clinic input of 'depr_kwd_optional_2'."
#  endif
#endif

static PyObject *
depr_kwd_optional_2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_kwd_optional_2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (kwnames && PyTuple_GET_SIZE(kwnames) && ((nargs < 2 && args[1]) || (nargs < 3 && args[2]))) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword arguments 'b' and 'c' to depr_kwd_optional_2() "
                "is deprecated. Parameters 'b' and 'c' will become "
                "positional-only in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    a = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        b = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    c = args[2];
skip_optional_pos:
    return_value = depr_kwd_optional_2_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_optional_3__doc__,
"depr_kwd_optional_3($module, /, a=None, b=None, c=None)\n"
"--\n"
"\n"
"Note: Passing keyword arguments \'a\', \'b\' and \'c\' to\n"
"depr_kwd_optional_3() is deprecated. Parameters \'a\', \'b\' and \'c\' will\n"
"become positional-only in Python 3.37.\n"
"");

#define DEPR_KWD_OPTIONAL_3_METHODDEF    \
    {"depr_kwd_optional_3", _PyCFunction_CAST(depr_kwd_optional_3), METH_FASTCALL|METH_KEYWORDS, depr_kwd_optional_3__doc__},

static PyObject *
depr_kwd_optional_3_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_kwd_optional_3'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_kwd_optional_3'.")
#  else
#    warning "Update the clinic input of 'depr_kwd_optional_3'."
#  endif
#endif

static PyObject *
depr_kwd_optional_3(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_kwd_optional_3",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *a = Py_None;
    PyObject *b = Py_None;
    PyObject *c = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (kwnames && PyTuple_GET_SIZE(kwnames) && ((nargs < 1 && args[0]) || (nargs < 2 && args[1]) || (nargs < 3 && args[2]))) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword arguments 'a', 'b' and 'c' to "
                "depr_kwd_optional_3() is deprecated. Parameters 'a', 'b' and 'c'"
                " will become positional-only in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        a = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        b = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    c = args[2];
skip_optional_pos:
    return_value = depr_kwd_optional_3_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_required_optional__doc__,
"depr_kwd_required_optional($module, a, /, b, c=None)\n"
"--\n"
"\n"
"Note: Passing keyword arguments \'b\' and \'c\' to\n"
"depr_kwd_required_optional() is deprecated. Parameters \'b\' and \'c\'\n"
"will become positional-only in Python 3.37.\n"
"");

#define DEPR_KWD_REQUIRED_OPTIONAL_METHODDEF    \
    {"depr_kwd_required_optional", _PyCFunction_CAST(depr_kwd_required_optional), METH_FASTCALL|METH_KEYWORDS, depr_kwd_required_optional__doc__},

static PyObject *
depr_kwd_required_optional_impl(PyObject *module, PyObject *a, PyObject *b,
                                PyObject *c);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_kwd_required_optional'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_kwd_required_optional'.")
#  else
#    warning "Update the clinic input of 'depr_kwd_required_optional'."
#  endif
#endif

static PyObject *
depr_kwd_required_optional(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_kwd_required_optional",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (kwnames && PyTuple_GET_SIZE(kwnames) && ((nargs < 2) || (nargs < 3 && args[2]))) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword arguments 'b' and 'c' to "
                "depr_kwd_required_optional() is deprecated. Parameters 'b' and "
                "'c' will become positional-only in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    a = args[0];
    b = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    c = args[2];
skip_optional_pos:
    return_value = depr_kwd_required_optional_impl(module, a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_noinline__doc__,
"depr_kwd_noinline($module, a, /, b, c=None, d=\'\')\n"
"--\n"
"\n"
"Note: Passing keyword arguments \'b\' and \'c\' to depr_kwd_noinline() is\n"
"deprecated. Parameters \'b\' and \'c\' will become positional-only in\n"
"Python 3.37.\n"
"");

#define DEPR_KWD_NOINLINE_METHODDEF    \
    {"depr_kwd_noinline", _PyCFunction_CAST(depr_kwd_noinline), METH_FASTCALL|METH_KEYWORDS, depr_kwd_noinline__doc__},

static PyObject *
depr_kwd_noinline_impl(PyObject *module, PyObject *a, PyObject *b,
                       PyObject *c, const char *d, Py_ssize_t d_length);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_kwd_noinline'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_kwd_noinline'.")
#  else
#    warning "Update the clinic input of 'depr_kwd_noinline'."
#  endif
#endif

static PyObject *
depr_kwd_noinline(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "OO|Os#:depr_kwd_noinline",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    const char *d = "";
    Py_ssize_t d_length;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &a, &b, &c, &d, &d_length)) {
        goto exit;
    }
    if (kwnames && PyTuple_GET_SIZE(kwnames) && ((nargs < 2) || (nargs < 3 && PySequence_Contains(kwnames, _Py_LATIN1_CHR('c'))))) {
        if (PyErr_Occurred()) { // PySequence_Contains() above can fail
            goto exit;
        }
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword arguments 'b' and 'c' to depr_kwd_noinline() is "
                "deprecated. Parameters 'b' and 'c' will become positional-only "
                "in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    return_value = depr_kwd_noinline_impl(module, a, b, c, d, d_length);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_kwd_multi__doc__,
"depr_kwd_multi($module, a, /, b, c, d, e, f, g, h)\n"
"--\n"
"\n"
"Note: Passing keyword arguments \'b\', \'c\', \'d\', \'e\', \'f\' and \'g\' to\n"
"depr_kwd_multi() is deprecated. Parameter \'b\' will become positional-\n"
"only in Python 3.37. Parameters \'c\' and \'d\' will become positional-\n"
"only in Python 3.38. Parameters \'e\', \'f\' and \'g\' will become\n"
"positional-only in Python 3.39.\n"
"");

#define DEPR_KWD_MULTI_METHODDEF    \
    {"depr_kwd_multi", _PyCFunction_CAST(depr_kwd_multi), METH_FASTCALL|METH_KEYWORDS, depr_kwd_multi__doc__},

static PyObject *
depr_kwd_multi_impl(PyObject *module, PyObject *a, PyObject *b, PyObject *c,
                    PyObject *d, PyObject *e, PyObject *f, PyObject *g,
                    PyObject *h);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_kwd_multi'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_kwd_multi'.")
#  else
#    warning "Update the clinic input of 'depr_kwd_multi'."
#  endif
#endif

static PyObject *
depr_kwd_multi(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 7
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), _Py_LATIN1_CHR('e'), _Py_LATIN1_CHR('f'), _Py_LATIN1_CHR('g'), _Py_LATIN1_CHR('h'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", "d", "e", "f", "g", "h", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_kwd_multi",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[8];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;
    PyObject *e;
    PyObject *f;
    PyObject *g;
    PyObject *h;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 8, /*maxpos*/ 8, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 7) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword arguments 'b', 'c', 'd', 'e', 'f' and 'g' to "
                "depr_kwd_multi() is deprecated. Parameter 'b' will become "
                "positional-only in Python 3.37. Parameters 'c' and 'd' will "
                "become positional-only in Python 3.38. Parameters 'e', 'f' and "
                "'g' will become positional-only in Python 3.39.", 1))
        {
            goto exit;
        }
    }
    a = args[0];
    b = args[1];
    c = args[2];
    d = args[3];
    e = args[4];
    f = args[5];
    g = args[6];
    h = args[7];
    return_value = depr_kwd_multi_impl(module, a, b, c, d, e, f, g, h);

exit:
    return return_value;
}

PyDoc_STRVAR(depr_multi__doc__,
"depr_multi($module, a, /, b, c, d, e, f, *, g)\n"
"--\n"
"\n"
"Note: Passing keyword arguments \'b\' and \'c\' to depr_multi() is\n"
"deprecated. Parameter \'b\' will become positional-only in Python 3.37.\n"
"Parameter \'c\' will become positional-only in Python 3.38.\n"
"\n"
"\n"
"Note: Passing more than 4 positional arguments to depr_multi() is\n"
"deprecated. Parameter \'e\' will become a keyword-only parameter in\n"
"Python 3.38. Parameter \'f\' will become a keyword-only parameter in\n"
"Python 3.37.\n"
"");

#define DEPR_MULTI_METHODDEF    \
    {"depr_multi", _PyCFunction_CAST(depr_multi), METH_FASTCALL|METH_KEYWORDS, depr_multi__doc__},

static PyObject *
depr_multi_impl(PyObject *module, PyObject *a, PyObject *b, PyObject *c,
                PyObject *d, PyObject *e, PyObject *f, PyObject *g);

// Emit compiler warnings when we get to Python 3.37.
#if PY_VERSION_HEX >= 0x032500C0
#  error "Update the clinic input of 'depr_multi'."
#elif PY_VERSION_HEX >= 0x032500A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'depr_multi'.")
#  else
#    warning "Update the clinic input of 'depr_multi'."
#  endif
#endif

static PyObject *
depr_multi(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), _Py_LATIN1_CHR('e'), _Py_LATIN1_CHR('f'), _Py_LATIN1_CHR('g'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", "d", "e", "f", "g", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "depr_multi",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[7];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d;
    PyObject *e;
    PyObject *f;
    PyObject *g;

    if (nargs > 4 && nargs <= 6) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing more than 4 positional arguments to depr_multi() is "
                "deprecated. Parameter 'e' will become a keyword-only parameter "
                "in Python 3.38. Parameter 'f' will become a keyword-only "
                "parameter in Python 3.37.", 1))
        {
            goto exit;
        }
    }
    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 6, /*maxpos*/ 6, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 3) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword arguments 'b' and 'c' to depr_multi() is "
                "deprecated. Parameter 'b' will become positional-only in Python "
                "3.37. Parameter 'c' will become positional-only in Python 3.38.", 1))
        {
            goto exit;
        }
    }
    a = args[0];
    b = args[1];
    c = args[2];
    d = args[3];
    e = args[4];
    f = args[5];
    g = args[6];
    return_value = depr_multi_impl(module, a, b, c, d, e, f, g);

exit:
    return return_value;
}
/*[clinic end generated code: output=517bb49913bafc4a input=a9049054013a1b77]*/
