/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(memoryview_hex__doc__,
"hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Return the data in the buffer as a str of hexadecimal numbers.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from the\n"
"    right, negative values count from the left.\n"
"\n"
"Example:\n"
">>> value = memoryview(b\'\\xb9\\x01\\xef\')\n"
">>> value.hex()\n"
"\'b901ef\'\n"
">>> value.hex(\':\')\n"
"\'b9:01:ef\'\n"
">>> value.hex(\':\', 2)\n"
"\'b9:01ef\'\n"
">>> value.hex(\':\', -2)\n"
"\'b901:ef\'");

#define MEMORYVIEW_HEX_METHODDEF    \
    {"hex", (PyCFunction)(void(*)(void))memoryview_hex, METH_FASTCALL|METH_KEYWORDS, memoryview_hex__doc__},

static PyObject *
memoryview_hex_impl(PyMemoryViewObject *self, PyObject *sep,
                    int bytes_per_sep);

static PyObject *
memoryview_hex(PyMemoryViewObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sep", "bytes_per_sep", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "hex", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *sep = NULL;
    int bytes_per_sep = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        sep = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    bytes_per_sep = _PyLong_AsInt(args[1]);
    if (bytes_per_sep == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = memoryview_hex_impl(self, sep, bytes_per_sep);

exit:
    return return_value;
}
/*[clinic end generated code: output=ee265a73f68b0077 input=a9049054013a1b77]*/
