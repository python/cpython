/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(complex_new__doc__,
"complex(real=0, imag=0)\n"
"--\n"
"\n"
"Create a complex number from a real part and an optional imaginary part.\n"
"\n"
"This is equivalent to (real + imag*1j) where imag defaults to 0.");

static PyObject *
complex_new_impl(PyTypeObject *type, PyObject *r, PyObject *i);

static PyObject *
complex_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"real", "imag", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "complex", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *r = _PyLong_Zero;
    PyObject *i = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        r = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    i = fastargs[1];
skip_optional_pos:
    return_value = complex_new_impl(type, r, i);

exit:
    return return_value;
}
/*[clinic end generated code: output=a0fe23fdbdc9b06b input=a9049054013a1b77]*/
