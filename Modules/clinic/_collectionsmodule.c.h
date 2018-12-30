/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
tuplegetter_new_impl(PyTypeObject *type, Py_ssize_t index, PyObject *doc);

static PyObject *
tuplegetter_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t index;
    PyObject *doc;

    if ((type == &tuplegetter_type) &&
        !_PyArg_NoKeywords("_tuplegetter", kwargs)) {
        goto exit;
    }
    if (!PyArg_ParseTuple(args, "nO:_tuplegetter",
        &index, &doc)) {
        goto exit;
    }
    return_value = tuplegetter_new_impl(type, index, doc);

exit:
    return return_value;
}
/*[clinic end generated code: output=83746071eacc28d3 input=a9049054013a1b77]*/
