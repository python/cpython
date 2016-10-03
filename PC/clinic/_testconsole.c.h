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
    {"write_input", (PyCFunction)_testconsole_write_input, METH_FASTCALL, _testconsole_write_input__doc__},

static PyObject *
_testconsole_write_input_impl(PyObject *module, PyObject *file,
                              PyBytesObject *s);

static PyObject *
_testconsole_write_input(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"file", "s", NULL};
    static _PyArg_Parser _parser = {"OS:write_input", _keywords, 0};
    PyObject *file;
    PyBytesObject *s;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &file, &s)) {
        goto exit;
    }
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
    {"read_output", (PyCFunction)_testconsole_read_output, METH_FASTCALL, _testconsole_read_output__doc__},

static PyObject *
_testconsole_read_output_impl(PyObject *module, PyObject *file);

static PyObject *
_testconsole_read_output(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"file", NULL};
    static _PyArg_Parser _parser = {"O:read_output", _keywords, 0};
    PyObject *file;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &file)) {
        goto exit;
    }
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
/*[clinic end generated code: output=3a8dc0c421807c41 input=a9049054013a1b77]*/
