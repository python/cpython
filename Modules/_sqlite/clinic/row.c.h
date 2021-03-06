/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
pysqlite_row_new_impl(PyTypeObject *type, pysqlite_Cursor *cursor,
                      PyObject *data);

static PyObject *
pysqlite_row_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    pysqlite_Cursor *cursor;
    PyObject *data;

    if ((type == pysqlite_RowType) &&
        !_PyArg_NoKeywords("Row", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("Row", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    if (!PyObject_TypeCheck(PyTuple_GET_ITEM(args, 0), pysqlite_CursorType)) {
        _PyArg_BadArgument("Row", "argument 1", (pysqlite_CursorType)->tp_name, PyTuple_GET_ITEM(args, 0));
        goto exit;
    }
    cursor = (pysqlite_Cursor *)PyTuple_GET_ITEM(args, 0);
    if (!PyTuple_Check(PyTuple_GET_ITEM(args, 1))) {
        _PyArg_BadArgument("Row", "argument 2", "tuple", PyTuple_GET_ITEM(args, 1));
        goto exit;
    }
    data = PyTuple_GET_ITEM(args, 1);
    return_value = pysqlite_row_new_impl(type, cursor, data);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_row_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Returns the keys of the row.");

#define PYSQLITE_ROW_KEYS_METHODDEF    \
    {"keys", (PyCFunction)pysqlite_row_keys, METH_NOARGS, pysqlite_row_keys__doc__},

static PyObject *
pysqlite_row_keys_impl(pysqlite_Row *self);

static PyObject *
pysqlite_row_keys(pysqlite_Row *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_row_keys_impl(self);
}
/*[clinic end generated code: output=8d29220b9cde035d input=a9049054013a1b77]*/
