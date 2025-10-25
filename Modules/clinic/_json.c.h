/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_json_scanstring__doc__,
"scanstring($module, pystr, end, strict=True, /)\n"
"--\n"
"\n"
"Scan the string s for a JSON string.\n"
"\n"
"Return a tuple of the decoded string and the index of the character in s\n"
"after the end quote.");

#define _JSON_SCANSTRING_METHODDEF    \
    {"scanstring", _PyCFunction_CAST(_json_scanstring), METH_FASTCALL, _json_scanstring__doc__},

static PyObject *
_json_scanstring_impl(PyObject *module, PyObject *pystr, Py_ssize_t end,
                      int strict);

static PyObject *
_json_scanstring(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *pystr;
    Py_ssize_t end;
    int strict = 1;

    if (!_PyArg_CheckPositional("scanstring", nargs, 2, 3)) {
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
    return_value = _json_scanstring_impl(module, pystr, end, strict);

exit:
    return return_value;
}

PyDoc_STRVAR(_json_encode_basestring_ascii__doc__,
"encode_basestring_ascii($module, pystr, /)\n"
"--\n"
"\n"
"Return an ASCII-only JSON representation of a Python string");

#define _JSON_ENCODE_BASESTRING_ASCII_METHODDEF    \
    {"encode_basestring_ascii", (PyCFunction)_json_encode_basestring_ascii, METH_O, _json_encode_basestring_ascii__doc__},

PyDoc_STRVAR(_json_encode_basestring__doc__,
"encode_basestring($module, pystr, /)\n"
"--\n"
"\n"
"Return a JSON representation of a Python string");

#define _JSON_ENCODE_BASESTRING_METHODDEF    \
    {"encode_basestring", (PyCFunction)_json_encode_basestring, METH_O, _json_encode_basestring__doc__},
/*[clinic end generated code: output=7f57cc1b016aad17 input=a9049054013a1b77]*/
