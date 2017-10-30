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
    static _PyArg_Parser _parser = {"|OO:complex", _keywords, 0};
    PyObject *r = _PyLong_Zero;
    PyObject *i = NULL;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &r, &i)) {
        goto exit;
    }
    return_value = complex_new_impl(type, r, i);

exit:
    return return_value;
}
/*[clinic end generated code: output=5017b2458bdc4ecd input=a9049054013a1b77]*/
