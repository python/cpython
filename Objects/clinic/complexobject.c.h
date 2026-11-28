/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_BadArgument()

PyDoc_STRVAR(complex_conjugate__doc__,
"conjugate($self, /)\n"
"--\n"
"\n"
"Return the complex conjugate of its argument. (3-4j).conjugate() == 3+4j.");

#define COMPLEX_CONJUGATE_METHODDEF    \
    {"conjugate", (PyCFunction)complex_conjugate, METH_NOARGS, complex_conjugate__doc__},

static PyObject *
complex_conjugate_impl(PyComplexObject *self);

static PyObject *
complex_conjugate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return complex_conjugate_impl((PyComplexObject *)self);
}

PyDoc_STRVAR(complex___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define COMPLEX___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", (PyCFunction)complex___getnewargs__, METH_NOARGS, complex___getnewargs____doc__},

static PyObject *
complex___getnewargs___impl(PyComplexObject *self);

static PyObject *
complex___getnewargs__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return complex___getnewargs___impl((PyComplexObject *)self);
}

PyDoc_STRVAR(complex___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Convert to a string according to format_spec.");

#define COMPLEX___FORMAT___METHODDEF    \
    {"__format__", (PyCFunction)complex___format__, METH_O, complex___format____doc__},

static PyObject *
complex___format___impl(PyComplexObject *self, PyObject *format_spec);

static PyObject *
complex___format__(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *format_spec;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("__format__", "argument", "str", arg);
        goto exit;
    }
    format_spec = arg;
    return_value = complex___format___impl((PyComplexObject *)self, format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(complex___complex____doc__,
"__complex__($self, /)\n"
"--\n"
"\n"
"Convert this value to exact type complex.");

#define COMPLEX___COMPLEX___METHODDEF    \
    {"__complex__", (PyCFunction)complex___complex__, METH_NOARGS, complex___complex____doc__},

static PyObject *
complex___complex___impl(PyComplexObject *self);

static PyObject *
complex___complex__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return complex___complex___impl((PyComplexObject *)self);
}

PyDoc_STRVAR(complex_new__doc__,
"complex(real=0, imag=0)\n"
"--\n"
"\n"
"Create a complex number from a string or numbers.\n"
"\n"
"If a string is given, parse it as a complex number.\n"
"If a single number is given, convert it to a complex number.\n"
"If the \'real\' or \'imag\' arguments are given, create a complex number\n"
"with the specified real and imaginary components.");

static PyObject *
complex_new_impl(PyTypeObject *type, PyObject *real_pos, PyObject *imag_pos,
                 PyObject *r, PyObject *i);

static PyObject *
complex_new_helper(PyTypeObject *type, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(real), &_Py_ID(imag), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "real", "imag", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "complex",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 0;
    PyObject *real_pos = NULL;
    PyObject *imag_pos = NULL;
    PyObject *r = NULL;
    PyObject *i = NULL;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    noptargs--;
    real_pos = fastargs[0];
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    imag_pos = fastargs[1];
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[2]) {
        r = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    i = fastargs[3];
skip_optional_kwonly:
    return_value = complex_new_impl(type, real_pos, imag_pos, r, i);

exit:
    return return_value;
}

static PyObject *
complex_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return complex_new_helper(type, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
complex_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);

    assert(_PyType_CAST(type)->tp_new == complex_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    return complex_new_helper(_PyType_CAST(type), args, nargs,
        kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
        NULL, kwnames);
}

PyDoc_STRVAR(complex_from_number__doc__,
"from_number($type, number, /)\n"
"--\n"
"\n"
"Convert number to a complex floating-point number.");

#define COMPLEX_FROM_NUMBER_METHODDEF    \
    {"from_number", (PyCFunction)complex_from_number, METH_O|METH_CLASS, complex_from_number__doc__},

static PyObject *
complex_from_number_impl(PyTypeObject *type, PyObject *number);

static PyObject *
complex_from_number(PyObject *type, PyObject *number)
{
    PyObject *return_value = NULL;

    return_value = complex_from_number_impl((PyTypeObject *)type, number);

    return return_value;
}
/*[clinic end generated code: output=35512ea9996f9935 input=a9049054013a1b77]*/
