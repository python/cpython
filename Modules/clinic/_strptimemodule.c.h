/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_strptime_impl__strptime_parse__doc__,
"_strptime_parse($module, data_string, format, /)\n"
"--\n"
"\n"
"Parse a time string according to a format.\n"
"\n"
"Returns a 3-tuple on success, or None if the format contains\n"
"directives that require the Python fallback path.");

#define _STRPTIME_IMPL__STRPTIME_PARSE_METHODDEF    \
    {"_strptime_parse", _PyCFunction_CAST(_strptime_impl__strptime_parse), METH_FASTCALL, _strptime_impl__strptime_parse__doc__},

static PyObject *
_strptime_impl__strptime_parse_impl(PyObject *module, PyObject *data_string,
                                    PyObject *format);

static PyObject *
_strptime_impl__strptime_parse(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *data_string;
    PyObject *format;

    if (!_PyArg_CheckPositional("_strptime_parse", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("_strptime_parse", "argument 1", "str", args[0]);
        goto exit;
    }
    data_string = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("_strptime_parse", "argument 2", "str", args[1]);
        goto exit;
    }
    format = args[1];
    return_value = _strptime_impl__strptime_parse_impl(module, data_string, format);

exit:
    return return_value;
}
/*[clinic end generated code: output=e22b619e9547671c input=a9049054013a1b77]*/
