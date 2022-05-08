/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_testconsole_write_input__doc__,
"write_input($module, /, file, s)\n"
"--\n"
"\n"
"Writes UTF-16-LE encoded bytes to the console as if typed by a user.");

#define _TESTCONSOLE_WRITE_INPUT_METHODDEF    \
    {"write_input", _PyCFunction_CAST(_testconsole_write_input), METH_FASTCALL|METH_KEYWORDS, _testconsole_write_input__doc__},

static PyObject *
_testconsole_write_input_impl(PyObject *module, PyObject *file,
                              PyBytesObject *s);

static PyObject *
_testconsole_write_input(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"file", "s", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "write_input", 0};
    PyObject *argsbuf[2];
    PyObject *file;
    PyBytesObject *s;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    file = args[0];
    if (!PyBytes_Check(args[1])) {
        _PyArg_BadArgument("write_input", "argument 's'", "bytes", args[1]);
        goto exit;
    }
    s = (PyBytesObject *)args[1];
    return_value = _testconsole_write_input_impl(module, file, s);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_testconsole_read_output__doc__,
"read_output($module, /, file)\n"
"--\n"
"\n"
"Reads a str from the console as written to stdout.");

#define _TESTCONSOLE_READ_OUTPUT_METHODDEF    \
    {"read_output", _PyCFunction_CAST(_testconsole_read_output), METH_FASTCALL|METH_KEYWORDS, _testconsole_read_output__doc__},

static PyObject *
_testconsole_read_output_impl(PyObject *module, PyObject *file);

static PyObject *
_testconsole_read_output(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"file", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "read_output", 0};
    PyObject *argsbuf[1];
    PyObject *file;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    file = args[0];
    return_value = _testconsole_read_output_impl(module, file);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#ifndef _TESTCONSOLE_WRITE_INPUT_METHODDEF
    #define _TESTCONSOLE_WRITE_INPUT_METHODDEF
#endif /* !defined(_TESTCONSOLE_WRITE_INPUT_METHODDEF) */

#ifndef _TESTCONSOLE_READ_OUTPUT_METHODDEF
    #define _TESTCONSOLE_READ_OUTPUT_METHODDEF
#endif /* !defined(_TESTCONSOLE_READ_OUTPUT_METHODDEF) */
/*[clinic end generated code: output=6e9f8b0766eb5a0e input=a9049054013a1b77]*/
