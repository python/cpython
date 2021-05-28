/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_collections__count_elements__doc__,
"_count_elements($module, mapping, iterable, /)\n"
"--\n"
"\n"
"Count elements in the iterable, updating the mapping");

#define _COLLECTIONS__COUNT_ELEMENTS_METHODDEF    \
    {"_count_elements", (PyCFunction)(void(*)(void))_collections__count_elements, METH_FASTCALL, _collections__count_elements__doc__},

static PyObject *
_collections__count_elements_impl(PyObject *module, PyObject *mapping,
                                  PyObject *iterable);

static PyObject *
_collections__count_elements(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *iterable;

    if (!_PyArg_CheckPositional("_count_elements", nargs, 2, 2)) {
        goto exit;
    }
    mapping = args[0];
    iterable = args[1];
    return_value = _collections__count_elements_impl(module, mapping, iterable);

exit:
    return return_value;
}

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
    if (!_PyArg_CheckPositional("_tuplegetter", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(PyTuple_GET_ITEM(args, 0));
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    doc = PyTuple_GET_ITEM(args, 1);
    return_value = tuplegetter_new_impl(type, index, doc);

exit:
    return return_value;
}
/*[clinic end generated code: output=947186d369f50f1e input=a9049054013a1b77]*/
