/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(test_empty_function__doc__,
"test_empty_function($module, /)\n"
"--\n"
"\n");

#define TEST_EMPTY_FUNCTION_METHODDEF    \
    {"test_empty_function", (PyCFunction)test_empty_function, METH_NOARGS, test_empty_function__doc__},

static PyObject *
test_empty_function_impl(PyObject *module);

static PyObject *
test_empty_function(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return test_empty_function_impl(module);
}

PyDoc_STRVAR(gh_32092_oob__doc__,
"gh_32092_oob($module, /, pos1, pos2, *varargs, kw1=None, kw2=None)\n"
"--\n"
"\n"
"Proof-of-concept of GH-32092 OOB bug.\n"
"\n"
"Array index out-of-bound bug in function\n"
"`_PyArg_UnpackKeywordsWithVararg` .\n"
"\n"
"Calling this function by gh_32092_oob(1, 2, 3, 4, kw1=5, kw2=6)\n"
"to trigger this bug (crash).");

#define GH_32092_OOB_METHODDEF    \
    {"gh_32092_oob", _PyCFunction_CAST(gh_32092_oob), METH_FASTCALL|METH_KEYWORDS, gh_32092_oob__doc__},

static PyObject *
gh_32092_oob_impl(PyObject *module, PyObject *pos1, PyObject *pos2,
                  PyObject *varargs, PyObject *kw1, PyObject *kw2);

static PyObject *
gh_32092_oob(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(pos1), &_Py_ID(pos2), &_Py_ID(kw1), &_Py_ID(kw2), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pos1", "pos2", "kw1", "kw2", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "gh_32092_oob",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = 0 + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *pos1;
    PyObject *pos2;
    PyObject *varargs = NULL;
    PyObject *kw1 = Py_None;
    PyObject *kw2 = Py_None;

    args = _PyArg_UnpackKeywordsWithVararg(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, 2, argsbuf);
    if (!args) {
        goto exit;
    }
    pos1 = args[0];
    pos2 = args[1];
    varargs = args[2];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[3]) {
        kw1 = args[3];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    kw2 = args[4];
skip_optional_kwonly:
    return_value = gh_32092_oob_impl(module, pos1, pos2, varargs, kw1, kw2);

exit:
    Py_XDECREF(varargs);
    return return_value;
}

PyDoc_STRVAR(gh_32092_kw_pass__doc__,
"gh_32092_kw_pass($module, /, pos, *args, kw=None)\n"
"--\n"
"\n"
"Proof-of-concept of GH-32092 keyword args passing bug.\n"
"\n"
"The calculation of `noptargs` in AC-generated function\n"
"`builtin_kw_pass_poc` is incorrect.\n"
"\n"
"Calling this function by gh_32092_kw_pass(1, 2, 3)\n"
"to trigger this bug (crash).");

#define GH_32092_KW_PASS_METHODDEF    \
    {"gh_32092_kw_pass", _PyCFunction_CAST(gh_32092_kw_pass), METH_FASTCALL|METH_KEYWORDS, gh_32092_kw_pass__doc__},

static PyObject *
gh_32092_kw_pass_impl(PyObject *module, PyObject *pos, PyObject *args,
                      PyObject *kw);

static PyObject *
gh_32092_kw_pass(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(pos), &_Py_ID(kw), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pos", "kw", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "gh_32092_kw_pass",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = 0 + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *pos;
    PyObject *__clinic_args = NULL;
    PyObject *kw = Py_None;

    args = _PyArg_UnpackKeywordsWithVararg(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, 1, argsbuf);
    if (!args) {
        goto exit;
    }
    pos = args[0];
    __clinic_args = args[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    kw = args[2];
skip_optional_kwonly:
    return_value = gh_32092_kw_pass_impl(module, pos, __clinic_args, kw);

exit:
    Py_XDECREF(__clinic_args);
    return return_value;
}
/*[clinic end generated code: output=f1f47303a227104b input=a9049054013a1b77]*/
