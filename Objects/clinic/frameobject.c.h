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
    static _PyArg_Parser _parser = {NULL, _keywords, "fastlocalsproxy", 0};
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *frame;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    frame = fastargs[0];
    return_value = fastlocalsproxy_new_impl(type, frame);

exit:
    return return_value;
}
/*[clinic end generated code: output=34617a00e21738f3 input=a9049054013a1b77]*/
