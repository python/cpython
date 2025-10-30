/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(py_scanstring__doc__,
"scanstring($module, pystr, end, strict=True, /)\n"
"--\n"
"\n"
"Scan the string s for a JSON string.\n"
"\n"
"End is the index of the character in s after the quote that started the\n"
"JSON string. Unescapes all valid JSON string escape sequences and raises\n"
"ValueError on attempt to decode an invalid string. If strict is False\n"
"then literal control characters are allowed in the string.\n"
"\n"
"Returns a tuple of the decoded string and the index of the character in s\n"
"after the end quote.");

#define PY_SCANSTRING_METHODDEF    \
    {"scanstring", _PyCFunction_CAST(py_scanstring), METH_FASTCALL, py_scanstring__doc__},

static PyObject *
py_scanstring_impl(PyObject *module, PyObject *pystr, Py_ssize_t end,
                   int strict);

static PyObject *
py_scanstring(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *pystr;
    Py_ssize_t end;
    int strict = 1;

    if (!_PyArg_CheckPositional("scanstring", nargs, 2, 3)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("scanstring", "argument 1", "str", args[0]);
        goto exit;
    }
    pystr = args[0];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        end = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    strict = PyObject_IsTrue(args[2]);
    if (strict < 0) {
        goto exit;
    }
skip_optional:
    return_value = py_scanstring_impl(module, pystr, end, strict);

exit:
    return return_value;
}

PyDoc_STRVAR(py_encode_basestring_ascii__doc__,
"encode_basestring_ascii($module, pystr, /)\n"
"--\n"
"\n"
"Return an ASCII-only JSON representation of a Python string");

#define PY_ENCODE_BASESTRING_ASCII_METHODDEF    \
    {"encode_basestring_ascii", (PyCFunction)py_encode_basestring_ascii, METH_O, py_encode_basestring_ascii__doc__},

static PyObject *
py_encode_basestring_ascii_impl(PyObject *module, PyObject *pystr);

static PyObject *
py_encode_basestring_ascii(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *pystr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("encode_basestring_ascii", "argument", "str", arg);
        goto exit;
    }
    pystr = arg;
    return_value = py_encode_basestring_ascii_impl(module, pystr);

exit:
    return return_value;
}

PyDoc_STRVAR(py_encode_basestring__doc__,
"encode_basestring($module, pystr, /)\n"
"--\n"
"\n"
"Return a JSON representation of a Python string");

#define PY_ENCODE_BASESTRING_METHODDEF    \
    {"encode_basestring", (PyCFunction)py_encode_basestring, METH_O, py_encode_basestring__doc__},

static PyObject *
py_encode_basestring_impl(PyObject *module, PyObject *pystr);

static PyObject *
py_encode_basestring(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *pystr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("encode_basestring", "argument", "str", arg);
        goto exit;
    }
    pystr = arg;
    return_value = py_encode_basestring_impl(module, pystr);

exit:
    return return_value;
}
/*[clinic end generated code: output=5bdd16375c95a4d9 input=a9049054013a1b77]*/
