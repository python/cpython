/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_csv_list_dialects__doc__,
"list_dialects($module, /)\n"
"--\n"
"\n"
"Return a list of all known dialect names.\n"
"\n"
"    names = csv.list_dialects()");

#define _CSV_LIST_DIALECTS_METHODDEF    \
    {"list_dialects", (PyCFunction)_csv_list_dialects, METH_NOARGS, _csv_list_dialects__doc__},

static PyObject *
_csv_list_dialects_impl(PyObject *module);

static PyObject *
_csv_list_dialects(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _csv_list_dialects_impl(module);
}

PyDoc_STRVAR(_csv_unregister_dialect__doc__,
"unregister_dialect($module, /, name)\n"
"--\n"
"\n"
"Delete the name/dialect mapping associated with a string name.\n"
"\n"
"    csv.unregister_dialect(name)");

#define _CSV_UNREGISTER_DIALECT_METHODDEF    \
    {"unregister_dialect", (PyCFunction)(void(*)(void))_csv_unregister_dialect, METH_VARARGS|METH_KEYWORDS, _csv_unregister_dialect__doc__},

static PyObject *
_csv_unregister_dialect_impl(PyObject *module, PyObject *name);

static PyObject *
_csv_unregister_dialect(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"name", NULL};
    PyObject *name;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:unregister_dialect", _keywords,
        &name))
        goto exit;
    return_value = _csv_unregister_dialect_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(_csv_get_dialect__doc__,
"get_dialect($module, /, name)\n"
"--\n"
"\n"
"Return the dialect instance associated with name.\n"
"\n"
"    dialect = csv.get_dialect(name)");

#define _CSV_GET_DIALECT_METHODDEF    \
    {"get_dialect", (PyCFunction)(void(*)(void))_csv_get_dialect, METH_VARARGS|METH_KEYWORDS, _csv_get_dialect__doc__},

static PyObject *
_csv_get_dialect_impl(PyObject *module, PyObject *name);

static PyObject *
_csv_get_dialect(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"name", NULL};
    PyObject *name;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:get_dialect", _keywords,
        &name))
        goto exit;
    return_value = _csv_get_dialect_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(_csv_field_size_limit__doc__,
"field_size_limit($module, /, new_limit=<unrepresentable>)\n"
"--\n"
"\n"
"Sets an upper limit on parsed fields.\n"
"\n"
"    csv.field_size_limit([limit])\n"
"\n"
"Returns old limit. If limit is not given, no new limit is set and\n"
"the old limit is returned");

#define _CSV_FIELD_SIZE_LIMIT_METHODDEF    \
    {"field_size_limit", (PyCFunction)(void(*)(void))_csv_field_size_limit, METH_VARARGS|METH_KEYWORDS, _csv_field_size_limit__doc__},

static PyObject *
_csv_field_size_limit_impl(PyObject *module, PyObject *new_limit);

static PyObject *
_csv_field_size_limit(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"new_limit", NULL};
    PyObject *new_limit = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:field_size_limit", _keywords,
        &new_limit))
        goto exit;
    return_value = _csv_field_size_limit_impl(module, new_limit);

exit:
    return return_value;
}
/*[clinic end generated code: output=16f9da39244ce644 input=a9049054013a1b77]*/
