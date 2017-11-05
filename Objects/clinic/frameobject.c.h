/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
fastlocalsproxy_new_impl(PyTypeObject *type, PyObject *frame);

static PyObject *
fastlocalsproxy_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"frame", NULL};
    static _PyArg_Parser _parser = {"O:fastlocalsproxy", _keywords, 0};
    PyObject *frame;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &frame)) {
        goto exit;
    }
    return_value = fastlocalsproxy_new_impl(type, frame);

exit:
    return return_value;
}
/*[clinic end generated code: output=5fa72522109d3584 input=a9049054013a1b77]*/
