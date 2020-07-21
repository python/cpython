/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
bool_new_impl(PyTypeObject *type, int x);

static PyObject *
bool_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    int x = 0;

    if ((type == &PyBool_Type) &&
        !_PyArg_NoKeywords("bool", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("bool", PyTuple_GET_SIZE(args), 0, 1)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 1) {
        goto skip_optional;
    }
    x = PyObject_IsTrue(PyTuple_GET_ITEM(args, 0));
    if (x < 0) {
        goto exit;
    }
skip_optional:
    return_value = bool_new_impl(type, x);

exit:
    return return_value;
}
/*[clinic end generated code: output=e186a8060a0c25ab input=a9049054013a1b77]*/
