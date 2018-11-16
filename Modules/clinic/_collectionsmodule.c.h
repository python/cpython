/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
tuplegetter_new_impl(PyTypeObject *type, Py_ssize_t index, PyObject *doc);

static PyObject *
tuplegetter_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"index", "doc", NULL};
    static _PyArg_Parser _parser = {"nO:_tuplegetter", _keywords, 0};
    Py_ssize_t index;
    PyObject *doc;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &index, &doc)) {
        goto exit;
    }
    return_value = tuplegetter_new_impl(type, index, doc);

exit:
    return return_value;
}
/*[clinic end generated code: output=add072a2dd2c77dc input=a9049054013a1b77]*/
