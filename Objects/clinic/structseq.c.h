/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
structseq_new_impl(PyTypeObject *type, PyObject *arg, PyObject *dict);

static PyObject *
structseq_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sequence", "dict", NULL};
    static _PyArg_Parser _parser = {"O|O:structseq", _keywords, 0};
    PyObject *arg;
    PyObject *dict = NULL;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &arg, &dict)) {
        goto exit;
    }
    return_value = structseq_new_impl(type, arg, dict);

exit:
    return return_value;
}
/*[clinic end generated code: output=cd643eb89b5d312a input=a9049054013a1b77]*/
