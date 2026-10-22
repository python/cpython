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
    {"write_input", (PyCFunction)(void(*)(void))_testconsole_write_input, METH_VARARGS|METH_KEYWORDS, _testconsole_write_input__doc__},

static PyObject *
_testconsole_write_input_impl(PyObject *module, PyObject *file, Py_buffer *s);

static PyObject *
_testconsole_write_input(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"file", "s", NULL};
    PyObject *file;
    Py_buffer s = {NULL, NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oy*:write_input", _keywords,
        &file, &s))
        goto exit;
    return_value = _testconsole_write_input_impl(module, file, &s);

exit:
    /* Cleanup for s */
    if (s.obj) {
       PyBuffer_Release(&s);
    }

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
    {"read_output", (PyCFunction)(void(*)(void))_testconsole_read_output, METH_VARARGS|METH_KEYWORDS, _testconsole_read_output__doc__},

static PyObject *
_testconsole_read_output_impl(PyObject *module, PyObject *file);

static PyObject *
_testconsole_read_output(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"file", NULL};
    PyObject *file;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:read_output", _keywords,
        &file))
        goto exit;
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
/*[clinic end generated code: output=d60ce07157e3741a input=a9049054013a1b77]*/
